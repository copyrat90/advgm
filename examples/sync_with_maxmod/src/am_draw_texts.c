// SPDX-FileCopyrightText: Copyright 2026 copyrat90
// SPDX-License-Identifier: 0BSD

#include "am_draw_texts.h"

#include "posprintf.h"

#include <advgm.h>
#include <maxmod.h>

#include <tonc.h>

#define AM_TEXTS_FRAME_X 60
#define AM_TEXTS_FRAME_Y 10

#define AM_TEXTS_LEFT_X 20
#define AM_TEXTS_TAB_X 70
#define AM_TEXTS_TOP_Y 40
#define AM_TEXTS_LINE_HEIGHT 13

#define AM_TEXT_HEIGHT 10

void am_draw_static_texts(void)
{
    tte_set_pos(AM_TEXTS_LEFT_X, AM_TEXTS_FRAME_Y);
    tte_write("frames:");

    tte_set_pos(AM_TEXTS_TAB_X, AM_TEXTS_TOP_Y);
    tte_write("PSG: advgm w/ timer1 ISR");

    tte_set_pos(AM_TEXTS_TAB_X, AM_TEXTS_TOP_Y + 1 * AM_TEXTS_LINE_HEIGHT);
    tte_write("  +");

    tte_set_pos(AM_TEXTS_TAB_X, AM_TEXTS_TOP_Y + 2 * AM_TEXTS_LINE_HEIGHT);
    tte_write("PCM: Maxmod");

    tte_set_pos(AM_TEXTS_LEFT_X, AM_TEXTS_TOP_Y + 5 * AM_TEXTS_LINE_HEIGHT);
    tte_write("Galactic Quest - Theme C");

    tte_set_pos(AM_TEXTS_LEFT_X, AM_TEXTS_TOP_Y + 6 * AM_TEXTS_LINE_HEIGHT);
    tte_write("  by potatoTeto  / CC BY-NC 4.0");
}

void am_redraw_music_position_texts(void)
{
    static int prev_maxmod_position = -1;

    static volatile int new_maxmod_position; // 0x30014a8
    static volatile uint32_t advgm_offset;   // 0x30014a4

    new_maxmod_position = mmGetPosition();

    if (new_maxmod_position != prev_maxmod_position)
    {
        advgm_offset = advgm_get_music_offset();

        tte_erase_rect(AM_TEXTS_LEFT_X, AM_TEXTS_TOP_Y, AM_TEXTS_TAB_X, AM_TEXTS_TOP_Y + AM_TEXT_HEIGHT);
        tte_erase_rect(AM_TEXTS_LEFT_X, AM_TEXTS_TOP_Y + 2 * AM_TEXTS_LINE_HEIGHT, AM_TEXTS_TAB_X,
                       AM_TEXTS_TOP_Y + 2 * AM_TEXTS_LINE_HEIGHT + AM_TEXT_HEIGHT);

        char text[12];

        posprintf(text, "%l", advgm_offset);
        tte_set_pos(AM_TEXTS_LEFT_X, AM_TEXTS_TOP_Y);
        tte_write(text);

        tte_set_pos(AM_TEXTS_LEFT_X, AM_TEXTS_TOP_Y + 2 * AM_TEXTS_LINE_HEIGHT);
        posprintf(text, "%l", new_maxmod_position);
        tte_write(text);

        prev_maxmod_position = new_maxmod_position;
    }
}

void am_redraw_elapsed_frames(unsigned elapsed_frames)
{
    tte_erase_rect(AM_TEXTS_FRAME_X, AM_TEXTS_FRAME_Y, AM_TEXTS_FRAME_X + 60, AM_TEXTS_FRAME_Y + AM_TEXT_HEIGHT);

    char text[12];

    posprintf(text, "%l", elapsed_frames);
    tte_set_pos(AM_TEXTS_FRAME_X, AM_TEXTS_FRAME_Y);
    tte_write(text);
}

void am_draw_init_error_and_exit(void)
{
    tte_erase_screen();

    tte_set_pos(AM_TEXTS_LEFT_X, AM_TEXTS_TOP_Y);
    tte_write("Maxmod failed to initialize!");

    for (;;)
        VBlankIntrWait();
}
