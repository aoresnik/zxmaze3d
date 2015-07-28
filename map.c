
#include <stdlib.h>
#include <string.h>

#define _MAP_C_INCLUDED

#include "map.h"
#include "common.h"

uchar map[MAP_SIZE];

/*
 * returns: nonzero if filled, 0 if not
 */
//uchar is_filled(int x, int y)
//{
//	return maze[x + MAZE_X*y] == '#';
//}

uchar is_filled(uchar x, uchar y);

#asm
_is_filled:
	ld	ix,0
	add	ix,sp
	ld	a,(ix+2)    ; y
	; a = y * 16
	rlca
	rlca
	rlca
	rlca
	add	(ix+4)    ; x
	ld hl,_map
	ld  d,0
	ld	e,a
	add hl,de
	ld a,(hl)
	cp '#'
	ld hl,0
	ret nz
	inc l
	ret
#endasm

#define is_wall(x0, y0, x1, y1) (is_filled(x0, y0) != is_filled(x1, y1))

// delta can be negative
uchar is_wall_delta(uchar x, uchar y, int delta);

#asm
_is_wall_delta:
	ld	ix,0
	add	ix,sp
	ld	a,(ix+4)    ; y
	; a = y * 16
	rlca
	rlca
	rlca
	rlca
	add	(ix+6)    ; x
	ld hl,_map
	ld  d,0
	ld	e,a
	add hl,de
	ld a,(hl)
	ld	e,(ix+2)    ; delta
	ld	d,(ix+3)    ; 
	add hl,de
	cp (hl)
	ld hl,0
	ret z
	inc l
	ret
#endasm

/*
uchar is_wall_delta(int x, int y, int delta)
{
	int i;
	i = x + MAZE_X*y;
	return maze[i] != maze[i + delta];
}
*/

// Is there a wall on the west of the cell?
#define is_wall_w(x0, y0) (is_wall_delta(x0, y0, -1))
// Is there a wall on the north of the cell
#define is_wall_n(x0, y0) (is_wall_delta(x0, y0, -16))

uchar is_wall_w_func(uchar x, uchar y)
{
	return is_wall_delta(x, y, -1);
}

uchar is_wall_n_func(uchar x, uchar y)
{
	return is_wall_delta(x, y, -16);
}

// Non time critical (used in map drawing)

int map_x(int maze_x)
{
	return 8*maze_x;
}

int map_y(int maze_y)
{
	return 8*maze_y;
}

int map_fx(int maze_f_x)
{
	return (8*maze_f_x) >> 8;
}

int map_fy(int maze_f_y)
{
	return (8*maze_f_y) >> 8;
}

/*
Equivalent to:
uchar maze_in_boundary_f(int f_x, int f_y)
{
	return f_x > 0 && f_x < 4096 && f_y > 0 && f_y < 2048;
}
*/
uchar maze_in_boundary_f(int f_x, int f_y);

#asm
_maze_in_boundary_f:
	ld	ix,0
	add	ix,sp

	ld	a,(ix+5)    ; f_x hi byte, int part
	cp  0
	jr  z, _out_of_boundary
	cp  MAZE_X
	jr  nc, _out_of_boundary
	
	ld	a,(ix+3)    ; f_y hi byte, int part
	cp  0
	jr  z, _out_of_boundary
	cp  MAZE_Y
	jr  nc, _out_of_boundary

_in_boundary:	
	ld hl,1
	ret
_out_of_boundary:
	ld hl,0
	ret
#endasm

void map_init(uchar *a_map)
{
	memcpy(map, a_map, MAP_SIZE);
}
