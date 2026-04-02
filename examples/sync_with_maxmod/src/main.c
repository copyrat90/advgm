// SPDX-FileCopyrightText: Copyright 2026 copyrat90
// SPDX-License-Identifier: 0BSD

#include "am_draw_texts.h"
#include "am_setup.h"
#include "am_sync.h"

#include <maxmod.h>
#include <tonc.h>

#include "soundbank.h"

// Your synced tune infos
#define MY_TUNE_MAXMOD MOD_GALACTIC_QUEST_MUS_THEME_C
extern const unsigned char galactic_quest_mus_theme_c[];
static const unsigned char* MY_TUNE_ADVGM = galactic_quest_mus_theme_c;
static const bool MY_TUNE_LOOP = true;

int main(void)
{
    am_setup_waitstates();
    am_sync_init();

    am_setup_gfx();
    am_draw_static_texts();

    am_setup_irq();
    VBlankIntrWait();

    if (!am_setup_maxmod())
        am_draw_init_error_and_exit();
    am_setup_advgm();

    VBlankIntrWait();

    am_sync_play(MY_TUNE_MAXMOD, MY_TUNE_ADVGM, MY_TUNE_LOOP);
    unsigned elapsed_frames = 0;

    for (;;)
    {
        VBlankIntrWait();

        mmFrame();

        key_poll();
        // Stop on B press
        if (key_hit(KEY_B))
        {
            elapsed_frames = 0;
            am_sync_stop();

            am_redraw_elapsed_frames(elapsed_frames);
        }
        // Play/Pause/Resume on A press
        if (key_hit(KEY_A))
        {
            if (!am_sync_playing())
                am_sync_play(MY_TUNE_MAXMOD, MY_TUNE_ADVGM, MY_TUNE_LOOP);
            else if (am_sync_paused())
                am_sync_resume();
            else
                am_sync_pause();
        }

        am_redraw_music_position_texts();

        if (am_sync_playing() && !am_sync_paused())
            am_redraw_elapsed_frames(elapsed_frames++);
    }
}
