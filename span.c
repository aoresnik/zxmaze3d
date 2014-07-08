
#include <stdio.h>
#include <stdlib.h>
#include <graphics.h>
#include <string.h>

#include "timing.h"

#define _SPAN_C_INCLUDED

#include "span.h"
#include "tables.h"

// Use pregenerated span draw routines (a bit faster, but uses 5k of RAM)
//#define USE_PG_SPAN_DRAW

#ifdef USE_PG_SPAN_DRAW
#include "ht_4x4_asm_routines.h"
#endif


/**
 * 8x8 bitmaps blocks of each intensity, 8 bytes each
 * Higher intensity means more white, lower more black
 * Computed with HT4x4Generate.java
 */
uchar ht_bits[] = {
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xee, 0xff, 0xff, 0xff, 0xee, 0xff, 0xff, 0xff, 
0xee, 0xff, 0xbb, 0xff, 0xee, 0xff, 0xbb, 0xff, 
0xee, 0xff, 0xaa, 0xff, 0xee, 0xff, 0xaa, 0xff, 
0xaa, 0xff, 0xaa, 0xff, 0xaa, 0xff, 0xaa, 0xff, 
0xaa, 0xdd, 0xaa, 0xff, 0xaa, 0xdd, 0xaa, 0xff, 
0xaa, 0xdd, 0xaa, 0x77, 0xaa, 0xdd, 0xaa, 0x77, 
0xaa, 0xdd, 0xaa, 0x55, 0xaa, 0xdd, 0xaa, 0x55, 
0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 
0xaa, 0x44, 0xaa, 0x55, 0xaa, 0x44, 0xaa, 0x55, 
0xaa, 0x44, 0xaa, 0x11, 0xaa, 0x44, 0xaa, 0x11, 
0xaa, 0x44, 0xaa, 0x00, 0xaa, 0x44, 0xaa, 0x00, 
0xaa, 0x00, 0xaa, 0x00, 0xaa, 0x00, 0xaa, 0x00, 
0x88, 0x00, 0xaa, 0x00, 0x88, 0x00, 0xaa, 0x00, 
0x88, 0x00, 0x22, 0x00, 0x88, 0x00, 0x22, 0x00, 
0x88, 0x00, 0x00, 0x00, 0x88, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

void span_init()
{
}


/*
 * cx is in character coordinatest
 * y is in pixel coordinates
 */
// uchar *screen_byte_for(uchar cx, uchar y)
// {
	// uchar mask;
	// return zx_pxy2saddr(0, y, &mask) + cx;
// }

uchar __CALLEE__ *screen_byte_for(uchar cx, uchar y);

#asm
._screen_byte_for
	; http://flockofspectrums.wordpress.com/2010/10/02/zx-spectrum-screen-memory/
	;
	; [0 | 1 | 0|y7 y6|y2 y1 y0][y5 y4 y3|x4 x3 x2 x1 x0]
	
	pop hl
	pop bc  ; y
	pop de  ; cx
	
	push hl ; return address
		
	ld	h,2
	
	ld  a,c ; y
	sla a	
	rl  h	; y7
	sla a   
	rl  h	; y6
	sla h
	sla h
	sla h
	
	and $E0 ; y5..y3
	or e    ; x4..x0
	ld l,a
	
	ld  a,c  ; y
	and $07  ; y2..y0
	or  h
	ld  h,a
	
	ret
#endasm


/**
 * Draws a 8 x dy block of pattern pointed to p_pat at screen address p_scr.
 * p_scr is expected to point at the top of the span in screen memory.
 */
uchar *draw_block_small(uchar *p_scr, uchar *p_pat, uchar y1, uchar y0);

#asm

_draw_block_small:
	ld	ix,0
	add	ix,sp
	
	ld	e,(ix+8)    ; p_scr
	ld	d,(ix+9)    ; 
	ld	l,(ix+6)    ; p_pat
	ld	h,(ix+7)    ; 
	ld  b,0
	ld  c,(ix+2)
	add hl,bc
	ld	a,(ix+4)    ; y1
	sub (ix+2)
	ld  b,a			; dy
	
_draw_block_small_l1:
	ld a,(hl)
	ld (de),a
	inc hl
	inc d
	djnz _draw_block_small_l1

	ret
#endasm

#ifdef USE_PG_SPAN_DRAW

void __CALLEE__ call_precompiled_span(int offset, int hl);

#asm
_call_precompiled_span:
	pop bc
	
	pop	hl    ; hl
	pop	de    ; offset
	
	push bc
	
	; call the routine
	push de
	
	ld	bc,0xAA55
	ld	de,0x55AA
	
	ret
	
	ret	
#endasm	

void draw_span(uchar cx, uchar y0, uchar y1, uchar intensity);

#asm
_draw_blocks:
	ld	ix,0
	add	ix,sp
	
	ld	e,(ix+8)    ; p_scr
	ld	d,(ix+9)    ; 
	
	ld	l,(ix+2)    ; p_pat
	ld	h,(ix+3)    ; 
	push hl

	ld  a,(ix+6) ; y0
	jr _draw_blocks_pg__

_draw_span:
	ld	ix,0
	add	ix,sp
	
	ld	d,2
	
	ld  a,(ix+6) ; y0
	ld  b,a ; y0 kept in b as long as possible
	sla a	
	rl  d	; y7
	sla a   
	rl  d	; y6
	sla d
	sla d
	sla d
	
	and $E0 ; y5..y3
	or (ix+8)    ; x4..x0
	ld e,a
	
	ld  a,b  ; y0
	and $07  ; y2..y0
	or  d
	ld  d,a
	; DE now contains the addres for byte at (cx, y0)
	
	ld  h,0
	ld	l,(ix+2)    ; intensity
	
	ld  a,b   ; y0
	
	add hl,hl
	add hl,hl
	add hl,hl
	ld bc,_ht_bits
	add hl,bc

	push hl			; keep p_pat on stack
	
	; HL now contains the pointer to pattern
	
_draw_blocks_pg__:
	; increase hl for yskip bytes
	; y0 already in a
	ld  b,a
	and 7
	ld	c,a         ; yskip = y0 & 7
	ld  a,b
	ld	b,0
	add hl,bc

	and $F8
	ld  b,a
	ld  a,(ix+4)
	sub b
	
	; if the drawing does not cross 8 boundary, use simpler routine
	ld b,8
	cp b
	jp c,_draw_block_pgl00__small
	
	ld  b,a         ; ny
	
	
	ld	a,c    		; yskip
	and a
	jr  z, _draw_blocks_pg__l0a   ; in case yskip=0, avoid drawing the beginning
	
	ld  a,8
	sub c			; yskip
	ld  b,a			; b = 8-yskip
	
_draw_block_pgl00:	
	ld a,(hl)
	ld (de),a
	inc hl
   	inc d
	djnz _draw_block_pgl00

	ld  a,(ix+6)
	and $F8
	ld  b,a
	ld  a,(ix+4)
	sub b
	sub 8
	ld  b,a         ; ny
	
	; de to beginning of line
	ld a,d
	sub 8
	ld d,a
	
	; Advance de to a next character line
	ld a,e
	add a,32
	ld e,a
	jr nc,_draw_blocks_pg__l0a
	
	; In case of overflow, add to the number of lines
	ld a,d
	add a,8
	ld d,a

_draw_blocks_pg__l0a:	

    ; the middle part: character aligned
	; de must contain screen
	; b is expected to contain ny-8 at this point
	; except if 8-aligned, in this case it is ny (so it is no. of lines to draw)
	ld a,b
	and $F8         ; obtain no of blocks
	jr z,_draw_blocks_pg__l2de  ; only do the looop in case of >0 blocks
	
;------------ draw b lines, leave HL pointing on the next line after end
	sub 8
	ld b,a

	ld	a,(ix+4)    ; y1
	srl a
	srl a
	srl a
	; must be filled with cell up to a
	and $07
	
	add b
	ld l,a
	ld h,0
	; offset from idx
	add hl,hl
	ld bc,_span_func_tbl
	add hl,bc
	ld	c,(hl)
	inc hl
	ld  b,(hl)
	
	pop hl
	push hl			; p_pat
	exx
	ld hl,_draw_blocks_pg__l323 ; return address
	push hl
	exx
	push bc         ; pregenerated routine address
	push de			; screen
	ld c,(hl)       ; load pattern into cedb
	inc hl
	ld e,(hl)
	inc hl
	ld d,(hl)
	inc hl
	ld b,(hl)

	pop hl			; screen

	ret  ; call the draw routine, address is on stack
_draw_blocks_pg__l323:
	
;--------------------------------------------------------------	
	
_draw_blocks_pg__l2:	
	pop de	; p_pat

	ld  a,(ix+4)
	and a,$07
	ret z			; return in case 0 more lines to draw
	ld b,a

_draw_blocks_pg__l4:	
	ld a,(de)
	ld (hl),a
	inc de
	inc h
	djnz _draw_blocks_pg__l4

	ret
	
_draw_blocks_pg__l2de:
	; _draw_blocks_pg__l2 expects hl to point to screen, but at the point of call it is in de
	ld h,d
	ld l,e
	jr _draw_blocks_pg__l2
	
_draw_block_pgl00__small:
	; c: yskip
	; a: ny
	; lines to draw: ny - yskip
	sub c
	pop bc
	ret z
	ld  b,a         ; lines to draw
_draw_block_pgl00__small1:	
	ld a,(hl)
	ld (de),a
	inc hl
   	inc d
	djnz _draw_block_pgl00__small1
	ret
#endasm

#if 0
void draw_precompiled_span_test_patterns()
{
	int i;
	uchar j;
	uchar x0, x1, x2;
	uchar y0, y1;
	int center;
    long duration;
	struct t_span *p_span, *p_span_end;
	int bytes;
	int kbps;
	
	bytes = 1;
	
	timing_start();
	
	for (i = 0; i < 24; i++)
	{
		y1 = 24-i;
		draw_span_pregenerated(i, 0, y1 * 8, i % 16);
	}

/*	
	for (i = 0; i <= 8; i++)
	{
		y1 = 16-i;
		// ni vec tocno
		call_precompiled_span(span_func_tbl[8 * (y1 - 1) + ((y1 - 1) % 8)], 16384 + 10 + i);
	}
*/
	duration = timing_elapsed();
	kbps = bytes / duration;
	
	printf("Drawn precompiled span test patterns - %d bytes in %ld ms, %d kB/s\n", bytes, duration, kbps);
}
#endif // #if 0

#else // USE_PG_SPAN_DRAW

void draw_span(uchar cx, uchar y0, uchar y1, uchar intensity);

#asm

._draw_blocks
	ld	ix,0
	add	ix,sp
	
	ld	e,(ix+8)    ; p_scr
	ld	d,(ix+9)    ; 
	
	ld	l,(ix+2)    ; p_pat
	ld	h,(ix+3)    ; 
	push hl			; keep p_pat on stack
	ld  a,(ix+6) ; y0
	jp _draw_blocks11111

._draw_span
	ld	ix,0
	add	ix,sp
	
	ld	d,2
	
	ld  b,(ix+6) ; y0 kept in b, c or a as long as possible
	ld  a,b
	sla a	
	rl  d	; y7
	sla a   
	rl  d	; y6
	sla d
	sla d
	sla d
	
	and $E0 ; y5..y3
	or (ix+8)    ; x4..x0
	ld e,a
	
	ld  a,b  ; y0
	and $07  ; y2..y0
	or  d
	ld  d,a
	; DE now contains the addres for byte at (cx, y0)
	
	ld  a,b   ; y0
	
	ld  h,0
	ld	l,(ix+2)    ; intensity
	add hl,hl
	add hl,hl
	add hl,hl
	ld bc,_ht_bits
	add hl,bc

	push hl			; keep p_pat on stack
	
	; HL now contains the pointer to pattern
	
_draw_blocks11111:
	; increase hl for yskip bytes
	; y0 in a
	ld  b,a    ; y0
	and 7
	ld	c,a         ; yskip = y0 & 7
	ld  a,b    ; y0
	ld	b,0
	add hl,bc       ; advance pointer to pattern

	and $F8          ; zero if < 8
	neg
	add a,(ix+4)     ; y1
	
	; if the drawing does not cross 8 boundary, use simpler routine
	cp 8
	jp c,_draw_block_l00__small
	
	ld  b,a         ; ny
	
	ld	a,c    		; yskip
	ld  c,b         ; ny
	and a
	jr  z, _draw_block_l0a   ; in case yskip=0, avoid drawing the beginning
	
	neg             ; a = -yskip
	add a,8
	ld  b,a			; b = 8-yskip
	
_draw_block_l00:	; 37T per iteration
	ld a,(hl)       ; 7T
	ld (de),a       ; 7T
	inc hl          ; 6T
   	inc d           ; 4T
	djnz _draw_block_l00 ; 13/8T

	ld  a,c         ; ny
	sub 8
	ld  b,a
	
	; de to beginning of line
	ld a,d
	sub 8
	ld d,a
	
	; Advance de to a next character line
	ld a,e
	add a,32
	ld e,a
	jr nc,_draw_block_l0a
	
	; In case of overflow, add to the number of lines
	ld a,d
	add a,8
	ld d,a

_draw_block_l0a:	

    ; the middle part: character aligned
	
	; b is expected to contain ny at this point
	
	srl b			; divide ny by 8 to obtain # of lines
	srl b
	srl b
	jr z,_draw_block_l2de  ; only do the looop in case of >0 lines

	pop hl   		; p_pat
	push hl
	push de
	ld c,(hl)
	inc hl
	ld e,(hl)
	inc hl
	ld d,(hl)
	inc hl
	ex	af,af
	ld a,(hl)
	ex	af,af

	pop hl			; screen
	
_draw_block_l1:
	
	ex	af,af

	; hl points to screen
	ld (hl),c		; 7 T
	inc h			; 4 T
	ld (hl),e
	inc h
	ld (hl),d
	inc h
	ld (hl),a
	inc h
	ld (hl),c
	inc h
	ld (hl),e
	inc h
	ld (hl),d
	inc h
	ld (hl),a

	ex	af,af

    ; Advance hl one line
	; Contrived. This diagram was helpful:
	; http://flockofspectrums.wordpress.com/2010/10/02/zx-spectrum-screen-memory/
	
	; It is not worth adding another loop for only inner-8-group lines because it would only save jr c
	; Except maybe in fully unrolled code
	
	; Advance to a next character line
	ld a,l
	add a,32
	ld l,a
	jr c,_draw_block_l1aa
	
	; return to the first line of character
	ld a,h
	and a,$F8
	ld h,a
	
	djnz _draw_block_l1
	
	jr _draw_block_l1b
_draw_block_l1aa:	
	; In case of overflow, add to the hi byte of address (bits 0-2 are 1)
	inc h

	djnz _draw_block_l1
	
 _draw_block_l1b:	
	
	; Draw the remaining ny&7 lines
	
_draw_block_l2:	

	pop de			; p_pat
	
	ld  a,(ix+4)
	and a,$07
	ret z			; return in case 0 more lines to draw
	ld b,a
	
_draw_block_l4:	
	ld a,(de)
	ld (hl),a
	inc de
	inc h
	djnz _draw_block_l4

	ret
	
_draw_block_l2de:
	; _draw_block_l2 expects hl to point to screen, but at the point of call it is in de
	ld h,d
	ld l,e
	jr _draw_block_l2
	
_draw_block_l00__small:
	; c: yskip
	; a: ny
	; lines to draw: ny - yskip
	sub c
	pop bc          ; p_pat (unused)
	ret z
	ld  b,a         ; lines to draw
_draw_block_l00__small1:	
	ld a,(hl)
	ld (de),a
	inc hl
   	inc d
	djnz _draw_block_l00__small1
	ret
#endasm

#endif // USE_PG_SPAN_DRAW



#asm

	XDEF	_draw_blocks

	
#endasm


/*
 * Updates the span at new heigth (assumes it has been drawn with p_span->prev_distidx)
 */
// void span_update1(struct maze_vspan *p_span)
// {
// 	// Making these variables static improves speed and is warranted, because
// 	// this function is on critical path and is non-recursive
// 	static uchar intensity, height;
// 	static uint distidx, prev_distidx;
// 
// 	distidx = p_span->distidx;
// 	prev_distidx = p_span->prev_distidx;
// #ifndef NDEBUG
// 	if (distidx > N_PRECALC_DRAW_DIST)
// 	{
// 		printf("Distance out of range: distidx=%d\n", distidx);
// 		distidx = N_PRECALC_DRAW_DIST;
// 	}
// #endif
// 
// 	// NOTE: relies that spans are vertically symetrical
// 	if (distidx > prev_distidx)
// 	{
// 		// The span is now farther away
// 		intensity = draw_intens[distidx];
// 	  
// 		// White (erase)
// 		draw_span(p_span->n, draw_heigths[prev_distidx], draw_heigths[distidx], INTENSITY_WHITE);
// 		draw_span(p_span->n, draw_heigths1[distidx], draw_heigths1[prev_distidx], INTENSITY_WHITE);
// 		
// 		if (intensity != draw_intens[prev_distidx])
// 		{
// 			draw_span(p_span->n, draw_heigths[distidx], draw_heigths1[distidx], intensity);
// 		}
// 		
// 		p_span->prev_distidx = distidx;
// 	}
// 	else if (distidx < prev_distidx)
// 	{
// 		// The span is now closer
// 		intensity = draw_intens[distidx];
// 		
// 		if (intensity != draw_intens[prev_distidx])
// 		{
// 			draw_span(p_span->n, draw_heigths[distidx], draw_heigths1[distidx], intensity);
// 		}
// 		else
// 		{
// 			// Extend existing span
// 			draw_span(p_span->n, draw_heigths[distidx], draw_heigths[prev_distidx], intensity);
// 			draw_span(p_span->n, draw_heigths1[prev_distidx], draw_heigths1[distidx], intensity);
// 		}
// 		
// 		p_span->prev_distidx = distidx;
// 	}
// }		

#define ASM_LOAD_PSPAN_BYTE(reg, offset) asm(\
"	ld "#reg",(ix+"#offset")\n");

#define ASM_LOAD_PSPAN_WORD(reg_hi, reg_lo, offset) asm(\
"	ld "#reg_lo",(ix+"#offset")\n\
	ld "#reg_hi",(ix+"#offset"+1)\n");

#define ASM_STORE_PSPAN_WORD(reg_hi, reg_lo, offset) asm(\
"	ld (ix+"#offset"),"#reg_lo"\n\
	ld (ix+"#offset"+1),"#reg_hi"\n");

#define ASM_PUSH_PSPAN_BYTE(offset) \
	ASM_LOAD_PSPAN_BYTE(l, offset); \
	asm(\
"	push hl\n");

#define ASM_LUT_LOAD_BYTE(reg, table, reg_offset) \
	asm(\
"	ld   hl,_"#table"\n\
	add hl,"#reg_offset"\n\
	ld "#reg",(hl)\n");

#define ASM_PUSH_LUT_BYTE(table, reg_offset) \
	ASM_LUT_LOAD_BYTE(l, table, reg_offset); \
	asm(\
"	push hl\n");

#define ASM_PUSH_CONST_BYTE(data) asm(\
"	ld l,"#data"\n\
	push hl\n");

#define ASM_CALL_DRAW_SPAN() asm(\
"	call _draw_span\n\
	ld   hl,8 ; pop 8 bytes of params\n\
	add  hl,sp\n\
	ld   sp,hl\n");

#define OFFSET_N 1
#define OFFSET_DISTIX 5
#define OFFSET_PREV_DISTIX 3

#define REG_DISTIDX de
#define REG_PREV_DISTIDX bc

void __FASTCALL__ span_update(struct maze_vspan *p_span)
{
#asm
	push hl
	pop  ix
	
#endasm

	ASM_LOAD_PSPAN_WORD(d, e, OFFSET_DISTIX);
	ASM_LOAD_PSPAN_WORD(b, c, OFFSET_PREV_DISTIX);
	
#asm
	ld  h,b
	ld  l,c
	and a
	sbc hl, de
	
	ret  z		; no change in distance
	
	jp   nc, _span_update_closer
	
_span_update_farther:	
	
#endasm
	
	ASM_PUSH_PSPAN_BYTE(OFFSET_N);
	ASM_PUSH_LUT_BYTE(draw_heigths, REG_PREV_DISTIDX);
	ASM_PUSH_LUT_BYTE(draw_heigths, REG_DISTIDX);
	ASM_PUSH_CONST_BYTE(INTENSITY_WHITE);
	
	ASM_PUSH_PSPAN_BYTE(OFFSET_N);
	ASM_PUSH_LUT_BYTE(draw_heigths1, REG_DISTIDX);
	ASM_PUSH_LUT_BYTE(draw_heigths1, REG_PREV_DISTIDX);
	ASM_PUSH_CONST_BYTE(INTENSITY_WHITE);
	
	ASM_LUT_LOAD_BYTE(a, draw_intens, REG_DISTIDX);
	ASM_LUT_LOAD_BYTE(l, draw_intens, REG_PREV_DISTIDX);
#asm

	cp   l
	jp   z,_span_update_draw2	; if equal intensities, return

	; draw the center area
#endasm

	ASM_PUSH_PSPAN_BYTE(OFFSET_N);
	ASM_PUSH_LUT_BYTE(draw_heigths, REG_DISTIDX);
	ASM_PUSH_LUT_BYTE(draw_heigths1, REG_DISTIDX);
	ASM_PUSH_LUT_BYTE(draw_intens, REG_DISTIDX);

#asm

	jp   _span_update_draw3
	
_span_update_closer:	
	
#endasm
	ASM_LUT_LOAD_BYTE(a, draw_intens, REG_DISTIDX);
	ASM_LUT_LOAD_BYTE(l, draw_intens, REG_PREV_DISTIDX);
#asm
	cp   l
	
	jp   z,_span_update_cl1	; if equal intensities, only extend
#endasm
	// different intensities, draw whole

	ASM_PUSH_PSPAN_BYTE(OFFSET_N);
	ASM_PUSH_LUT_BYTE(draw_heigths, REG_DISTIDX);
	ASM_PUSH_LUT_BYTE(draw_heigths1, REG_DISTIDX);
	ASM_PUSH_LUT_BYTE(draw_intens, REG_DISTIDX);
	
#asm
	jp   _span_update_draw1
	
_span_update_cl1:
#endasm
	
	// Extend exisitng span
	ASM_PUSH_PSPAN_BYTE(OFFSET_N);
	ASM_PUSH_LUT_BYTE(draw_heigths, REG_DISTIDX);
	ASM_PUSH_LUT_BYTE(draw_heigths, REG_PREV_DISTIDX);
	ASM_PUSH_LUT_BYTE(draw_intens, REG_DISTIDX);

	ASM_PUSH_PSPAN_BYTE(OFFSET_N);
	ASM_PUSH_LUT_BYTE(draw_heigths1, REG_PREV_DISTIDX);
	ASM_PUSH_LUT_BYTE(draw_heigths1, REG_DISTIDX);
	ASM_PUSH_LUT_BYTE(draw_intens, REG_DISTIDX);

#asm
	jp _span_update_draw2
	
_span_update_draw3:
#endasm
	// remember the changed distance
	ASM_STORE_PSPAN_WORD(d, e, OFFSET_PREV_DISTIX);
	ASM_CALL_DRAW_SPAN();
	ASM_CALL_DRAW_SPAN();
	ASM_CALL_DRAW_SPAN();
#asm
	ret
_span_update_draw2:	
#endasm
	// remember the changed distance
	ASM_STORE_PSPAN_WORD(d, e, OFFSET_PREV_DISTIX);
	ASM_CALL_DRAW_SPAN();
	ASM_CALL_DRAW_SPAN();
#asm
	ret
_span_update_draw1:	
#endasm
	// remember the changed distance
	ASM_STORE_PSPAN_WORD(d, e, OFFSET_PREV_DISTIX);
	ASM_CALL_DRAW_SPAN();
}


#asm

	XREF	_draw_heigths

	
#endasm
