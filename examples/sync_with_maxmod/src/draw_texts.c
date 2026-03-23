#include "draw_texts.h"

#include "posprintf.h"

#include <advgm.h>
#include <maxmod.h>

#include <tonc.h>

#define TEXTS_FRAME_X 60
#define TEXTS_FRAME_Y 10

#define TEXTS_LEFT_X 20
#define TEXTS_TAB_X 70
#define TEXTS_TOP_Y 40
#define TEXTS_LINE_HEIGHT 13

#define TEXT_HEIGHT 10

void draw_static_texts(void)
{
    tte_set_pos(TEXTS_LEFT_X, TEXTS_FRAME_Y);
    tte_write("frames:");

    tte_set_pos(TEXTS_TAB_X, TEXTS_TOP_Y);
    tte_write("PSG: advgm w/ timer1 ISR");

    tte_set_pos(TEXTS_TAB_X, TEXTS_TOP_Y + 1 * TEXTS_LINE_HEIGHT);
    tte_write("  +");

    tte_set_pos(TEXTS_TAB_X, TEXTS_TOP_Y + 2 * TEXTS_LINE_HEIGHT);
    tte_write("PCM: Maxmod");

    tte_set_pos(TEXTS_LEFT_X, TEXTS_TOP_Y + 5 * TEXTS_LINE_HEIGHT);
    tte_write("Galactic Quest - Theme C");

    tte_set_pos(TEXTS_LEFT_X, TEXTS_TOP_Y + 6 * TEXTS_LINE_HEIGHT);
    tte_write("  by potatoTeto  / CC BY-NC 4.0");
}

void redraw_music_position_texts(void)
{
    static int prev_maxmod_position = -1;

    const int new_maxmod_position = mmGetPosition();

    if (new_maxmod_position != prev_maxmod_position)
    {
        const uint32_t advgm_offset = advgm_get_music_offset();

        tte_erase_rect(TEXTS_LEFT_X, TEXTS_TOP_Y, TEXTS_TAB_X, TEXTS_TOP_Y + TEXT_HEIGHT);
        tte_erase_rect(TEXTS_LEFT_X, TEXTS_TOP_Y + 2 * TEXTS_LINE_HEIGHT, TEXTS_TAB_X,
                       TEXTS_TOP_Y + 2 * TEXTS_LINE_HEIGHT + TEXT_HEIGHT);

        char text[12];

        posprintf(text, "%l", advgm_offset);
        tte_set_pos(TEXTS_LEFT_X, TEXTS_TOP_Y);
        tte_write(text);

        tte_set_pos(TEXTS_LEFT_X, TEXTS_TOP_Y + 2 * TEXTS_LINE_HEIGHT);
        posprintf(text, "%l", new_maxmod_position);
        tte_write(text);

        prev_maxmod_position = new_maxmod_position;
    }
}

void redraw_elapsed_frames(unsigned elapsed_frames)
{
    tte_erase_rect(TEXTS_FRAME_X, TEXTS_FRAME_Y, TEXTS_FRAME_X + 60, TEXTS_FRAME_Y + TEXT_HEIGHT);

    char text[12];

    posprintf(text, "%l", elapsed_frames);
    tte_set_pos(TEXTS_FRAME_X, TEXTS_FRAME_Y);
    tte_write(text);
}

[[noreturn]] void draw_init_error_and_exit(void)
{
    tte_erase_screen();

    tte_set_pos(TEXTS_LEFT_X, TEXTS_TOP_Y);
    tte_write("Maxmod failed to initialize!");

    for (;;)
        VBlankIntrWait();
}
