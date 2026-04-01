#include "setup.h"

#include "sync.h"

// Seperate source file, because libugba definitions collapse with libtonc's.
#include <ugba/interrupts.h>

#include <maxmod.h>

void setup_irq(void)
{
    // libtonc's IRQ handler is buggy, so we're using libugba's here.
    IRQ_Init();

    IRQ_SetHandler(IRQ_TIMER1, sync_timer1_interrupt_handler);
    IRQ_Enable(IRQ_TIMER1);

    IRQ_SetHandler(IRQ_VBLANK, mmVBlank);
    IRQ_Enable(IRQ_VBLANK);
}
