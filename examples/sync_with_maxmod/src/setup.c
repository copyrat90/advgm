#include "setup.h"

#include <advgm.h>
#include <maxmod.h>
#include <mm_mas.h>
#include <tonc.h>

#include <stdatomic.h>
#include <stdint.h>

#include "build/gen_bin/soundbank.bin.h"

// To get the sample count, you need to reference the hidden `mmLayerMain` directly.
#include <core/player_types.h>
extern mpl_layer_information mmLayerMain;

#define MEMORY_BARRIER atomic_signal_fence(memory_order_seq_cst);

#define MAXMOD_CHANNELS_COUNT 16
#define MAXMOD_MIX_MODE MM_MIX_31KHZ
#define MAXMOD_MIX_LEN MM_MIXLEN_31KHZ

static EWRAM_BSS struct synced_play_states
{
    bool playing;
    bool loop;

    bool vblank_handled;
    bool advgm_updated;

    // Base timer value (quotient of cycles/64)
    uint16_t regular_tm_data;

    // Startup-only timer value
    uint16_t startup_tm_data;

    unsigned startup_vblank_delay_counter;
    atomic_uint startup_vblank_delay_needed;

    // Accumulate the sub-tick of a 64Hz timer.
    //
    // Denominator is always 8 (= 64/8), so it "overflows" on 8.
    // when that happens, `REG_TM1D` is set to `-(regular_tm_data + 1)`, instead of `-regular_tm_data`
    unsigned timer_accumulator;

    // This is accumulated to `timer_accumulator` everytime the advgm playback is updated.
    unsigned timer_remainder;

} play_states;

void sync_init(void)
{
    atomic_init(&play_states.startup_vblank_delay_needed, 0);
}

void sync_stop(void)
{
    // Stop the timer1
    REG_TM1CNT = 0;

    // Reset the playing flag ASAP so that the
    // vblank interrupt handler can't do weird things.
    MEMORY_BARRIER;
    play_states.playing = false;
    MEMORY_BARRIER;

    mmStop();
    advgm_stop();

    play_states.startup_vblank_delay_counter = 0;
    play_states.vblank_handled = false;
    play_states.advgm_updated = false;

    atomic_store_explicit(&play_states.startup_vblank_delay_needed, 0, memory_order_release);
}

void sync_play(mm_word maxmod_module_id, const uint8_t* advgm_music, bool loop)
{
    sync_stop();

    play_states.loop = loop;

    // Starts the playback.
    //
    // Note that the timer1 is not started yet, that's done in the delayed vblank callback.
    // Until then, advgm playback is never updated.
    advgm_play(advgm_music, loop);
    MEMORY_BARRIER;
    mmStart(maxmod_module_id, loop ? MM_PLAY_LOOP : MM_PLAY_ONCE);

    // Set the playing flag ASAP to not miss counting `startup_vblank_delay_counter` after `mmStart()`.
    MEMORY_BARRIER;
    play_states.playing = true;
    MEMORY_BARRIER;

    // You need to fetch these after `mmStart()` call,
    // because you need to reference the currently playing module info.
    // But `vblank_interrupt_handler()` will be called, so you can have a race condition if you're not careful.
    //
    // Though, it won't be so problematic because you need to at least wait for 2 vblanks.
    const mm_hword samples_per_tick = mmLayerMain.tickrate;
    const mm_hword sample_position = mmLayerMain.sampcount;

    // Calculate the startup delay.
    int samples_to_delay_on_startup = (int)samples_per_tick - sample_position;
    if (samples_to_delay_on_startup < 0)
        samples_to_delay_on_startup = 0;

    // Copied from `maxmod/source/gba/mixer.c`
    static const mm_hword mp_mixing_lengths[] = {
        136, 176, 224, 264,
        //  8khz, 10khz, 13khz, 16khz,
        304, 352, 448, 528
        // 18khz, 21khz, 27khz, 31khz
    };

    const mm_hword mixlen = mp_mixing_lengths[MAXMOD_MIX_MODE];

    const unsigned startup_delay_quotient = (unsigned)samples_to_delay_on_startup / mixlen;
    const unsigned startup_delay_remainder = (unsigned)samples_to_delay_on_startup % mixlen;

    // Don't forget the 2 more vblanks!
    const unsigned startup_vblank_delay_needed = 2 + startup_delay_quotient;

    // Reverse the Maxmod sample count calculation to obtain the timer value of the tempo.
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

    unsigned cycles = (unsigned)((1ULL << 23) * samples_per_tick * 5 / bpmdv);

    // Truncate the clock cycles to multiple of 8
    unsigned timer_numerator = cycles / 8;
    const unsigned timer_denominator = 64 / 8;

    unsigned timer_quotient = timer_numerator / timer_denominator;
    unsigned timer_remainder = timer_numerator % timer_denominator;

    play_states.regular_tm_data = (uint16_t)timer_quotient;
    play_states.timer_remainder = timer_remainder;

    // The startup delay should be converted to timer value, much like above.
    cycles = (unsigned)((1ULL << 23) * startup_delay_remainder * 5 / bpmdv);

    timer_numerator = cycles / 8; // trunc to multiple of 8

    timer_quotient = timer_numerator / timer_denominator;
    timer_remainder = timer_numerator % timer_denominator;

    play_states.startup_tm_data = (uint16_t)timer_quotient;
    play_states.timer_accumulator = timer_remainder;

    // Publish the `startup_vblank_delay_needed`
    atomic_store_explicit(&play_states.startup_vblank_delay_needed, startup_vblank_delay_needed, memory_order_release);
}

bool sync_playing(void)
{
    return play_states.playing;
}

static void timer1_interrupt_handler(void);

static void vblank_interrupt_handler(void)
{
    // Skips if not played with sync.
    if (!play_states.playing)
        return;

    // Skips if already started the advgm update.
    if (play_states.vblank_handled)
        return;

    // Checks if enough vblank has been passed or not.
    //
    // `startup_vblank_delay_needed` might not be already set to valid value,
    // so we need to check if it's valid (not-zero) to avoid race condition.
    const unsigned startup_vblank_delay_needed =
        atomic_load_explicit(&play_states.startup_vblank_delay_needed, memory_order_acquire);
    if (startup_vblank_delay_needed == 0 || play_states.startup_vblank_delay_counter + 1 < startup_vblank_delay_needed)
    {
        ++play_states.startup_vblank_delay_counter;
        return;
    }

    atomic_store_explicit(&play_states.startup_vblank_delay_needed, 0, memory_order_release);

    // Enough vblank waited, we'll now start the timer
    // (i.e. start the advgm update)
    play_states.vblank_handled = true;

    // Additionally delay the playback considering Maxmod startup delay.
    if (play_states.startup_tm_data != 0)
        REG_TM1D = -(uint16_t)play_states.startup_tm_data;
    else
    {
        play_states.advgm_updated = true;
        timer1_interrupt_handler();
    }

    // Start the advgm timer.
    REG_TM1CNT = TM_FREQ_64 | TM_IRQ | TM_ENABLE;
}

static void timer1_interrupt_handler(void)
{
    // Accumulate the sub-tick of the 64Hz timer
    play_states.timer_accumulator += play_states.timer_remainder;

    // If sub-tick has been accumulated to form an additional tick
    if (play_states.timer_accumulator >= 8)
    {
        play_states.timer_accumulator -= 8;

        // Wait an additional tick
        REG_TM1D = -(play_states.regular_tm_data + 1);
    }
    else
    {
        REG_TM1D = -play_states.regular_tm_data;
    }

    // If this was the first time advgm updated,
    // the timer must be using old delay value, which is invalid now.
    if (!play_states.advgm_updated)
    {
        play_states.advgm_updated = true;

        // Restart the timer to use `regular_tm_data` instead of `startup_tm_data`.
        REG_TM1CNT = 0;
        REG_TM1CNT = TM_FREQ_64 | TM_IRQ | TM_ENABLE;
    }

    // Update advgm playback last, for the restarted timer accuracy.
    MEMORY_BARRIER;
    const bool success = advgm_update();
    ((void)success);
}

void setup_advgm(void)
{
    advgm_set_master_volume(ADVGM_MASTER_VOLUME_FULL);
}

bool setup_maxmod(void)
{
    alignas(4) EWRAM_BSS static uint8_t module_channels_buffer[MM_SIZEOF_MODCH * MAXMOD_CHANNELS_COUNT];
    alignas(4) EWRAM_BSS static uint8_t active_channels_buffer[MM_SIZEOF_ACTCH * MAXMOD_CHANNELS_COUNT];
    alignas(4) EWRAM_BSS static uint8_t mixing_channels_buffer[MM_SIZEOF_MIXCH * MAXMOD_CHANNELS_COUNT];
    alignas(4) static uint8_t mixing_buffer[MAXMOD_MIX_LEN];
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

void setup_waitstates(void)
{
    REG_WAITCNT = WS_STANDARD;
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
