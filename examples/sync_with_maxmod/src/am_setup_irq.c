// SPDX-FileCopyrightText: Copyright 2026 copyrat90
// SPDX-License-Identifier: 0BSD

#include "am_setup.h"

#include "am_sync.h"

// Seperate source file, because libugba definitions collapse with libtonc's.
#include <ugba/interrupts.h>

#include <maxmod.h>

void am_setup_irq(void)
{
    // libtonc's IRQ handler is buggy, so we're using libugba's here.
    IRQ_Init();

    IRQ_SetHandler(IRQ_TIMER1, am_sync_timer1_interrupt_handler);
    IRQ_Enable(IRQ_TIMER1);

    IRQ_SetHandler(IRQ_VBLANK, mmVBlank);
    IRQ_Enable(IRQ_VBLANK);
}
