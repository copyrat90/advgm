// SPDX-FileCopyrightText: Copyright 2026 copyrat90
// SPDX-License-Identifier: 0BSD

#ifndef AM_DRAW_TEXTS_H
#define AM_DRAW_TEXTS_H

#ifdef __cplusplus
extern "C"
{
#endif

void am_draw_static_texts(void);
void am_redraw_music_position_texts(void);
void am_redraw_elapsed_frames(unsigned elapsed_frames);

void am_draw_init_error_and_exit(void);

#ifdef __cplusplus
}
#endif

#endif // AM_DRAW_TEXTS_H
