#include "draw_texts.h"
#include "setup.h"

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
    setup_gfx();
    draw_static_texts();

    if (!setup_maxmod())
        draw_init_error_and_exit();
    setup_advgm();

    setup_irq();

    VBlankIntrWait();

    sync_play(MY_TUNE_MAXMOD, MY_TUNE_ADVGM, MY_TUNE_LOOP);

    for (;;)
    {
        VBlankIntrWait();

        mmFrame();

        // Restart on A press
        key_poll();
        if (key_hit(KEY_A))
            sync_play(MY_TUNE_MAXMOD, MY_TUNE_ADVGM, MY_TUNE_LOOP);

        redraw_music_position_texts();
    }
}
