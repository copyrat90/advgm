#include "setup.h"

#include <advgm.h>
#include <mm_mas.h>
#include <maxmod.h>
#include <tonc.h>

#include <stdint.h>

#include "build/gen_bin/soundbank.bin.h"

// To get the sample count, you need to reference the hidden `mmLayerMain` directly.
#include "core/player_types.h"
extern mpl_layer_information mmLayerMain;

#define IWRAM_BSS // IWRAM is the default location for .bss symbols
#define EWRAM_BSS __attribute__((section(".sbss")))
#define MEMORY_BARRIER asm volatile("" ::: "memory")

#define MAXMOD_CHANNELS_COUNT 8
#define MAXMOD_MIX_MODE MM_MIX_31KHZ
#define MAXMOD_MIX_LEN MM_MIXLEN_31KHZ

static EWRAM_BSS struct synced_play_states
{
    unsigned vblank_delay_before_advgm_play;

    // Accumulate the sub-tick of a 64Hz timer.
    //
    // Denominator is always 8 (= 64/8), so it "overflows" on 8.
    // when that happens, `REG_TM1D` is set to `-(tm_data + 1)`, instead of `-tm_data`
    unsigned timer_accumulator;

    // This is accumulated to `timer_accumulator` everytime the advgm playback is updated.
    unsigned timer_remainder;

    // Base timer value (quotient of cycles/64)
    uint16_t tm_data;

    bool loop;
} play_states;

void sync_stop(void)
{
    // Stop the timer1
    REG_TM1CNT = 0;

    mmStop();
    advgm_stop();
}

void sync_play(mm_word maxmod_module_id, const uint8_t* advgm_music, bool loop)
{
    sync_stop();

    MEMORY_BARRIER;

    // To align the advgm playback with Maxmod's,
    // you need to start the advgm playback on the first vblank callback after the Maxmod playback has been started.
    play_states.vblank_delay_before_advgm_play = 1;

    play_states.loop = loop;

    MEMORY_BARRIER;

    // Starts the playback.
    //
    // Note that the timer1 is not started yet, that's done in the first vblank callback.
    // Until then, advgm playback is never updated.
    advgm_play(advgm_music, loop);
    mmStart(maxmod_module_id, loop ? MM_PLAY_LOOP : MM_PLAY_ONCE);
}

static void vblank_interrupt_handler(void)
{
    // Wait for the Maxmod playback to be started.
    if (!mmActive())
        return;

    // Delaying the `advgm_play()` call until the first vblank.
    if (play_states.vblank_delay_before_advgm_play == 0)
        return;
    if (--play_states.vblank_delay_before_advgm_play != 0)
        return;

    // Reverse the Maxmod sample count calculation to obtain the tempo.
    //
    // To know `mm_bpmdv` for specific Maxmod mixing rate,
    // I copied the `mp_bpm_divisors` LUT from `mmMixerInit()` in `gba/mixer.c`
    //
    // `MM_MAIN` layer calculations for `mpp_setbpm()` in `maxmod/source/core/mas.c`:
    //   0. layer_info->tickrate := (mm_bpmdv / raw_bpm) & ~1
    //   1. tempo := mm_bpmdv / layer_info->tickrate
    //   2. tick_rate (Hz) := tempo * 2 / 5
    //   3. clock_cycles := (1 << 24) / tick_rate
    //   4. truncated_clock_cycles : truncate above so that it's multiple of 8
    //   5. REG_TM1D := -truncated_clock_cycles / 64 (using 64Hz timer)
    //
    // Maxmod allows the tempo from 16 to 510 (considering `mm_mastertempo` multiplier),
    // So the `REG_TM1D` value for 64Hz timer is between -40947.5 and -1226.625
    static const mm_word mp_bpm_divisors[] = {
        20302, 26280, 33447, 39420,
        // 8khz, 10khz, 13khz, 16khz,
        45393, 52560, 66895, 78840
        // 18khz, 21khz, 27khz, 31khz
    };

    static const mm_word bpmdv = mp_bpm_divisors[MAXMOD_MIX_MODE];

    unsigned cycles = (unsigned)((1ULL << 23) * mmLayerMain.tickrate * 5 / bpmdv);

    // Truncate the clock cycles to multiple of 8
    unsigned timer_numerator = cycles / 8;
    const unsigned timer_denominator = 64 / 8;

    const unsigned timer_quotient = timer_numerator / timer_denominator;
    const unsigned timer_remainder = timer_numerator % timer_denominator;

    play_states.tm_data = (uint16_t)timer_quotient;
    play_states.timer_remainder = timer_remainder;
    play_states.timer_accumulator = timer_remainder;

    // Start the timer1
    REG_TM1D = -play_states.tm_data;
    REG_TM1CNT = TM_FREQ_64 | TM_IRQ | TM_ENABLE;
}

static void timer1_interrupt_handler(void)
{
    // Update advgm playback first
    const bool success = advgm_update();
    ((void)success);

    // Accumulate the sub-tick of the 64Hz timer
    play_states.timer_accumulator += play_states.timer_remainder;

    // If sub-tick has been accumulated to form an additional tick
    if (play_states.timer_accumulator >= 8)
    {
        play_states.timer_accumulator -= 8;

        // Wait an additional tick
        REG_TM1D = -(play_states.tm_data + 1);
    }
    else
    {
        REG_TM1D = -play_states.tm_data;
    }
}

void setup_advgm(void)
{
    advgm_set_master_volume(ADVGM_MASTER_VOLUME_FULL);
}

bool setup_maxmod(void)
{
    EWRAM_BSS static uint8_t module_channels_buffer[MM_SIZEOF_MODCH * MAXMOD_CHANNELS_COUNT];
    EWRAM_BSS static uint8_t active_channels_buffer[MM_SIZEOF_ACTCH * MAXMOD_CHANNELS_COUNT];
    EWRAM_BSS static uint8_t mixing_channels_buffer[MM_SIZEOF_MIXCH * MAXMOD_CHANNELS_COUNT];
    alignas(4) IWRAM_BSS static uint8_t mixing_buffer[MAXMOD_MIX_LEN];
    alignas(4) EWRAM_BSS static uint8_t wave_output_buffer[MAXMOD_MIX_LEN];

    mm_gba_system maxmod_configs = {
        .mixing_mode = MAXMOD_MIX_MODE,
        .mod_channel_count = MAXMOD_CHANNELS_COUNT,
        .mix_channel_count = MAXMOD_CHANNELS_COUNT,
        .module_channels = (mm_addr)module_channels_buffer,
        .active_channels = (mm_addr)active_channels_buffer,
        .mixing_channels = (mm_addr)mixing_channels_buffer,
        .mixing_memory = (mm_addr)mixing_buffer,
        .wave_memory = (mm_addr)wave_output_buffer,
        .soundbank = (mm_addr)soundbank_bin,
    };

    const bool success = mmInit(&maxmod_configs);
    if (success)
        mmSetVBlankHandler(vblank_interrupt_handler);

    return success;
}

void setup_gfx(void)
{
    REG_DISPCNT = DCNT_MODE0 | DCNT_BG0;
    tte_init_chr4c_default(0, BG_CBB(0) | BG_SBB(31));
}

void setup_irq(void)
{
    irq_init(NULL);

    irq_add(II_TIMER1, timer1_interrupt_handler);
    irq_enable(II_TIMER1);

    irq_add(II_VBLANK, mmVBlank);
    irq_enable(II_VBLANK);
}
