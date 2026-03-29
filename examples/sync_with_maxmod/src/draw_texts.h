#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

void draw_static_texts(void);
void redraw_music_position_texts(void);
void redraw_elapsed_frames(unsigned elapsed_frames);

[[noreturn]] void draw_init_error_and_exit(void);

#ifdef __cplusplus
}
#endif
