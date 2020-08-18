
#include <stdlib.h>
#include <string.h>

#include <graphics.h>
#include <spectrum.h>

#define _DRAW_C_INCLUDED

#include "draw-utils.h"
#include "map.h"
#include "common.h"

/**
 * Draws a 8 block of pattern pointed to p_pat at screen address p_char.
 * p_scr is expected to point at first line on screen memory.
 */
uchar __CALLEE__ *draw_char(uchar *p_scr, uchar *p_pat);

#asm
_draw_char:
    pop bc ; ret address
    pop hl ; p_char
    pop de ; p_screen
    push bc ; ret address
    ld  b,8            ; dy
    
_draw_char_l1:
    ld a,(hl)
    ld (de),a
    inc hl
    inc d
    djnz _draw_char_l1

    ret
#endasm

void draw_gray_area(uchar x0, uchar y0, uchar x1, uchar y1)
{
    uchar i;
    
    for (i = 0; i < 8; i += 4)
    {
        draw(x0, y1-i, x1-i, y0);
        
        draw(x1-i, y1, x1, y1-i);
    }
}

// Note: this is not the cleanest - uses map (move there)
void draw_map_line_fixed(int maze_x0, int maze_y0, int maze_x1, int maze_y1)
{
    draw(map_fx(maze_x0), map_fy(maze_y0), map_fx(maze_x1), map_fy(maze_y1));
}

void draw_stipled_block_horiz(uchar x0, uchar x1, uchar y)
{
    uchar x;
    
    for (x = x0; x <= x1; x += 2)
    {
        plot(x, y - 2);
        plot(x, y + 2);
    }
}

void draw_stipled_block_vert(uchar x, uchar y0, uchar y1)
{
    uchar y;
    
    for (y =  y0; y <= y1; y += 2)
    {
        plot(x - 2, y);
        plot(x + 2, y);
    }
}

void draw_attr_colored_area(uchar x0, uchar y0, uchar x1, uchar y1, uchar attr)
{
    uchar x, y;
    
    for (x = x0; x <= x1; x++)
    {
        for (y = y0; y <= y1; y++)
        {
            zx_cyx2aaddr(y, x)[0] = attr;
        }
    }
}

void draw_init()
{
}

