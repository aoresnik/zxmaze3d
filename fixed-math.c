
#include <stdlib.h>
#include <string.h>

#define _FIXED_MATH_C_INCLUDED

#include "fixed-math.h"
#include "tables.h"

// If enabled, a faster f_sqr variant is used, but it occupies a 
// fixed range of addresses 60k-62k
//#define UGLY_F_SQRT

#if N_PRECALC_DRAW_DIST!=512
#error N_PRECALC_DRAW_DIST is expected to be 512 by code in this file. Check if modifications are needed
#endif
#if N_PRECALC_FACTOR!=16
#error N_PRECALC_FACTOR is expected to be 16 by code in this file. Check if modifications are needed
#endif

// No saturation check, no early exit check, expects the arg1 to be bottom 8 bits
//    C_       arg1[7..0] (being shifted)
//    B_:D_:E  arg2[23..0] (being shifted)
//    A_:H_:L  product[23..0]
#define F_MULT_ITERATION_P_16b(id) asm(\
"	srl c\n\
	jr nc,_f_multiply_next_bit"#id"\n\
._f_multiply_bit_on"#id"\n\
	add hl,de\n\
	adc b\n\
._f_multiply_next_bit"#id"\n\
	sla e\n\
	rl d\n\
	rl b\n");

// Saturation check, early exit check, expects the arg1 to be top 8 bits
//    B_       arg1[15..8] (being shifted)
//    C_:D_:E  arg2[23..0] (being shifted)
//    A_:H_:L  product[31..8]
#define F_MULT_ITERATION_P_8b(id) asm(\
"	srl b\n\
	jr c, _f_multiply_bit_on"#id"\n\
	jp nz, _f_multiply_next_bit"#id"\n\
    ; if Z and NC, then exit the loop\n\
	jp  _f_multiply_exit_hl\n\
._f_multiply_bit_on"#id"\n\
	add hl,de\n\
	adc c\n\
    ; saturation, if top byte of the 32-bit product is not 0 or 255\n\
	jp z,_f_multiply_not_satur"#id"\n\
	cp 255\n\
	jp nz,_f_multiply_satur_ah\n\
._f_multiply_not_satur"#id"\n\
._f_multiply_next_bit"#id"\n\
	sla e\n\
	rl d\n\
	rl c\n");

// No saturation check, early exit check, expects the arg1 to be only 8 bits
//    C_       arg1[7..0] (being shifted)
//    B_:D_:E  arg2[23..0] (being shifted)
//    A_:H_:L  product[23..0]
#define F_MULT_ITERATION_P_8b_only(id) asm(\
"	srl c\n\
	jr c, _f_multiply_bit_on"#id"\n\
	jp nz, _f_multiply_next_bit"#id"\n\
	; if Z and NC, then exit the loop\n\
	jp _f_multiply_exit_ah\n\
._f_multiply_bit_on"#id"\n\
	add hl,de\n\
	adc b\n\
._f_multiply_next_bit"#id"\n\
	sla e\n\
	rl d\n\
	rl b\n");

int __CALLEE__ f_multiply(int f_a, int f_b)
{
#asm
;
; Registers used:
;   arg1 on A:D:E (sign extended to 24 bits)
;   arg2 on B:C (in case that is negative, arg1 and arg2 are both negated, so that it becomes nonegative, unsigned value)
;
; Result (bits 23..9 of the product) computed in A:H or H:L 
;
; (based on library source - l_long_mult.asm)
;

   pop  hl	 ; save the return address

   pop	bc   ; arg2
   pop	de   ; arg1
   
   push hl	 ; replace return address on stack

; sign extend arg1 in D:E to A:D:E
   xor  a
   bit	7,d   ; is arg1 negative
   jr	z,_f_multiply_arg1_nonneg
   dec	a   ; arg1  is negative, set sign extend to -1
._f_multiply_arg1_nonneg

   bit	7,b    ; arg2 sign
   jr	z,_f_multiply_nonneg

; ----------  code path for arg2 negative ------------
; negate both args, i.e. multiply each by -1, so that arg2 becomes nonnegative
; and then perform the algorithm for that case
; the catch is that if the product is +0x0080,0000, this is not representable as signed 16 bits,
; but is not detected as saturation, so this case is corrected at the end

._f_multiply_neg
   
   ; absolute value, i.e. negate arg2 (B:C), careful that -80,00h becomes 80,00h
   and a					   ; clear carry
   ld 	hl,0 
   sbc	hl,bc
   ld	b,h
   ld	c,l
   
   ; negate arg1 (24 bit A:D:E)
   and a					   ; clear carry
   ld	hl,0
   sbc	hl,de
   ex   de,hl    ; only needed DE=HL
   ld   h,a
   ld   a,0
   sbc  h

_f_multiply_arg1_zero:

; ----------  code path for arg2 non-negative ------------
; add shifted arg1 values to product
; arg2 is at this point unsigned 16-bit value (i.e. must be non-negative)

._f_multiply_nonneg

   inc b
   dec b   
   jp z, _f_multiply_p_8b_only

   ld  h,b
   ld  l,a
   push hl   ; save the high byte
   
; use B_ for arg1 byte 2, A for result byte 2
   ld b,a
   
   xor a
   ld  hl,0

#endasm
    F_MULT_ITERATION_P_16b(1);
    F_MULT_ITERATION_P_16b(2);
    F_MULT_ITERATION_P_16b(3);
    F_MULT_ITERATION_P_16b(4);
    F_MULT_ITERATION_P_16b(5);
    F_MULT_ITERATION_P_16b(6);
    F_MULT_ITERATION_P_16b(7);
    F_MULT_ITERATION_P_16b(8);
#asm

   ; shift and sign extend result in A:H to A:H:L 
   ld l,h
   ld h,a
   rla       ; trick: A contains H and is not needed anymore
   ld a,0
   sbc a
   
   ; arg1 bits 15..0 to D:E
   ld e,d
   ld d,b
   
   pop bc    ; arg2 bits 15..9 to B, sign extended arg1 bits 23..16 to C
  
#endasm
    F_MULT_ITERATION_P_8b(9);
    F_MULT_ITERATION_P_8b(10);
    F_MULT_ITERATION_P_8b(11);
    F_MULT_ITERATION_P_8b(12);
    F_MULT_ITERATION_P_8b(13);
    F_MULT_ITERATION_P_8b(14);
    F_MULT_ITERATION_P_8b(15);
    F_MULT_ITERATION_P_8b(16);
#asm

; the top 24 bits of product (bits 31..9) are in A:H:L, return bits 23..9
_f_multiply_exit_hl:

; fix the cases of result +0x0080,0000 when both args negative
    or a       ; are bits 31..23 zero, i.e. is result non-negative?
	ret nz     ; nonzero, result is negative

    ld a,0x80
    cp h
    ret nz
    
    xor a
    cp l
    ret nz

    ; result +0x0080,0000 not representable, return positive saturation
    jp  z,_f_multiply_satur_noneg
    
_f_multiply_p_8b_only:
    ld b,a   ; use B for arg1 bits 23..16
    xor a
   
    ld  hl,0

#endasm
    F_MULT_ITERATION_P_8b_only(b9);
    F_MULT_ITERATION_P_8b_only(b10);
    F_MULT_ITERATION_P_8b_only(b11);
    F_MULT_ITERATION_P_8b_only(b12);
    F_MULT_ITERATION_P_8b_only(b13);
    F_MULT_ITERATION_P_8b_only(b14);
    F_MULT_ITERATION_P_8b_only(b15);
    F_MULT_ITERATION_P_8b_only(b16);
#asm


; the middle 16 bits of the product (bits 23..9) are in A:H, return them
_f_multiply_exit_ah:
    ld	l,h
    ld	h,a
	ret
	
; ----------  in case of result overflow ------------

._f_multiply_satur_ah
   ; in this case, the product overflowed 16-bits
   ; use the sign of A (bits 31..24) which has correct sign and round to max value
   bit 7,a
   jp nz,_f_multiply_satur_neg
   
._f_multiply_satur_noneg
   ld hl,0x7FFF
   ret
._f_multiply_satur_neg
   ld hl,0x8000
   ret

#endasm
}

/*
Note: change the number of unrolled loops if parameters change
(currently, shift for 4)
Otherwise, equivalent of:
int distidx_from_f_dist(int f_dist)
{
	return f_dist * N_PRECALC_FACTOR / 256;
}
*/
unsigned int __FASTCALL__ distidx_from_f_dist(int f_dist);

#asm
_distidx_from_f_dist:
	srl h
	rr l
	srl h
	rr l
	srl h
	rr l
	srl h
	rr l
	ret
#endasm

/**
Note: change the number of unrolled loops if parameters change
(currently, shift for 4)
Otherwise, equivalent of:
int f_dist_from_distidx(uint distidx)
{
	return distidx * 256 / N_PRECALC_FACTOR;
}
*/
unsigned int __FASTCALL__ f_dist_from_distidx(uint distidx);

#asm
_f_dist_from_distidx:
	add hl,hl
	add hl,hl
	add hl,hl
	add hl,hl
	ret
#endasm

#ifdef UGLY_F_SQRT

/*
 * Generates asm for one iteration of searching in table
 * 
 * Params
 *  offset - contains the offset of guess in the table (one bit must be set)
 *  offsetmsb - must be the high byte of offset
 *  offsetlsb - must be the low byte of offset
 * 
 * Registers
 *  HL    - pointer to current guess,
 *          if the value of param is higher or equal than a guess at (HL+offset),
 *          sets HL to HL+offset, otherwise leaves HL unchanged.
 *  DE:BC - parameter (value being searched for)
 */
#define DISTIDX_ITERATION(offset) asm(\
"	ld  a,d\n\
	cp  (hl)\n\
	jr  c,_f16_stb_lower"#offset"_0\n\
	jr	nz,_f16_stb_higher"#offset"_0\n\
	ld  a,e\n\
	dec hl\n\
	cp  (hl)\n\
	jr  c,_f16_stb_lower"#offset"_1\n\
	jr	nz,_f16_stb_higher"#offset"_1\n\
	ld  a,b\n\
	dec hl\n\
	cp  (hl)\n\
	jr  c,_f16_stb_lower"#offset"_2\n\
	jr	nz,_f16_stb_higher"#offset"_2\n\
	ld  a,c\n\
	dec hl\n\
	cp  (hl)\n\
	jr  c,_f16_stb_lower"#offset"_3\n\
_f16_stb_higher"#offset"_3:\n\
	inc hl\n\
_f16_stb_higher"#offset"_2:\n\
	inc hl\n\
_f16_stb_higher"#offset"_1:\n\
	inc hl\n\
_f16_stb_higher"#offset"_0:\n\
	jp _f16_stb_next"#offset"\n\
_f16_stb_lower"#offset"_3:\n\
	inc hl\n\
_f16_stb_lower"#offset"_2:\n\
	inc hl\n\
_f16_stb_lower"#offset"_1:\n\
	inc hl\n\
_f16_stb_lower"#offset"_0:\n\
")

// Different cases for offset bit in LS byte or MS byte
// First try to add the offset, and if guess is larger, undo it
// (this can be speeded up a bit by aligning _f16_sqrs on 2k 
// and using set/res instructions, but complicates things)

#define DISTIDX_ITERATION_MSB(offset, bit) asm(\
"	set "#bit",h\n"); \
	DISTIDX_ITERATION(offset);\
asm(\
"	res "#bit",h\n\
_f16_stb_next"#offset":\n\
")

#define DISTIDX_ITERATION_LSB(offset, bit) asm(\
"	set "#bit",l\n"); \
	DISTIDX_ITERATION(offset); \
asm(\
"	res "#bit",l\n\
_f16_stb_next"#offset":\n\
")


uint __FASTCALL__ distidx_sqrt(unsigned long f16_l)
{
#asm
;_distidx_sqrt:

;	param is in de:hl at this point

	ld b,h
	ld c,l
	
	ld	hl, 0xF000+3   ; pointer to current point
	
_f_sqrt_loop1:

    ; try advancing for the step
#endasm

	DISTIDX_ITERATION_MSB(1024, 2);
	DISTIDX_ITERATION_MSB(512, 1);
	DISTIDX_ITERATION_MSB(256, 0);
	DISTIDX_ITERATION_LSB(128, 7);
	DISTIDX_ITERATION_LSB(64, 6);
	DISTIDX_ITERATION_LSB(32, 5);
	DISTIDX_ITERATION_LSB(16, 4);
	DISTIDX_ITERATION_LSB(8, 3);
	DISTIDX_ITERATION_LSB(4, 2);
	
#asm	
_f16_end:
	
    ; byte offset from pointer
	ld bc,0xF000+3
	and a
	sbc hl,bc
	
    ; index from byte offset
    srl h
	rr  l
    srl h
	rr  l
	
#endasm
}

void ugly_f_sqrt_init()
{
	memcpy(0xF000, f16_sqrs, N_PRECALC_DRAW_DIST * sizeof(long));
}


#else // UGLY_F_SQRT

/*
 * Generates asm for one iteration of searching in table
 * 
 * Params
 *  offset - contains the offset of guess in the table (one bit must be set)
 *  offsetmsb - must be the high byte of offset
 *  offsetlsb - must be the low byte of offset
 * 
 * Registers
 *  HL    - pointer to current guess,
 *          if the value of param is higher or equal than a guess at (HL+offset),
 *          sets HL to HL+offset, otherwise leaves HL unchanged.
 *  DE:BC - parameter (value being searched for)
 */
#define DISTIDX_ITERATION(offset) asm(\
"	ld  a,d\n\
	cp  (hl)\n\
	jr  c,_f16_stb_lower"#offset"_0\n\
	jr	nz,_f16_stb_higher"#offset"_0\n\
	ld  a,e\n\
	dec hl\n\
	cp  (hl)\n\
	jr  c,_f16_stb_lower"#offset"_1\n\
	jr	nz,_f16_stb_higher"#offset"_1\n\
	ld  a,b\n\
	dec hl\n\
	cp  (hl)\n\
	jr  c,_f16_stb_lower"#offset"_2\n\
	jr	nz,_f16_stb_higher"#offset"_2\n\
	ld  a,c\n\
	dec hl\n\
	cp  (hl)\n\
	jr  c,_f16_stb_lower"#offset"_3\n\
_f16_stb_higher"#offset"_3:\n\
	inc hl\n\
_f16_stb_higher"#offset"_2:\n\
	inc hl\n\
_f16_stb_higher"#offset"_1:\n\
	inc hl\n\
_f16_stb_higher"#offset"_0:\n\
	jp _f16_stb_next"#offset"\n\
_f16_stb_lower"#offset"_3:\n\
	inc hl\n\
_f16_stb_lower"#offset"_2:\n\
	inc hl\n\
_f16_stb_lower"#offset"_1:\n\
	inc hl\n\
_f16_stb_lower"#offset"_0:\n\
")

// Different cases for offset bit in LS byte or MS byte
// First try to add the offset, and if guess is larger, undo it
// (this can be speeded up a bit by aligning _f16_sqrs on 2k 
// and using set/res instructions, but complicates things)

#define DISTIDX_ITERATION_MSB(offset, offsetmsb) asm(\
"	ld  a,"#offsetmsb"\n\
	add h\n\
	ld  h,a\n"); \
	DISTIDX_ITERATION(offset);\
asm(\
"	ld  a,h\n\
	sub "#offsetmsb"\n\
	ld  h,a\n\
_f16_stb_next"#offset":\n\
")

#define DISTIDX_ITERATION_LSB(offset, offsetlsb) asm(\
"	ld  a,"#offsetlsb"\n\
	add l\n\
	ld  l,a\n\
	jp nc,_f16_stb_1_"#offset"\n\
	inc h\n\
_f16_stb_1_"#offset":\n"); \
	DISTIDX_ITERATION(offset); \
asm(\
"	ld  a,l\n\
	sub "#offsetlsb"\n\
	ld  l,a\n\
	jp nc,_f16_stb_next"#offset"\n\
	dec h\n\
_f16_stb_next"#offset":\n\
")


uint __FASTCALL__ distidx_sqrt(unsigned long f16_l)
{
#asm
;_distidx_sqrt:

;	param is in de:hl at this point

	ld b,h
	ld c,l
	
	ld	hl, _f16_sqrs+3   ; pointer to current point
	
_f_sqrt_loop1:

    ; try advancing for the step
#endasm

	DISTIDX_ITERATION_MSB(1024, 0x04);
	DISTIDX_ITERATION_MSB(512, 0x02);
	DISTIDX_ITERATION_MSB(256, 0x01);
	DISTIDX_ITERATION_LSB(128, 0x80);
	DISTIDX_ITERATION_LSB(64, 0x40);
	DISTIDX_ITERATION_LSB(32, 0x20);
	DISTIDX_ITERATION_LSB(16, 0x10);
	DISTIDX_ITERATION_LSB(8, 0x08);
	DISTIDX_ITERATION_LSB(4, 0x04);
	
#asm	
_f16_end:
	
    ; byte offset from pointer
	ld bc,_f16_sqrs+3
	and a
	sbc hl,bc
	
    ; index from byte offset
    srl h
	rr  l
    srl h
	rr  l
	
#endasm
}

#endif // UGLY_F_SQRT

/*
 * Calculates an approximation of square of a 8-bit shifted value (using f16_sqrs). 
 * Result is 16-bit shifted value.
 */

// long f_sqr_approx(int f_a)
// {
//      return f16_sqrs[distidx_from_f_dist(abs(f_a))];
// }

long __FASTCALL__ f_sqr_approx(int f_a);

#asm
_f_sqr_approx:
        // f_sqr(i) f16_sqrs[distidx_from_f_dist(abs(i))]
	; abs
	bit 7,h
	jr z,_f_sqr_approx_positive

	; negate hl
	and a		; clear C
	ld	b,h
	ld	c,l
	ld	hl,0
	sbc	hl,bc

_f_sqr_approx_positive:	

        ; distidx from f_dist shifts 4 left, offset from index 2 left
	srl h
	rr l
	srl h
	rr l
	
	ld a,l
	and a,0xFC
	ld l,a

	ld bc,_f16_sqrs
	add hl,bc
	
	; read long to return
	ld c,(hl)	; temporarily, to not clobber hl
	inc hl
	ld b,(hl)
	inc hl
	ld e,(hl)	
	inc hl
	ld d,(hl)
	
	ld h, b
	ld l, c

	; __FASTCALL__ result in DE:HL
	ret
#endasm


/*
int fixed_from_int(int i)
{
	return i << 8;
}
*/

int __FASTCALL__ fixed_from_int(int i);

#asm
_fixed_from_int:
	ld	h,l
	ld	l,0
	ret
#endasm

/*
Converted to asm, speedup from 1280 to 840 ms
int int_from_fixed(int fixed)
{
	return fixed / 256;
}
*/

unsigned int __FASTCALL__ int_from_fixed(unsigned int fixed);

#asm
_int_from_fixed:
	ld	l,h
	ld	h,0
	ret
#endasm

// Not used and causes FP libraries to be included, so it's commented out
/*
int fixed_from_float(float f)
{
	if (f > 127.99)
	{
		return 0x7FFF;
	}
	if (f < -127.99)
	{
		return -0x8000;
	}
	if (f == 1.0)
	{
		return 256;
	}
	else if (f == -1.0)
	{
		return -256;
	}
	return f * 256.0;
}

float float_from_fixed(int fixed)
{
	return fixed / 256.0;
}
*/

void fixed_math_init()
{
#ifdef UGLY_F_SQRT
	ugly_f_sqrt_init();
#endif
}

#asm
   
	XDEF _f_multiply
	XDEF _distidx_sqrt
	XDEF _f_sqr_approx
	XDEF _int_from_fixed
	XDEF _fixed_from_int
	XDEF _distidx_from_f_dist
	XDEF _f_dist_from_distidx

#endasm

