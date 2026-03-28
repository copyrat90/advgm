#pragma once

#include <mm_types.h>
#include <stdint.h>

void sync_init(void);

void sync_play(mm_word maxmod_module_id, const uint8_t* advgm_music, bool loop);
void sync_stop(void);

bool sync_playing(void);

void sync_vblank_interrupt_handler(void);
void sync_timer1_interrupt_handler(void);
