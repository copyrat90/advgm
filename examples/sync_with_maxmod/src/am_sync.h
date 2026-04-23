// SPDX-FileCopyrightText: Copyright 2026 copyrat90
// SPDX-License-Identifier: 0BSD

#ifndef AM_SYNC_H
#define AM_SYNC_H

#include <mm_types.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

void am_sync_play(mm_word maxmod_module_id, const uint8_t* advgm_music, bool loop);
void am_sync_stop(void);

void am_sync_pause(void);
void am_sync_resume(void);

bool am_sync_playing(void);
bool am_sync_paused(void);

void am_sync_vblank_interrupt_handler(void);
void am_sync_timer1_interrupt_handler(void);
mm_word am_sync_maxmod_tick_callback_handler(mm_word msg, mm_word param);

#ifdef __cplusplus
}
#endif

#endif // AM_SYNC_H
