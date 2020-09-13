#ifndef _FIXED_MATH_C_INCLUDED

/*
 * Must be called before any other routines
 */
extern void fixed_math_init();

/*
 * Fixed-point saturated signed multiply.
 * If the smaller param is second, it's faster.
 */
extern int __CALLEE__ f_multiply(int f_a, int f_b);

/*
 * Calculates an approximation of square of a 8-bit shifted value (using f16_sqrs).
 * Result is 16-bit shifted value.
 */
extern long __FASTCALL__ f_sqr_approx(int f_a);

/*
 * Returns the distidx representation of the approximation of the square root of the
 * specified 16-bit shifted fixed point number.
 */
extern uint __FASTCALL__ distidx_sqrt(unsigned long f16_l);

/*
 * See tables.h for definition of distidx
 */
extern unsigned int __FASTCALL__ distidx_from_f_dist(int f_dist);

/*
 * See tables.h for definition of distidx
 */
extern unsigned int __FASTCALL__ f_dist_from_distidx(uint distidx);

/*
 * Returns
 */
extern int __FASTCALL__ fixed_from_int(int i);

extern unsigned int __FASTCALL__ int_from_fixed(unsigned int fixed);

// No speedup from asm
#define fixed_ceil(x) (((x) + 0xFF) & 0x7F00)
#define fixed_floor(x) ((x) & 0x7F00)

#endif // defined _FIXED_MATH_C_INCLUDED
