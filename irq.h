#ifndef _IRQ_C_INCLUDED

/*
 * Sets the specified address as a interrupt routine which will be called every 20ms (uses IM2).
 */
extern void irq_set_isr(void *isr);

#endif // defined _IRQ_C_INCLUDED

