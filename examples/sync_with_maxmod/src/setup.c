#include "setup.h"

#include "setup_options.h"
#include "sync.h"

#include <advgm.h>
#include <maxmod.h>
#include <tonc.h>

#include "build/gen_bin/soundbank.bin.h"

void setup_advgm(void)
{
    advgm_set_master_volume(ADVGM_MASTER_VOLUME_FULL);
}

bool setup_maxmod(void)
{
    alignas(4) EWRAM_BSS static uint8_t module_channels_buffer[MM_SIZEOF_MODCH * SETUP_OPTIONS_MAXMOD_CHANNELS_COUNT];
    alignas(4) EWRAM_BSS static uint8_t active_channels_buffer[MM_SIZEOF_ACTCH * SETUP_OPTIONS_MAXMOD_CHANNELS_COUNT];
    alignas(4) EWRAM_BSS static uint8_t mixing_channels_buffer[MM_SIZEOF_MIXCH * SETUP_OPTIONS_MAXMOD_CHANNELS_COUNT];
    alignas(4) static uint8_t mixing_buffer[SETUP_OPTIONS_MAXMOD_MIX_LEN];
    alignas(4) EWRAM_BSS static uint8_t wave_output_buffer[SETUP_OPTIONS_MAXMOD_MIX_LEN];

    mm_gba_system maxmod_configs = {
        .mixing_mode = SETUP_OPTIONS_MAXMOD_MIX_MODE,
        .mod_channel_count = SETUP_OPTIONS_MAXMOD_CHANNELS_COUNT,
        .mix_channel_count = SETUP_OPTIONS_MAXMOD_CHANNELS_COUNT,
        .module_channels = (mm_addr)module_channels_buffer,
        .active_channels = (mm_addr)active_channels_buffer,
        .mixing_channels = (mm_addr)mixing_channels_buffer,
        .mixing_memory = (mm_addr)mixing_buffer,
        .wave_memory = (mm_addr)wave_output_buffer,
        .soundbank = (mm_addr)soundbank_bin,
    };

    const bool success = mmInit(&maxmod_configs);
    if (success)
        mmSetVBlankHandler(sync_vblank_interrupt_handler);

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

    irq_add(II_TIMER1, sync_timer1_interrupt_handler);
    irq_enable(II_TIMER1);

    irq_add(II_VBLANK, mmVBlank);
    irq_enable(II_VBLANK);
}
