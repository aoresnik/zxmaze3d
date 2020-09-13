
#include <stdio.h>
#include <stdlib.h>
#include <graphics.h>
#include <string.h>

#include "timing.h"

#define _SPAN_C_INCLUDED

#include "common.h"
#include "span.h"
#include "tables.h"

// Use run-time compiled span draw routines
#define USE_PG_SPAN_DRAW


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


#if defined(USE_PG_SPAN_DRAW)

void * entry_pts[193];

#define PRECOMP_BUF_SIZE 544

uchar precomp_buffer[PRECOMP_BUF_SIZE];

int precomp_pos;

void precomp_emit(uchar b)
{
    if (precomp_pos <= PRECOMP_BUF_SIZE)
    {
        precomp_buffer[precomp_pos] = b;
        precomp_pos++;
    }
    else
    {
        puts("Precomp buffer overflow. Increase the value of PRECOMP_BUF_SIZE and recompile");
        exit(127);
    }
}

void precomp_set_enry_point(int line, void *addr)
{
    bpoke(0xF800+line, ((unsigned int) addr) & 0x00FF);
    bpoke(0xF900+line, (((unsigned int) addr) & 0xFF00) >> 8);
}

void init_precomp_draw()
{
    int line;

    precomp_pos = 0;

    for (line = 0; line < 192; line++)
    {
        precomp_set_enry_point(line, &precomp_buffer[precomp_pos]);

        // Write a 8-pixel slice of the span
        switch (line % 4)
        {
            case 0:
                precomp_emit(0x71); //       LD   (HL),C
                break;
            case 1:
                precomp_emit(0x73); //       LD   (HL),E
                break;
            case 2:
                precomp_emit(0x72); //       LD   (HL),D
                break;
            case 3:
                precomp_emit(0x70); //       LD   (HL),B
        }

        // Move HL to new line
        if (((line + 1) % 64) == 0)
        {
            precomp_emit(0x3E); //       LD   A,20H
            precomp_emit(0x20); //
            precomp_emit(0x85); //       ADD  L
            precomp_emit(0x6F); //       LD   L,A
            precomp_emit(0x24); //       INC  H
        }
        else if (((line + 1) % 8) == 0)
        {
            precomp_emit(0x3E); //       LD   A,20H
            precomp_emit(0x20); //
            precomp_emit(0x85); //       ADD  L
            precomp_emit(0x6F); //       LD   L,A
            precomp_emit(0x3E); //       LD   A,F9H
            precomp_emit(0xF9); //
            precomp_emit(0x84); //       ADD  H
            precomp_emit(0x67); //       LD   H,A
        }
        else
        {
            precomp_emit(0x24); //       INC  H
        }
    }

    precomp_set_enry_point(192, &precomp_buffer[precomp_pos]);
    precomp_emit(0xC9); //       RET

    debug_printf("Generated precompiled span draw code, %d bytes\n", precomp_pos);
}

/*
 * Paints the 8-pixels wide vertical span in column cx (0..32), with Bayer halftone pattern
 * of intensity (but could in principle draw any other 8x8 pattern), from line y0 (0..191)
 * to line y1 (0..191); y1 > y0.
 *
 * Fills the specified span with halftone 4x4 pattern of intensity
 * (supported levels are from INTENSITY_BLACK to INTENSITY_WHITE).
 *
 * Uses the precompiled unrolled loops to draw.
 */
void draw_span(uchar cx, uchar y0, uchar y1, uchar intensity);

#asm
_draw_blocks:
    ld    ix,0
    add    ix,sp

    ld    e,(ix+8)    ; p_scr
    ld    d,(ix+9)    ;

    ld    l,(ix+2)    ; p_pat
    ld    h,(ix+3)    ;
    push hl

    ld  a,(ix+6) ; y0
    jr _draw_blocks_pg__

_draw_span:
    ld    ix,0
    add    ix,sp

; TODO: for draw_blocks
_draw_blocks_pg__:
    ld  a,(ix+6) ; y0
    and 0xF8
    ld  l,a

    ld  c,(ix+4)  ; y1

    ld  a,c
    and 0xF8
    cp  l
    jp  z,_draw_span_pg_case_small

    ld  h,0xF8
    ld  l,c
    ld  e,(hl)
    inc h
    ld  d,(hl)
    ld  a,(de)
    ld  c,a            ; save the previous content of the stop marker
    ld  a,0xC9
    ld  (de),a      ; write RET
    exx             ; save the address of the stop marker and prev byte

    ld hl,_draw_blocks_pg_ret
    push hl         ; return address from the draw code

    ld  l,(ix+6)  ; y0
    ld  h,0xF8
    ld  e,(hl)
    inc h
    ld  d,(hl)
    push de         ; entry point of the draw code, for line y0

    ld  h,0
    ld    l,(ix+2)    ; intensity
    add hl,hl
    add hl,hl
    add hl,hl
    ld bc,_ht_bits
    add hl,bc

    ; HL now contains the pointer to pattern

    ld c,(hl)       ; load pattern into cedb
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ld b,(hl)

    ; calculate the screen address
    ld    h,2

    ld  a,(ix+6) ; y0
    sla a
    rl  h    ; y7
    sla a
    rl  h    ; y6
    sla h
    sla h
    sla h

    and $E0 ; y5..y3
    or (ix+8)    ; x4..x0
    ld l,a

    ld  a,(ix+6) ; y0
    and $07  ; y2..y0
    or  h
    ld  h,a

    ret  ; call the draw routine, address is on stack
_draw_blocks_pg_ret:
    exx             ;
    ld a,c
    ld (de),a       ; restore the previous value of the end marker

    ret

._draw_span_pg_case_small
    ld    d,2

    ld  b,(ix+6) ; y0 kept in b, c or a as long as possible
    ld  a,b
    sla a
    rl  d    ; y7
    sla a
    rl  d    ; y6
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
    ld    l,(ix+2)    ; intensity
    add hl,hl
    add hl,hl
    add hl,hl
    ld bc,_ht_bits
    add hl,bc
    ; HL now contains the pointer to pattern

    ; increase hl for yskip bytes
    ; y0 in a
    ld  b,a    ; y0
    and 7
    ld    c,a         ; yskip = y0 & 7
    ld  a,b    ; y0
    ld    b,0
    add hl,bc       ; advance pointer to pattern

    neg
    add a,(ix+4)    ; y1
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

#else

/*
 * Paints the 8-pixels wide vertical span in column cx (0..32), with Bayer halftone pattern
 * of intensity (but could in principle draw any other 8x8 pattern), from line y0 (0..191)
 * to line y1 (0..191); y1 > y0.
 *
 * Fills the specified span with halftone 4x4 pattern of intensity
 * (supported levels are from INTENSITY_BLACK to INTENSITY_WHITE).
 *
 * (this is a regular version, without precompiled unrolled loops)
 *
 * NOTE: Why the complexity?
 *
 * In ZX Spectrum memory layout, it's slow to move pointer to a screen address to the next
 * line in general. But if the pointer points to a line y that is not y%8 == 7 (i.e. not the
 * last line in a 8x8 cell), it's very fast: you only increase it by 0x100.
 *
 * So within a 8x8 character cell, if HL points to a screen byte, you can move to the next
 * line with simple "INC H" instruction. This is by design (it speeds up drawing of characters
 * and makes up a bit of lost speed because Spectrum's lacks a true text mode).
 *
 * This function is optimized by making it keep track where it is and uses fast advance if it
 * can, slow advance if it can't.
 */
void draw_span(uchar cx, uchar y0, uchar y1, uchar intensity);

#asm

._draw_blocks
    ld    ix,0
    add    ix,sp

    ld    e,(ix+8)    ; p_scr
    ld    d,(ix+9)    ;

    ld    l,(ix+2)    ; p_pat
    ld    h,(ix+3)    ;
    push hl            ; keep p_pat on stack
    ld  a,(ix+6) ; y0
    jp _draw_blocks11111

._draw_span
    ld    ix,0
    add    ix,sp

    ld    d,2

    ld  b,(ix+6) ; y0 kept in b, c or a as long as possible
    ld  a,b
    sla a
    rl  d    ; y7
    sla a
    rl  d    ; y6
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
    ld    l,(ix+2)    ; intensity
    add hl,hl
    add hl,hl
    add hl,hl
    ld bc,_ht_bits
    add hl,bc

    push hl            ; keep p_pat on stack

    ; HL now contains the pointer to pattern

_draw_blocks11111:
    ; increase hl for yskip bytes
    ; y0 in a
    ld  b,a    ; y0
    and 7
    ld    c,a         ; yskip = y0 & 7
    ld  a,b    ; y0
    ld    b,0
    add hl,bc       ; advance pointer to pattern

    and $F8          ; zero if < 8
    neg
    add a,(ix+4)     ; y1

    ; if the drawing does not cross 8 boundary, use simpler routine
    cp 8
    jp c,_draw_block_l00__small

    ld  b,a         ; ny

    ld    a,c            ; yskip
    ld  c,b         ; ny
    and a
    jr  z, _draw_block_l0a   ; in case yskip=0, avoid drawing the beginning

    neg             ; a = -yskip
    add a,8
    ld  b,a            ; b = 8-yskip

_draw_block_l00:    ; 37T per iteration
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

    srl b            ; divide ny by 8 to obtain # of lines
    srl b
    srl b
    jr z,_draw_block_l2de  ; only do the looop in case of >0 lines

    pop hl           ; p_pat
    push hl
    push de
    ld c,(hl)
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ex    af,af
    ld a,(hl)
    ex    af,af

    pop hl            ; screen

_draw_block_l1:

    ex    af,af

    ; hl points to screen
    ld (hl),c        ; 7 T
    inc h            ; 4 T
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

    ex    af,af

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

    pop de            ; p_pat

    ld  a,(ix+4)
    and a,$07
    ret z            ; return in case 0 more lines to draw
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

    XDEF    _draw_blocks

#endasm

/*
 * Updates the span on screen at new heigth (assumes it has been drawn with p_span->prev_distidx)
 */
void __FASTCALL__ span_update(struct maze_vspan *p_span)
{
    #define ASM_LOAD_PSPAN_BYTE(reg, offset) asm(\
    "    ld "#reg",(ix+"#offset")\n");

    #define ASM_LOAD_PSPAN_WORD(reg_hi, reg_lo, offset) asm(\
    "    ld "#reg_lo",(ix+"#offset")\n\
        ld "#reg_hi",(ix+"#offset"+1)\n");

    #define ASM_STORE_PSPAN_WORD(reg_hi, reg_lo, offset) asm(\
    "    ld (ix+"#offset"),"#reg_lo"\n\
        ld (ix+"#offset"+1),"#reg_hi"\n");

    #define ASM_PUSH_PSPAN_BYTE(offset) \
        ASM_LOAD_PSPAN_BYTE(l, offset); \
        asm(\
    "    push hl\n");

    #define ASM_LUT_LOAD_BYTE(reg, table, reg_offset) \
        asm(\
    "    ld   hl,_"#table"\n\
        add hl,"#reg_offset"\n\
        ld "#reg",(hl)\n");

    #define ASM_PUSH_LUT_BYTE(table, reg_offset) \
        ASM_LUT_LOAD_BYTE(l, table, reg_offset); \
        asm(\
    "    push hl\n");

    #define ASM_PUSH_CONST_BYTE(data) asm(\
    "    ld l,"#data"\n\
        push hl\n");

    #define ASM_CALL_DRAW_SPAN() asm(\
    "    call _draw_span\n\
        ld   hl,8 ; pop 8 bytes of params\n\
        add  hl,sp\n\
        ld   sp,hl\n");

    #define OFFSET_N 1
    #define OFFSET_DISTIX 5
    #define OFFSET_PREV_DISTIX 3
    #define REG_DISTIDX de
    #define REG_PREV_DISTIDX bc

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

    ret  z        ; no change in distance

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
    jp   z,_span_update_draw2    ; if equal intensities, return

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

    jp   z,_span_update_cl1    ; if equal intensities, only extend
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

    #undef ASM_LOAD_PSPAN_BYTE
    #undef ASM_LOAD_PSPAN_WORD
    #undef ASM_STORE_PSPAN_WORD
    #undef ASM_PUSH_PSPAN_BYTE
    #undef ASM_LUT_LOAD_BYTE
    #undef ASM_PUSH_LUT_BYTE
    #undef ASM_PUSH_CONST_BYTE
    #undef ASM_CALL_DRAW_SPAN
    #undef OFFSET_N
    #undef OFFSET_DISTIX
    #undef OFFSET_PREV_DISTIX
    #undef REG_DISTIDX
    #undef REG_PREV_DISTIDX
}

/* C version of span_update() */
// void span_update_c(struct maze_vspan *p_span)
// {
//     // Making these variables static improves speed and is warranted, because
//     // this function is on critical path and is non-recursive
//     static uchar intensity, height;
//     static uint distidx, prev_distidx;
//
//     distidx = p_span->distidx;
//     prev_distidx = p_span->prev_distidx;
// #ifndef NDEBUG
//     if (distidx > N_PRECALC_DRAW_DIST)
//     {
//         printf("Distance out of range: distidx=%d\n", distidx);
//         distidx = N_PRECALC_DRAW_DIST;
//     }
// #endif
//
//     // NOTE: relies that spans are vertically symetrical
//     if (distidx > prev_distidx)
//     {
//         // The span is now farther away
//         intensity = draw_intens[distidx];
//
//         // White (erase)
//         draw_span(p_span->n, draw_heigths[prev_distidx], draw_heigths[distidx], INTENSITY_WHITE);
//         draw_span(p_span->n, draw_heigths1[distidx], draw_heigths1[prev_distidx], INTENSITY_WHITE);
//
//         if (intensity != draw_intens[prev_distidx])
//         {
//             draw_span(p_span->n, draw_heigths[distidx], draw_heigths1[distidx], intensity);
//         }
//
//         p_span->prev_distidx = distidx;
//     }
//     else if (distidx < prev_distidx)
//     {
//         // The span is now closer
//         intensity = draw_intens[distidx];
//
//         if (intensity != draw_intens[prev_distidx])
//         {
//             draw_span(p_span->n, draw_heigths[distidx], draw_heigths1[distidx], intensity);
//         }
//         else
//         {
//             // Extend existing span
//             draw_span(p_span->n, draw_heigths[distidx], draw_heigths[prev_distidx], intensity);
//             draw_span(p_span->n, draw_heigths1[prev_distidx], draw_heigths1[distidx], intensity);
//         }
//
//         p_span->prev_distidx = distidx;
//     }
// }


void span_init()
{
#ifdef USE_PG_SPAN_DRAW
    init_precomp_draw();
#endif
}

#asm

    XREF    _draw_heigths

#endasm
