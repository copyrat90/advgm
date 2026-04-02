// SPDX-FileCopyrightText: Copyright 2026 copyrat90
// SPDX-License-Identifier: 0BSD

#include "am_setup.h"

#include "am_setup_options.h"
#include "am_sync.h"

#include <advgm.h>
#include <maxmod.h>
#include <tonc.h>

#include "build/gen_bin/soundbank.bin.h"

void am_setup_advgm(void)
{
    advgm_set_master_volume(ADVGM_MASTER_VOLUME_FULL);
}

bool am_setup_maxmod(void)
{
    _Alignas(4)
        EWRAM_BSS static uint8_t module_channels_buffer[MM_SIZEOF_MODCH * AM_SETUP_OPTIONS_MAXMOD_CHANNELS_COUNT];
    _Alignas(4)
        EWRAM_BSS static uint8_t active_channels_buffer[MM_SIZEOF_ACTCH * AM_SETUP_OPTIONS_MAXMOD_CHANNELS_COUNT];
    _Alignas(4)
        EWRAM_BSS static uint8_t mixing_channels_buffer[MM_SIZEOF_MIXCH * AM_SETUP_OPTIONS_MAXMOD_CHANNELS_COUNT];
    _Alignas(4) static uint8_t mixing_buffer[AM_SETUP_OPTIONS_MAXMOD_MIX_LEN];
    _Alignas(4) EWRAM_BSS static uint8_t wave_output_buffer[AM_SETUP_OPTIONS_MAXMOD_MIX_LEN];

    mm_gba_system maxmod_configs = {
        .mixing_mode = AM_SETUP_OPTIONS_MAXMOD_MIX_MODE,
        .mod_channel_count = AM_SETUP_OPTIONS_MAXMOD_CHANNELS_COUNT,
        .mix_channel_count = AM_SETUP_OPTIONS_MAXMOD_CHANNELS_COUNT,
        .module_channels = (mm_addr)module_channels_buffer,
        .active_channels = (mm_addr)active_channels_buffer,
        .mixing_channels = (mm_addr)mixing_channels_buffer,
        .mixing_memory = (mm_addr)mixing_buffer,
        .wave_memory = (mm_addr)wave_output_buffer,
        .soundbank = (mm_addr)soundbank_bin,
    };

    const bool success = mmInit(&maxmod_configs);
    if (success)
    {
        mmSetEventHandler(am_sync_maxmod_tick_callback_handler);
        mmSetVBlankHandler(am_sync_vblank_interrupt_handler);
    }

    return success;
}

void am_setup_waitstates(void)
{
    REG_WAITCNT = WS_STANDARD;
}

void am_setup_gfx(void)
{
    REG_DISPCNT = DCNT_MODE0 | DCNT_BG0;
    tte_init_chr4c_default(0, BG_CBB(0) | BG_SBB(31));
}
