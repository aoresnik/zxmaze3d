
#include <stdlib.h>
#include <string.h>
#include <im2.h>
#include <stdio.h>

#define _IRQ_C_INCLUDED

#include "common.h"
#include "irq.h"
#include "timing.h"

#define ADDR_IRQ_TRAMPOLINE_1B        (ADDR_IRQ_TRAMPOLINE & 0x00FF)

#if ((ADDR_IRQ_TRAMPOLINE_1B + (ADDR_IRQ_TRAMPOLINE_1B * 256)) != ADDR_IRQ_TRAMPOLINE)
#error IRQ trampoline address (ADDR_IRQ_TRAMPOLINE) must have equal values for top byte and bottom byte 
#endif

#if ((ADDR_IRQ_VECTOR_TABLE & 0x00FF) != 0)
#error IRQ vector table (ADDR_IRQ_VECTOR_TABLE) must be aligned on 256-byte boundary
#endif

void irq_set_isr(void *isr)
{
    int l0, l1, l2, l3, l4;
    
    // See: http://www.z88dk.org/wiki/doku.php?id=library:interrupts
    #asm
    di
    #endasm
    im2_Init(ADDR_IRQ_VECTOR_TABLE);
    
    // In IM2, the byte read from bus might be different from 0xFF
    // workaround: fill all the bytes in the IRQ vector table with the same value,
    // so that any two consecutive bytes point to IRQ trampoline
    // (http://scratchpad.wikia.com/wiki/Interrupts)
    
    // initialize 257-byte im2 vector table with the address of trampoline
    memset(ADDR_IRQ_VECTOR_TABLE, ADDR_IRQ_TRAMPOLINE_1B, 257);       
    // POKE jump instruction and address to trampoline
    bpoke(ADDR_IRQ_TRAMPOLINE, 195);              
    wpoke(ADDR_IRQ_TRAMPOLINE+1, (unsigned int) (isr));   
    #asm
    ei
    #endasm
    
    debug_printf("Installed ISR\n");
    
    l0 = loops_until_interrupt();
    l1 = loops_until_interrupt();
    l2 = loops_until_interrupt();
    l3 = loops_until_interrupt();
    l4 = loops_until_interrupt();
    
    // Allow estimation of the impact of ISR on available CPU cycles
     debug_printf("Loops per interrupt: now %d, %d, %d, %d, %d, ...; orig %d\n", l0, l1, l2, l3, l4, t_loops);    
}
