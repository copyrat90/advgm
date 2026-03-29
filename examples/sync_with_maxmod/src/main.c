#include "draw_texts.h"
#include "setup.h"
#include "sync.h"

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
    setup_waitstates();
    sync_init();

    setup_gfx();
    draw_static_texts();

    setup_irq();
    VBlankIntrWait();

    if (!setup_maxmod())
        draw_init_error_and_exit();
    setup_advgm();

    VBlankIntrWait();

    sync_play(MY_TUNE_MAXMOD, MY_TUNE_ADVGM, MY_TUNE_LOOP);
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
            sync_stop();

            redraw_elapsed_frames(elapsed_frames);
        }
        // Play/Pause/Resume on A press
        if (key_hit(KEY_A))
        {
            if (!sync_playing())
                sync_play(MY_TUNE_MAXMOD, MY_TUNE_ADVGM, MY_TUNE_LOOP);
            else if (sync_paused())
                sync_resume();
            else
                sync_pause();
        }

        redraw_music_position_texts();

        if (sync_playing() && !sync_paused())
            redraw_elapsed_frames(elapsed_frames++);
    }
}
