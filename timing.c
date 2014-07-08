
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "common.h"

// Use assembly routines for better timing resolution
#define ENABLE_ASM_TIMING

/**
 * Waits for interrupt and returns a number of loops waited 
 */

#ifdef ENABLE_ASM_TIMING

uint loops_until_interrupt()
#asm
	
	ld hl,23672 ; FRAMES
	ld de,0
	
; Observe only a change in LSB of FRAMES

_load_frames:
	ld a,(hl)
	
	jr _cmp_frames2
_cmp_frames:	
	inc de
_cmp_frames2:
	cp (hl)
	jr z,_cmp_frames

	ld h,d
	ld l,e
	
	ret 
	
#endasm

#else // ENABLE_ASM_TIMING

int loops_until_interrupt()
{
    clock_t start;
	int count;
	
	count = 0;
	start = clock();
	while (clock() == start)
	{
		count++;
	}
	
	return count;
}

#endif // ENABLE_ASM_TIMING

long t_start;
long t_us_overhead;
uint t_loops;

void timing_start()
{
    loops_until_interrupt();
	t_start = clock();
}

/*
 * Returns time elapsed since timing_start() in ms
 * Takes at least 20 ms, since it busy-waits for timing interrupt.
 */
long timing_elapsed()
{
	uint l;
	long t_end;
	
	l = loops_until_interrupt();
    t_end = clock();
	
	return (t_end - t_start) * 20 - ((l * 20) / t_loops);
} 

/*
 * Returns time elapsed since timing_start() in us
 * Takes at least 20 ms, since it busy-waits for timing interrupt.
 */
long timing_elapsed_us()
{
	uint l;
	long t_end;
	
	l = loops_until_interrupt();
    t_end = clock();
	
	return ((t_end - t_start) * 20000L) - ((20000L * l) / t_loops) - t_us_overhead;
} 

void timing_init()
{
	uint t_res;

	// Wait for interrupt to occur first, so that the measurement means entire interval
    loops_until_interrupt();
	t_loops = loops_until_interrupt();
	
	t_res = (uint) (20000 / t_loops);
	
	debug_printf("Timer interrupt wait loop: %d per 20ms (resolution %d us)\n", t_loops, t_res);
	
	t_us_overhead = 0;
	timing_start();
	t_us_overhead = timing_elapsed_us();
	
	debug_printf("us timer: %ld us overhead per call\n", t_us_overhead);
}
