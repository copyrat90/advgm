#pragma once

#include <mm_types.h>
#include <stdbool.h>
#include <stdint.h>

void sync_init(void);

void sync_play(mm_word maxmod_module_id, const uint8_t* advgm_music, bool loop);
void sync_stop(void);

void sync_pause(void);
void sync_resume(void);

bool sync_playing(void);
bool sync_paused(void);

void sync_vblank_interrupt_handler(void);
void sync_timer1_interrupt_handler(void);
mm_word sync_maxmod_tick_callback_handler(mm_word msg, mm_word param);
