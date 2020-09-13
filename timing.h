
// Must be called before any other functions
void timing_init();

int loops_until_interrupt();

/*
 * Called immediately before the code whose time is to be measured.
 * The elapsed time can be obtained after that by one of timing_elapsed() or timing_elapsed_us()
 */
void timing_start();

/*
 * Returns time elapsed since timing_start() in ms
 * Takes at least 20 ms, since it busy-waits for timing interrupt.
 */
long timing_elapsed();

/*
 * Returns time elapsed since timing_start() in us
 * Takes at least 20 ms, since it busy-waits for timing interrupt.
 */
long timing_elapsed_us();

extern uint t_loops;

