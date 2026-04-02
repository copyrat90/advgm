// SPDX-FileCopyrightText: Copyright 2026 copyrat90
// SPDX-License-Identifier: 0BSD

#ifndef AM_SETUP_H
#define AM_SETUP_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

void am_setup_advgm(void);
bool am_setup_maxmod(void);

void am_setup_waitstates(void);
void am_setup_gfx(void);
void am_setup_irq(void);

#ifdef __cplusplus
}
#endif

#endif // AM_SETUP_H
