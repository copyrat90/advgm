#pragma once

#include <mm_types.h>
#include <stdint.h>

void setup_advgm(void);
bool setup_maxmod(void);

void setup_waitstates(void);
void setup_gfx(void);
void setup_irq(void);

void sync_play(mm_word maxmod_module_id, const uint8_t* advgm_music, bool loop);
void sync_stop(void);

bool sync_playing(void);
