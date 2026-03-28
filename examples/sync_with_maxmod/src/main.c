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

uint64_t s[2] = {3027351822319785405ULL, 7263045373434617570ULL};

uint64_t xorshift128plus(void)
{
    uint64_t x = s[0];
    uint64_t const y = s[1];
    s[0] = y;
    x ^= x << 23;                         // a
    s[1] = x ^ y ^ (x >> 17) ^ (y >> 26); // b, c
    return s[1] + y;
}

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

    unsigned frames_to_pause_resume = 3;

    for (;;)
    {
        VBlankIntrWait();

        mmFrame();

        ++elapsed_frames;

        if (elapsed_frames >= frames_to_pause_resume)
        {
            if (sync_paused())
                sync_resume();
            else
                sync_pause();

            const uint64_t r = xorshift128plus();

            if (r % 15 == 0)
            {
                // Test calling both pause and resume in a single frame
                if (sync_paused())
                    sync_resume();
                else
                    sync_pause();

                frames_to_pause_resume = elapsed_frames + 1 + r % 14;
            }
            else
            {
                frames_to_pause_resume = elapsed_frames + r % 15;
            }
        }

        redraw_music_position_texts();

        if (sync_playing() && !sync_paused())
            redraw_elapsed_frames(elapsed_frames);
    }
}
