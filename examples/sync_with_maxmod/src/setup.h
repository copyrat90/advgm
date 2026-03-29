#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

void setup_advgm(void);
bool setup_maxmod(void);

void setup_waitstates(void);
void setup_gfx(void);
void setup_irq(void);

#ifdef __cplusplus
}
#endif
