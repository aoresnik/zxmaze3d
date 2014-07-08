/******************************************************************************
 *
 * zxmaze3d
 *
 * Simple 3D room demo for ZX Spectrum. Drawn using halftones. 
 * Uses ray-casting algorithm (inspired by Wolfenstein 3D).
 *
 * by Andrej Oresnik 2014
 *
 ******************************************************************************/

#include <stdio.h>
#include <input.h>
#include <stdlib.h>
#include <conio.h> 
#include <string.h>
#include <math.h>

#include <graphics.h>
#include <games.h>
#include <sound.h>
#include <im2.h>

#include "common.h"
#include "cmd.h"
#include "timing.h"
#include "span.h"
#include "fixed-math.h"

#include "tables.h"

// Enables measuring and printing the times of various stages of rendering
#define DEBUG_FRAME_TIMES

// If enabled, a colored map of sectors is drawn after init
//#define DEBUG_DRAW_SECTORS_MAP

// Enables drawing all directions on map (otherwise only first and last)
//#define DEBUG_MAP_DRAW_ALL_DIRECTIONS

// Digital frames per second display (faster; if disabled, analog display is shown)
#define OPTION_DIGITAL_FPS

// Memory to reserve for sectors (at some point it s printed how much is used)
#define MAX_SECTORS 10

// Memory to reserve for boundaries (at some point it s printed how much is used)
#define MAX_BOUNDARIES 40

// -- Maze routines -----------------------------------------------------------

// The code is written with these dimensions in mind. It could work with max 16x16 maze.

#define MAZE_X 16
#define MAZE_Y 8

#define COLOR_WALLS	INK_BLUE
#define COLOR_BACKGROUND INK_BLACK
#define COLOR_FPS	INK_CYAN

#define CLIP_DISTANCE 0x0080

// Expected to be less than or equal to clip distance
#define MOVE_STEP 0x0010
#define MOVE_STEP_SHIFT 4

/*
 * Maze contents.
 */
char maze[] =  
 "################" 
 "#              #" 
 "#              #" 
 "#  ### ### #####" 
 "#  # # #########" 
 "#  # ###       #" 
 "#              #" 
 "################" 
;

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
	ld hl,_maze
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
	ld hl,_maze
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

// -- Drawing routines -------------------------------------------------------

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
	ld  b,8			; dy
	
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

void draw_maze_line_fixed(int maze_x0, int maze_y0, int maze_x1, int maze_y1)
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

// -- Map routines -----------------------------------------------------------

#ifndef NDEBUG
void view_debug_pathwalk_draw_on_map();
#endif

void draw_maze_map()
{
	uchar i, j;
	uchar x0, x1, y0, y1;
	
	y1 = map_y(0);
	for (i = 0; i < MAZE_Y; i++)
	{
		y0 = y1;
		y1 = map_y(i+1);
		x1 = map_x(0);
		for (j = 0; j < MAZE_X; j++)
		{
			x0 = x1;
			x1 = map_x(j+1);
			if (j > 0 && is_wall_w(j, i))
			{
				draw(x0, y0, x0, y1);
			}
			if (i > 0 && is_wall_n(j, i))
			{
				draw(x0, y0, x1, y0);
			}
			if (is_filled(j, i))
			{
				draw_gray_area(x0, y0, x1, y1);
			}
		}
	}
#ifndef NDEBUG
	view_debug_pathwalk_draw_on_map();
#endif	
}

// -- Maze sector routines ---------------------------------------------------

#define BOUND_TYPE_HORIZONTAL	0
#define BOUND_TYPE_VERTICAL	1

struct t_sector_boundary
{
	struct t_sector_boundary *next;
	
	uchar type;
	uchar n;
	
	// If type == BOUND_TYPE_HORIZONTAL, then y0==y1
	// else if type == BOUND_TYPE_VERTICAL, then x0==x1
	char x0,y0,x1,y1;
	
	// Fixed point version of coords
	int f_x0,f_y0,f_x1,f_y1;
	
	// The boundary has to be tracked so that it can be avoided while 
	// checking where does the ray cross
	struct t_sector_boundary *neighbour_boundary;
	struct t_maze_sector *neighbour_sector;
	
	// if placed here, memory corruption bug is avoided
	struct t_sector_boundary *next_in_edge;
};

/*
 * One of the edges, consisting of boudary
 */
struct t_sector_edge
{
	// Linked list of boundaires of sector
	struct t_sector_boundary *boundaries;
	
	uchar type;
	//uchar x0,y0,x1,y1;
	
	// Number of bounaries that this edge consists of
	uchar nbounds;
	
};

struct t_maze_sector
{
	char x0,y0,x1,y1;
	uchar n;
	
	// Linked list of boundaires of sector
	struct t_sector_boundary *boundaries;
	
	struct t_sector_edge edge_n, edge_e, edge_s, edge_w;
};

struct t_maze_sector sectors[MAX_SECTORS];

int nsectors = 0;

struct t_sector_boundary bounds[MAX_BOUNDARIES];

int n_all_bounds = 0;

/**
 * Which sector is on block x+MAZE_X*y? MAZE_X*MAZE_Y entries.
 */
struct t_maze_sector *sectors_on_blocks[128];

struct t_maze_sector *get_sector_on_blocks(uchar x, uchar y)
{
	return sectors_on_blocks[x + MAZE_X*y];
}

int add_sector(uchar x0, uchar y0, uchar x1, uchar y1)
{
	struct t_maze_sector *p_sector;
	
#ifndef NDEBUG
	if (nsectors >= MAX_SECTORS)
	{
		debug_printf("Sector limit %d exceeded, increase MAX_SECTORS and recompile \n" + MAX_SECTORS);
		exit(0);
	}
#endif	
	
	p_sector = &sectors[nsectors];
	
	p_sector->x0 = x0;
	p_sector->y0 = y0;
	p_sector->x1 = x1;
	p_sector->y1 = y1;
	p_sector->boundaries = NULL;
	p_sector->n = nsectors;
	
	nsectors++;
	return p_sector->n;
}

void sectors_draw_bound(struct t_sector_boundary *p_bound)
{
	uchar x0, x1, y0, y1;
	
	if (p_bound->x1 > p_bound->x0)
	{
		x0 = map_x(p_bound->x0);
		x1 = map_x(p_bound->x1);
	}
	else
	{
		x1 = map_x(p_bound->x0);
		x0 = map_x(p_bound->x1);
	}
	if (p_bound->y1 > p_bound->y0)
	{
		y0 = map_y(p_bound->y0);
		y1 = map_y(p_bound->y1);
	}
	else
	{
		y1 = map_y(p_bound->y0);
		y0 = map_y(p_bound->y1);
	}
	
	if (p_bound->type == BOUND_TYPE_HORIZONTAL)
	{
		draw_stipled_block_horiz(x0, x1, y0);
		draw(x0, y0-3, x0, y0+3);
		draw(x1, y0-3, x1, y0+3);
	}
	else
	{
		draw_stipled_block_vert(x0, y0, y1);
		draw(x0-3, y0, x0+3, y0);
		draw(x0-3, y1, x0+3, y1);
	}
}

/*
 * returns: 
 * 1 no filled blocks exist, 
 * 0 at least one nonfilled block or other sector is present in the area
 */
uchar is_non_filled_block(uchar x0, uchar y0, uchar x1, uchar y1)
{
	uchar x, y;
	
	for (x = x0; x <= x1; x++)
	{
		for (y = y0; y <= y1; y++)
		{
			if (is_filled(x, y) || (get_sector_on_blocks(x, y) != NULL))
			{
				return 0;
			}
		}
	}
	
	return 1;
}

void try_increases(struct t_maze_sector *sect, uchar dx0, uchar dy0, uchar dx1, uchar dy1)
{
	while (is_non_filled_block(sect->x0 - dx0, sect->y0 - dy0, sect->x1 + dx1, sect->y1 + dy1))
	{
		sect->x0 = sect->x0 - dx0;
		sect->y0 = sect->y0 - dy0;
		sect->x1 = sect->x1 + dx1;
		sect->y1 = sect->y1 + dy1;
	}
}

int find_max_rect(struct t_maze_sector *sect)
{
	try_increases(sect, 1, 1, 0, 0);
	try_increases(sect, 0, 0, 1, 1);
	try_increases(sect, 1, 0, 0, 0);
	try_increases(sect, 0, 0, 1, 0);
	try_increases(sect, 0, 1, 0, 0);
	try_increases(sect, 0, 0, 0, 1);
}

int add_boundary(struct t_maze_sector *sect, struct t_maze_sector *bound_sect, uchar x0, uchar y0, uchar x1, uchar y1)
{
	int i;
	struct t_sector_boundary *p_bound;

#ifndef NDEBUG
	if (n_all_bounds >= MAX_BOUNDARIES)
	{
		debug_printf("Boundaries limit %d exceeded, increase MAX_BOUNDARIES and recompile \n", MAX_BOUNDARIES);
		exit(0);
	}
	if ((x0 == x1) && (y0 == y1))
	{
		debug_printf("ERROR: Empty boundary: x=%d y=%d\n", x0, y0);
	}
#endif	

	
	p_bound = &bounds[n_all_bounds];
	p_bound->n = n_all_bounds;
	p_bound->type = (x1 != x0) ? BOUND_TYPE_HORIZONTAL : BOUND_TYPE_VERTICAL;
	p_bound->x0 = x0;
	p_bound->y0 = y0;
	p_bound->x1 = x1;
	p_bound->y1 = y1;
	p_bound->f_x0 = fixed_from_int(x0);
	p_bound->f_y0 = fixed_from_int(y0);
	p_bound->f_x1 = fixed_from_int(x1);
	p_bound->f_y1 = fixed_from_int(y1);

	p_bound->neighbour_boundary = NULL;
	p_bound->neighbour_sector = bound_sect;
	
	p_bound->next = sect->boundaries;
	sect->boundaries = p_bound;
	
	n_all_bounds++;
	return p_bound->n;
}

void set_edge_boundary(struct t_sector_edge *edge, struct t_sector_boundary *bounds, uchar nbounds)
{
	int i;
	struct t_sector_boundary *p_bound;

#ifndef NDEBUG
	if (nbounds < 1)
	{
		debug_printf("ERROR: Empty sector edge!");
		exit(0);
	}
#endif	

	edge->boundaries = bounds;
	edge->nbounds = nbounds;
	edge->type = bounds->type;
	
	// Set up the linked list of boundaries on this edge by field next_in_edge
	p_bound = bounds;
	for (i = 0; i < nbounds-1; i++)
	{
		p_bound->next_in_edge = p_bound->next;
		p_bound = p_bound->next;
	}
	p_bound->next_in_edge = NULL;
}

void find_sectors(uchar x0, uchar y0, struct t_maze_sector *from_sect, struct t_sector_boundary *from_bound);

/*
 * lc_* line coordinates delta vs. coordinates for checking blocks
 */
int find_boundaries_line(struct t_maze_sector *sect, int n, char x, char y, char dx, char dy, char dirx, char diry, char lc_x, char lc_y)
{
	char i;
	char bound_x, bound_y;
	char dstx, dsty;
	int nbounds;
	struct t_maze_sector *cur_sect, *bound_sect;
	
	nbounds = 0;
	
	bound_x = x;
	bound_y = y;

	cur_sect = bound_sect = NULL;
	
	for (i = 0; i <= n; i++)
	{
		dstx = x + dirx;
		dsty = y + diry;
		
		if (is_filled(dstx, dsty))
		{
			// wall
			cur_sect = NULL;
		}
		else 
		{
			if (get_sector_on_blocks(dstx, dsty) == NULL)
			{
				find_sectors(dstx, dsty, NULL, NULL);
			}
			
			// Must not be null anymore
			cur_sect = get_sector_on_blocks(dstx, dsty);
		}
		
		// For first field, there's no change
		if (i == 0)
		{
			bound_sect = cur_sect;
		}
			
		if (bound_sect != cur_sect)
		{
			// Add boundary for existing sector only if sector changes (it can change to null)
			add_boundary(sect, bound_sect, x + lc_x, y + lc_y, bound_x + lc_x, bound_y + lc_y);
			nbounds++;
			bound_x = x;
			bound_y = y;
			bound_sect = cur_sect;
		}
		
		x += dx;
		y += dy;
	}

	add_boundary(sect, bound_sect, x + lc_x, y + lc_y, bound_x + lc_x, bound_y + lc_y);
	nbounds++;
	return nbounds;
}

int count_bounds_in_edge(struct t_sector_edge *p_edge)
{
	struct t_sector_boundary *p_bound;
	int n;
	
	n = 0;
	p_bound = p_edge->boundaries;
	while (p_bound != NULL)
	{
		p_bound = p_bound->next_in_edge;
		n++;
	}
	return n;
}

void find_boundaries(struct t_maze_sector *sect)
{
	int nbounds;
	
    // Add in the reverse order than the rendering code expects
	
	// z88dk seems to work with max 10 params (otherwise complains about too many args for a function)

	// West boundary, advancing down, checking left
	nbounds = find_boundaries_line(sect, sect->y1 - sect->y0, sect->x0, sect->y0, 0, 1, -1, 0, 0, 0);
    set_edge_boundary(&(sect->edge_w), sect->boundaries, nbounds);
	 
	// South boundary, advancing right, checking down
	nbounds = find_boundaries_line(sect, sect->x1 - sect->x0, sect->x0, sect->y1, 1, 0, 0, 1, 0, 1);
    set_edge_boundary(&(sect->edge_s), sect->boundaries, nbounds);

	// East boundary, advancing up, checking right
	nbounds = find_boundaries_line(sect, sect->y1 - sect->y0, sect->x1, sect->y1, 0, -1, 1, 0, 1, 1);
    set_edge_boundary(&(sect->edge_e), sect->boundaries, nbounds);

	// North boundary, advancing left, checking up
	nbounds = find_boundaries_line(sect, sect->x1 - sect->x0, sect->x1, sect->y0, -1, 0, 0, -1, 1, 0);
    set_edge_boundary(&(sect->edge_n), sect->boundaries, nbounds);
}

/**
 * Common boundaries go in opposite directions and have matching coords
 */
int boundaries_are_common(struct t_sector_boundary *bound1, struct t_sector_boundary *bound2)
{
	return (bound1->x0 == bound2->x1) && (bound1->x1 == bound2->x0) && (bound1->y0 == bound2->y1) && (bound1->y1 == bound2->y0);
}

struct t_sector_boundary *find_common_boundary(struct t_sector_boundary *bound, struct t_sector_boundary *bounds)
{
	struct t_sector_boundary *p_bound, *cmn;

	cmn = NULL;
	p_bound = bounds;
	while (p_bound != NULL)
	{
		if (boundaries_are_common(p_bound, bound))
		{
			cmn = p_bound;
		}
		p_bound = p_bound->next;
	}
	
	return cmn;
}

void find_common_boundaries()
{
	int i;
	int ncommon;
	struct t_sector_boundary *p_bound;
	
	ncommon = 0;
	
	for (i = 0; i < nsectors; i++)
	{
		p_bound = sectors[i].boundaries;
		while (p_bound != NULL)
		{
			if (p_bound->neighbour_sector != NULL)
			{
				ncommon++;
				p_bound->neighbour_boundary = find_common_boundary(p_bound, p_bound->neighbour_sector->boundaries);
#ifndef NDEBUG				
				if (p_bound->neighbour_boundary == NULL)
				{
					debug_printf("ERROR: No common boundary found for boundary pointing to neighbouring sector!\n");
				}
#endif				
			}
			else
			{
				p_bound->neighbour_boundary	= NULL;
			}
			p_bound = p_bound->next;
		}
	}
	
	debug_printf("%d common boundaries found\n", ncommon);
}

void find_sectors(uchar x0, uchar y0, struct t_maze_sector *from_sect, struct t_sector_boundary *from_bound)
{
	int i;
	uchar x, y;

	i = add_sector(x0, y0, x0, y0);
		
	find_max_rect(&sectors[i]);
	
	// Mark the blocks where the sector is
	for (x = sectors[i].x0; x <= sectors[i].x1; x++)
	{
		for (y = sectors[i].y0; y <= sectors[i].y1; y++)
		{
			sectors_on_blocks[x + MAZE_X*y] = &sectors[i];
		}
	}

	find_boundaries(&sectors[i]);
}

void sectors_init()
{
	int i;
	long t_sectors;

	timing_start();
	
	for (i = 0; i < 128; i++)
	{
		sectors_on_blocks[i] = NULL;
	}

	find_sectors(2, 2, NULL, NULL);
	
	debug_printf("%d sectors and %d boundaries in map\n", nsectors, n_all_bounds);

	find_common_boundaries();
	
	t_sectors = timing_elapsed();
	
	debug_printf("\nSectors calc time %ld ms\n", t_sectors);
	
}

// -- Ray casting routines ---------------------------------------------------

/**
 * Represents the viewpoint of the player: coordinates and direction.
 * It also encapsulates other precomputed data, that are dependent on them.
 */
struct viewport 
{
	/*
	 * Position in fixed point: 1.0 is 256
	 */
	int f_pos_x;
	int f_pos_y;

	/*
	 * Direction in units of 2*PI/N_DIRECTIONS
	 */
	uchar direction;

};

struct viewport view;

void view_init_origin()
{
	view.f_pos_x = 0x200;
	view.f_pos_y = 0x200;
	view.direction = 0;
}

struct maze_vspan spans[32];

/**
 * Sets up a demo configuration of spans.
 */
void view_init()
{
	int i;
	struct maze_vspan *p_span;

	for (i = 0; i < 32; i ++)
	{
		p_span = &spans[i];
		
		//p_span->intensity = N_INTENSITIES-i;
		//p_span->prev_intensity = N_INTENSITIES;
		p_span->n = i;
		//p_span->x = i;
		//p_span->prev_y0 = 96;
		//p_span->y0 = 96;
		//p_span->prev_y1 = 96;
		p_span->prev_distidx = 0;
		//p_span->prev_height = 96;
		//p_span->y1 = 96;
	}
	
}

void pos_crosses_bound(int *delta_x, int *delta_y)
{
}

uchar is_non_obstructed(int new_x, int new_y)
{
	if (is_filled(int_from_fixed(new_x + MOVE_STEP), int_from_fixed(new_y + MOVE_STEP)))
	{
		return 0;
	}
	if (is_filled(int_from_fixed(new_x + MOVE_STEP), int_from_fixed(new_y - MOVE_STEP)))
	{
		return 0;
	}
	if (is_filled(int_from_fixed(new_x - MOVE_STEP), int_from_fixed(new_y + MOVE_STEP)))
	{
		return 0;
	}
	if (is_filled(int_from_fixed(new_x - MOVE_STEP), int_from_fixed(new_y - MOVE_STEP)))
	{
		return 0;
	}
	return 1;
}

unsigned int __FASTCALL__ step_delta_from_dir_delta(int dir_delta);

#asm
_step_delta_from_dir_delta:
	sra h
	rr l
	sra h
	rr l
	sra h
	rr l
	sra h
	rr l
	ret
#endasm

void pos_move_forward(uchar direction)
{
	int new_x, new_y;
	
	new_x = view.f_pos_x + step_delta_from_dir_delta(f_dir_cos[view.direction]);
	new_y = view.f_pos_y + step_delta_from_dir_delta(f_dir_sin[view.direction]);
	
	if (is_non_obstructed(new_x, new_y))
	{
		view.f_pos_x = new_x;
		view.f_pos_y = new_y;
	}
}

void pos_move_back(uchar direction)
{
	int new_x, new_y;
	
	new_x = view.f_pos_x - step_delta_from_dir_delta(f_dir_cos[view.direction]);
	new_y = view.f_pos_y - step_delta_from_dir_delta(f_dir_sin[view.direction]);
	
	if (is_non_obstructed(new_x, new_y))
	{
		view.f_pos_x = new_x;
		view.f_pos_y = new_y;
	}
}

/*
 * Causes the spans to redraw fully at next redraw occasion
 */
void spans_force_redraw()
{
	struct maze_vspan *p_span;
	int i;

	p_span = spans;
	for (i = 0; i < 32; i ++)
	{
		//p_span->prev_intensity = -1;
		p_span->prev_distidx = 1;
		//p_span->prev_y0 = 0;
		//p_span->prev_y1 = 191;
		//p_span->prev_height = 96;
		p_span++;
	}
}

// -- Sector-based recalc routines -------------------------------------------

void view_set_span_to_no_bound(struct maze_vspan *p_span)
{
	debug_printf("ERROR: No boundary found for direction %d\n", p_span->direction);
	p_span->distidx = 0x1FF;
//	p_span->y0 = 90;
//	p_span->y1 = 102;
//	p_span->intensity = 0;
}

#define BOUNDARY_OVERHANG	8

/*
 * Returns the distidx (distance index) between two points if
 * the second point has f_b difference in one coordinate and f_a_squared is a squared distance
 * in other coordinate. f_b can be negative.
 */
// int distidx_hypot_for_a2_b(long f_a_squared, int f_b)
// {
// 	return distidx_sqrt(f_sqr_approx(f_b) + f_a_squared);
// }

int __CALLEE__ distidx_hypot_for_a2_b(long f_a_squared, int f_b);

#asm
_distidx_hypot_for_a2_b:

	pop ix		; ret address
	pop hl		; f_b

        // f_sqr(i) f16_sqrs[distidx_from_f_dist(abs(i))]
	; abs
	bit 7,h
	jr z,_distidx_hypot_for_a2_b_pos

	; negate hl
	and a		; clear C
	ld	b,h
	ld	c,l
	ld	hl,0
	sbc	hl,bc

_distidx_hypot_for_a2_b_pos:	
	
        ; distidx from f_dist shifts 4 right, offset from index 2 left
	srl h
	rr l
	srl h
	rr l
	
	; round to 4
	ld a,l
	and a,0xFC
	ld l,a

	ld bc,_f16_sqrs
	add hl,bc
	
	; read long at hl
	ld c,(hl)
	inc hl
	ld b,(hl)
	inc hl
	ld e,(hl)
	inc hl
	ld d,(hl)

	; DE:BC now contains f_b^2
	
	pop hl		; f_a_squared LSB
	add hl,bc
	ld b,h
	ld c,l
	
	
	pop hl		; f_a_squared MSB
	adc hl,de	; add carry of previous add
	ex  de,hl       ; only need to move HL to DE

	; DE:BC now contains f_b^2+f_a_squared
	
	; __FASTCALL__ param in DE:HL
	ld h,b
	ld l,c
	
	push ix		; ret address

	jp _distidx_sqrt	; save a ret instruction (tail call)

#endasm

/*
 * OK, as a global state variable - not the cleanest way, but makes the code run in 55ms instead of 60ms
 * Window which is currently being calculated from p_span (including) to p_end_span (not including).
 * (Ab)used also in some functions that iterate over spans.
 */ 
static struct maze_vspan *p_span, *p_end_span;


void view_calc_edges(struct t_maze_sector *sector);

void calc_dist_horiz_edge_n(struct t_maze_sector *sector);

void calc_dist_horiz_edge_s(struct t_maze_sector *sector);

void calc_dist_vert_edge_e(struct t_maze_sector *sector);

void calc_dist_vert_edge_w(struct t_maze_sector *sector);

void calc_dist_horiz_edge_n(struct t_maze_sector *sector)
{
	int f_dy;
	int f_x1;
	long b;
	struct maze_vspan *p_end_span1;
	struct t_sector_boundary *p_bound;
	int f_dx;
	
	p_bound = sector->edge_n->boundaries;
	
	f_dy = p_bound->f_y0 - view.f_pos_y;
	f_dx = f_multiply(f_dir_ctan[p_span->direction], f_dy);
	
	if (f_dx >= (p_bound->f_x0 - BOUNDARY_OVERHANG - view.f_pos_x))
	{	
		b = f_sqr_approx(f_dy);
		
		while ((p_bound != NULL) && (p_span < p_end_span))
		{
			f_x1 = (p_bound->f_x1 + BOUNDARY_OVERHANG) - view.f_pos_x;
			if (p_bound->neighbour_boundary == NULL)
			{
				while (f_dx <= f_x1)
				{
					p_span->distidx = distidx_hypot_for_a2_b(b, f_dx);
					
					if (++p_span >= p_end_span)
					{
						break;
					}
					if (f_dir_sin[p_span->direction] >= 0)
					{
						break;
					}
					f_dx = f_multiply(f_dir_ctan[p_span->direction], f_dy);
				}
			}
			else // if ((p_bound->neighbour_boundary != NULL)
			{
				p_end_span1 = p_end_span;
				p_end_span = p_span;
				while (f_dx <= f_x1)
				{
					if (++p_end_span >= p_end_span1)
					{
						break;
					}
					if (f_dir_sin[p_end_span->direction] >= 0)
					{
						break;
					}
					f_dx = f_multiply(f_dir_ctan[p_end_span->direction], f_dy);
				}
				if (p_end_span != p_span)
				{
					view_calc_edges(p_bound->neighbour_sector);
				}
				p_end_span = p_end_span1;
			}
			p_bound = p_bound->next_in_edge;
		}
	}
	
	if ((p_span < p_end_span) && (f_dir_cos[p_span->direction] > 0))
	{
		calc_dist_vert_edge_e(sector);
	}
}	

void calc_dist_horiz_edge_s(struct t_maze_sector *sector)
{
	int f_dy;
	int f_x1;
	long b;
	struct maze_vspan *p_end_span1;
	struct t_sector_boundary *p_bound;
	int f_dx;
	
	p_bound = sector->edge_s->boundaries;
	
	f_dy = p_bound->f_y0 - view.f_pos_y;
	f_dx = f_multiply(f_dir_ctan[p_span->direction], f_dy);
	
	if (f_dx <= (p_bound->f_x0 + BOUNDARY_OVERHANG - view.f_pos_x))
	{	
		b = f_sqr_approx(f_dy);
	
		while ((p_bound != NULL) && (p_span < p_end_span))
		{
			f_x1 = p_bound->f_x1 - BOUNDARY_OVERHANG - view.f_pos_x;
			if (p_bound->neighbour_boundary == NULL)
			{
				while (f_dx >= f_x1)
				{
					p_span->distidx = distidx_hypot_for_a2_b(b, f_dx);
					if (++p_span >= p_end_span)
					{
						break;
					}
					if (f_dir_sin[p_span->direction] <= 0)
					{
						break;
					}
					f_dx = f_multiply(f_dir_ctan[p_span->direction], f_dy);
				}
			}
			else // if ((p_bound->neighbour_boundary != NULL)
			{
				p_end_span1 = p_end_span;
				p_end_span = p_span;
				while (f_dx >= f_x1)
				{
					if (++p_end_span >= p_end_span1)
					{
						break;
					}
					if (f_dir_sin[p_end_span->direction] <= 0)
					{
						break;
					}
					f_dx = f_multiply(f_dir_ctan[p_end_span->direction], f_dy);
				}
				if (p_span != p_end_span)
				{
					view_calc_edges(p_bound->neighbour_sector);
				}
				p_end_span = p_end_span1;
			}
			p_bound = p_bound->next_in_edge;
		}
	}
	
	if ((p_span < p_end_span) && (f_dir_cos[p_span->direction] < 0))
	{
		calc_dist_vert_edge_w(sector);
	}
}	

void calc_dist_vert_edge_e(struct t_maze_sector *sector)
{
	int f_dx;
	int f_y1;
	long a;
	struct maze_vspan *p_end_span1;
	struct t_sector_boundary *p_bound;
	int f_dy;
	
	p_bound = sector->edge_e->boundaries;
	
	f_dx = p_bound->f_x0 - view.f_pos_x;
	f_dy = f_multiply(f_dir_tan[p_span->direction], f_dx);
	
	if (f_dy >= (p_bound->f_y0 - BOUNDARY_OVERHANG - view.f_pos_y))
	{	
		a = f_sqr_approx(f_dx);
	
		while ((p_bound != NULL) && (p_span < p_end_span))
		{
			f_y1 = p_bound->f_y1 + BOUNDARY_OVERHANG - view.f_pos_y;
			if (p_bound->neighbour_boundary == NULL)
			{
				while (f_dy <= f_y1)
				{
					p_span->distidx = distidx_hypot_for_a2_b(a, f_dy);
					if (++p_span >= p_end_span)
					{
						break;
					}
					if (f_dir_cos[p_span->direction] <= 0)
					{
						break;
					}
					f_dy = f_multiply(f_dir_tan[p_span->direction], f_dx);
				}
			}
			else // if ((p_bound->neighbour_boundary != NULL)
			{
				p_end_span1 = p_end_span;
				p_end_span = p_span;
				while (f_dy <= f_y1)
				{
					if (++p_end_span >= p_end_span1)
					{
						break;
					}
					if (f_dir_cos[p_end_span->direction] <= 0)
					{
						break;
					}
					f_dy = f_multiply(f_dir_tan[p_end_span->direction], f_dx);
				}
				if (p_span != p_end_span)
				{
					view_calc_edges(p_bound->neighbour_sector);
				}
				p_end_span = p_end_span1;
			}
			p_bound = p_bound->next_in_edge;
		}
	}
	
	if ((p_span < p_end_span) && (f_dir_sin[p_span->direction] > 0))
	{
		calc_dist_horiz_edge_s(sector);
	}
}	

void calc_dist_vert_edge_w(struct t_maze_sector *sector)
{
	int f_dx;
	int f_y1;
	long a;
	struct maze_vspan *p_end_span1;
	struct t_sector_boundary *p_bound;
	int f_dy;
	
	p_bound = sector->edge_w->boundaries;
	
	f_dx = p_bound->f_x0 - view.f_pos_x;
	f_dy = f_multiply(f_dir_tan[p_span->direction], f_dx);
	
	if (f_dy <= (p_bound->f_y0 + BOUNDARY_OVERHANG - view.f_pos_y))
	{	
		a = f_sqr_approx(f_dx);
	
		while ((p_bound != NULL) && (p_span < p_end_span))
		// one way which worked to avoid the bug in top left corner (memory corruption as it seeems, 
		// which corrupts the next_in_boundary pointer); ultimately solved by reordering fields in struct maze_vspan
		// if (p_span < p_end_span)
		{
			f_y1 = p_bound->f_y1 - view.f_pos_y - BOUNDARY_OVERHANG;
			if (p_bound->neighbour_sector == NULL)
			{
				while (f_dy >= f_y1)
				{
					p_span->distidx = distidx_hypot_for_a2_b(a, f_dy);
					if (++p_span >= p_end_span)
					{
						break;
					}
					if (f_dir_cos[p_span->direction] >= 0)
					{
						break;
					}
					f_dy = f_multiply(f_dir_tan[p_span->direction], f_dx);
				}
			}
			else // if ((p_bound->neighbour_boundary != NULL)
			{
				// this got printed where it shouldn't have been (looking NW)
				// solved by  reordering fields in struct maze_vspan
				p_end_span1 = p_end_span;
				p_end_span = p_span;
				while (f_dy >= f_y1)
				{
					if (++p_end_span >= p_end_span1)
					{
						break;
					}
					if (f_dir_cos[p_end_span->direction] >= 0)
					{
						break;
					}
					f_dy = f_multiply(f_dir_tan[p_end_span->direction], f_dx);
				}
				if (p_span != p_end_span)
				{
					view_calc_edges(p_bound->neighbour_sector);
				}
				p_end_span = p_end_span1;
			}
			p_bound = p_bound->next_in_edge;
		}
	}

	if ((p_span < p_end_span) && (f_dir_sin[p_span->direction] < 0))
	{
		calc_dist_horiz_edge_n(sector);
	}
}	


/*
 * Computes the range of spans from p_start_span (including) to p_end_span (excluding).
 * 
 * Assumed to be called only in state where p_span < p_end_span
 *
 * NOTE: Returns the span after the last one computed. Due to numerical anomalies, it can happen that
 * when computing spans for adjacent sectors, the last one(s) get evaluated differently than in
 * the calling function. In this case, these spans are made a part of the next bound.
 */
void view_calc_edges(struct t_maze_sector *sector)
{
	// Where to begin? 
	// Then, each calc function continues along the wall
	if (f_dir_sin[p_span->direction] >= 0)
	{
		if (f_dir_cos[p_span->direction] >= 0)
		{
			calc_dist_vert_edge_e(sector);
		}
		else 
		{
			calc_dist_horiz_edge_s(sector);
		}
	}
	else 
	{
		if (f_dir_cos[p_span->direction] >= 0)
		{
			calc_dist_horiz_edge_n(sector);
		}
		else 
		{
			calc_dist_vert_edge_w(sector);
		}
	}
	
}

void view_calc_spans_sectors()
{
	struct t_maze_sector *sector;
	//struct maze_vspan *p_tspan; // use global variable - not nice, but improves speed
	
	// Update directions
	for (p_span = spans; p_span < &spans[32]; p_span++)
	{
		p_span->direction = (view.direction + p_span->n - 15) & N_PRECALC_MASK;
	}

	sector = get_sector_on_blocks(int_from_fixed(view.f_pos_x), int_from_fixed(view.f_pos_y));
	if (sector != NULL)
	{
	        p_span = &spans[0];
		p_end_span = &spans[32];
		view_calc_edges(sector);
	}
	else
	{
		debug_printf("Outside of the walls.\n");
	}
}

// -- View draw routines------------------------------------------------------



void draw_init()
{
	view_init_origin();
}

/*
 * Redraws the maze from the point of view of the current position and direction.
 * This routine should be tuned with worst case times in mind.
 */
void draw_maze()
{
//	struct maze_vspan *p_span; // use global variable - not nice, but improves speed
/*
	for (p_span = spans; p_span < &spans[32]; p_span++)
	{
		span_update(p_span);
	}
*/
	span_update(&spans[ 0]); span_update(&spans[ 1]); span_update(&spans[ 2]); span_update(&spans[ 3]);
	span_update(&spans[ 4]); span_update(&spans[ 5]); span_update(&spans[ 6]); span_update(&spans[ 7]);
	span_update(&spans[ 8]); span_update(&spans[ 9]); span_update(&spans[10]); span_update(&spans[11]);
	span_update(&spans[12]); span_update(&spans[13]); span_update(&spans[14]); span_update(&spans[15]);
	span_update(&spans[16]); span_update(&spans[17]); span_update(&spans[18]); span_update(&spans[19]);
	span_update(&spans[20]); span_update(&spans[21]); span_update(&spans[22]); span_update(&spans[23]);
	span_update(&spans[24]); span_update(&spans[25]); span_update(&spans[26]); span_update(&spans[27]);
	span_update(&spans[28]); span_update(&spans[29]); span_update(&spans[30]); span_update(&spans[31]);
}

void debug_draw_ray(struct maze_vspan *p_span)
{
	int f_x1, f_y1;
	int dir_ray;
	int f_dist;

//	timing_start();

	f_dist = f_dist_from_distidx(p_span->distidx);
	dir_ray = p_span->direction;

	f_x1 = view.f_pos_x + f_multiply(f_dist, f_dir_cos[dir_ray]);
	f_y1 = view.f_pos_y + f_multiply(f_dist, f_dir_sin[dir_ray]);
	
	draw_maze_line_fixed(view.f_pos_x, view.f_pos_y, f_x1, f_y1);

//	t_draw = timing_elapsed();
}

void debug_draw_directions()
{
#ifdef DEBUG_MAP_DRAW_ALL_DIRECTIONS
	int i;

	for (i = 0; i < 32; i ++)
	{
		debug_draw_ray(&spans[i]);
	}
#else	
	debug_draw_ray(&spans[0]);
	debug_draw_ray(&spans[31]);
#endif	
}

#ifdef DEBUG_DRAW_SECTORS_MAP

void sectors_draw_on_map()
{
	int i;
	struct t_sector_boundary *p_bound;
	
	for (i = 0; i < nsectors; i++)
	{
		draw_attr_colored_area(sectors[i].x0, sectors[i].y0, sectors[i].x1, sectors[i].y1, 0x07 | ((i & 7) << 3));
		
		p_bound = sectors[i].boundaries;
		while (p_bound != NULL)
		{
			sectors_draw_bound(p_bound);
			p_bound = p_bound->next;
		}
	}
}

#endif

void view_show_debug_screens()
{
#ifdef DEBUG_DRAW_SECTORS_MAP
	draw_maze_map();
	sectors_draw_on_map();

	in_WaitForKey();	
	in_WaitForNoKey();
#endif	
}

#ifndef NDEBUG

// Debug path walk routines

struct pathwalk_segment_t 
{
    int x, y;
};

/*
 * End is marked by x=0
 */
struct pathwalk_segment_t pathwalk[] = {
    { 0x200, 0x200 },
    { 0x600, 0x200 },
    { 0x180, 0x280 },
    { 0x200, 0x600 },
    { 0, 0 }
};

/*
 * Expected to be called only from view_debug_pathwalk().
 * Moves along the specified line. Does not stop exactly at end (to keep things reasonably fast)!
 * 
 * Returns the number of frames rendered.
 */ 
int view_debug_walk_line(struct pathwalk_segment_t *from, struct pathwalk_segment_t *to)
{
	int i, steps;
	int delta_x, delta_y;
	int step_x, step_y;
	
	delta_x = to->x - from->x;
	delta_y = to->y - from->y;

	// Does not actually move for MOVE_STEP, but this is faster to calculate
	if (abs(delta_x) > abs(delta_y))
	{
	    steps = abs(delta_x / MOVE_STEP);
	}
	else
	{
	    steps = abs(delta_y / MOVE_STEP);
	}
	
	step_x = delta_x / steps;
	step_y = delta_y / steps;
	
	view.f_pos_x = from->x;
	view.f_pos_y = from->y;
	view_calc_spans_sectors();
	draw_maze();
	
	for (i = 0; i < steps; i++)
	{
		view.f_pos_x += step_x;
		view.f_pos_y += step_y;
		view_calc_spans_sectors();
		draw_maze();
	}
	
	return steps + 1;
}

void view_debug_pathwalk()
{
	struct viewport view_save;
	struct pathwalk_segment_t *p_segment;
	int i, frames;
	long t;

	memcpy(&view_save, &view, sizeof(struct viewport));
	spans_force_redraw();
	
	p_segment = pathwalk;
	frames = 0;

	timing_start();
	do 
	{
		frames += view_debug_walk_line(p_segment, p_segment + 1);
		p_segment++;
	} 
	while (p_segment[1].x != 0);
	
	t = timing_elapsed();
	
	debug_printf("Time for path walk %ld ms, %d frames, %ld ms per frame, %ld dFPS\n", t, frames, (t/frames), (10000L * frames/t));
	debug_printf("Press any key to continue...\n");
	in_WaitForKey();	
	in_WaitForNoKey();

	spans_force_redraw();

	memcpy(&view, &view_save, sizeof(struct viewport));

}

void view_debug_pathwalk_draw_on_map()
{
	struct pathwalk_segment_t *p_segment;
	int x0, y0, x1, y1;

	p_segment = pathwalk;

	timing_start();
	do 
	{
		draw_maze_line_fixed(p_segment[0].x, p_segment[0].y, p_segment[1].x, p_segment[1].y);
		
		p_segment++;
	} 
	while (p_segment[1].x != 0);
}

#endif // NDEBUG

// -- Keyboard routines ------------------------------------------------------

// bit masks (one bit set in each)
#define CMD_LEFT	0x08
#define CMD_FORWARD	0x04
#define CMD_BACK	0x02
#define CMD_RIGHT	0x01

#define CMD_PATHWALK	0x80

//http://wordpress.animatez.co.uk/programming/assembly-language/z80/z80-library-routines/keyboard-asm
uchar keys_map[] = {
	0xFB, 0x02, CMD_FORWARD, // 'W' key, row 3 bit 2
	0xFD, 0x01, CMD_LEFT, // 'A' key, row 3 bit 1
	0xFD, 0x02, CMD_BACK, // 'S' key, row 3 bit 2
	0xFD, 0x04, CMD_RIGHT, // 'D' key, row 3 bit 2
	0xDF, 0x01, CMD_PATHWALK, // 'P' key, row 3 bit 1
	0
};

// bit masks (one bit set in each)
#define CMD_TOGGLE_MAP	0x01
#define CMD_TOGGLE_TEXT	0x02

uchar toggle_keys_map[] = {
	0x7F, 0x04,  // 'M' key, row 5 bit 3
	0xFB, 0x10,  // 'T' key, row 3 bit 5
	0
};


/*
 * Processes the movement commands that were enqueued while drawing the last frame.
 */
void view_process_cmds()
{
	uchar cmd;
	while ((cmd = cmds_get_next()) != 0)
	{
		if ((cmd & CMD_LEFT) != 0)
		{
			view.direction = (view.direction - 1) & N_PRECALC_MASK;
		}
		if ((cmd & CMD_RIGHT) != 0)
		{
			view.direction = (view.direction + 1) & N_PRECALC_MASK;
		}
		if ((cmd & CMD_FORWARD) != 0)
		{
			pos_move_forward(view.direction);
		}
		if ((cmd & CMD_BACK) != 0)
		{
			pos_move_back(view.direction);
		}
#ifndef NDEBUG		
		if ((cmd & CMD_PATHWALK) != 0)
		{
		    view_debug_pathwalk();
		    
		    // Remove commands generated in pathwalk
		    while ((cmd = cmds_get_next()) != 0)
		    {
		    }
		}
#endif		
	}
}

// -- Frames per second display routines -------------------------------------

#ifdef OPTION_DIGITAL_FPS

#define DRAW_CHAR(p_scr, c) draw_char((p_scr), (uchar *) (0x3C00 + 8 * (c)))

#define DRAW_DIGIT(p_scr, c) draw_char((p_scr), (uchar *) (0x3D80 + 8 * (c)))

void fps_draw_scale()
{
	memset(zx_cyx2aaddr(0, 28), COLOR_FPS | (COLOR_BACKGROUND << 3), 4);
}

void fps_draw_fps(int ftps)
{
	DRAW_DIGIT(((uchar *) (16384 + 31)), ftps % 10);
	ftps = ftps / 10;
	DRAW_DIGIT((((uchar *) 16384 + 29)), ftps % 10);
	ftps = ftps / 10;
	DRAW_DIGIT(((uchar *) (16384 + 28)), ftps % 10);
	DRAW_CHAR(((uchar *) (16384 + 30)), '.');
}

#else

int prev_ftps;

void fps_draw_scale()
{
	int i;
	uchar *p_attr;
	
	for (i = 0; i < 125; i++)
	{
		if ((i % 10) == 0)
		{
			plot(i * 2, 190);
		}
		plot(i * 2, 191);
	}
	
	p_attr = zx_cyx2aaddr(23, 0);
	memset(p_attr, COLOR_FPS | (COLOR_BACKGROUND << 3), 32);
}

/*
 * ftps is in tenth of frames per second; 10=1 FPS, 20=2 FPS and so on
 */
void fps_draw_fps(int ftps)
{
	int i;
	
	ftps = (ftps >= 125) ? 125 : ftps;
	
	for (i = 0; i < ftps; i++)
	{
		plot(i * 2, 188);
	}
	for (; i < prev_ftps + 1; i++)
	{
		unplot(i * 2, 188);
	}
	prev_ftps = ftps;
}

#endif

// -- Main program -----------------------------------------------------------

void view_do_game_loop()
{
    long start, end, t_calc, t_draw, t_draw_map, t_cmd;
	uint drawn_frames;
	long t_last_fps_upd, t_now;
	int full_redraw_trigger;
	struct t_cmd_toggle_snapshot cmd_t_timings, cmd_t_map;
	
	zx_border(COLOR_BACKGROUND);
	zx_colour(COLOR_WALLS | (COLOR_BACKGROUND << 3));
	// Blue ink, black paper
	printf("\020%d\021%d", COLOR_WALLS, COLOR_BACKGROUND);
	
	fps_draw_scale();
	drawn_frames = 0;
	t_cmd = 0;
	t_last_fps_upd = clock();
	full_redraw_trigger = 1;
	
	while (1)
	{
		if (cmd_toggle_snapshot_update(&cmd_t_timings, CMD_TOGGLE_TEXT))
		{
			full_redraw_trigger = 1;
		}
		if (cmd_toggle_snapshot_update(&cmd_t_map, CMD_TOGGLE_MAP))
		{
			full_redraw_trigger = 1;
		}
		
		if (full_redraw_trigger)
		{
			spans_force_redraw();
		}
	
		if (cmd_t_timings.state)
		{
			timing_start();
		}
		view_calc_spans_sectors();
		if (cmd_t_timings.state)
		{
			t_calc = timing_elapsed();
		}
	
		if (cmd_t_timings.state)
		{
			timing_start();
		}

		draw_maze();

		if (full_redraw_trigger)
		{
			fps_draw_scale();
			if (cmd_t_map.state)
			{
				draw_maze_map();
			}
			full_redraw_trigger = 0;
		}
		
		drawn_frames++;
		t_now = clock();

		if (cmd_t_timings.state)
		{
			t_draw = timing_elapsed();
			t_draw_map = 0;
		}
		
		if (cmd_t_map.state)
		{
			if (cmd_t_timings.state)
			{
				timing_start();
			}
			debug_draw_directions();

			if (cmd_t_timings.state)
			{
				t_draw_map = timing_elapsed();
			}
		}
		
		if (cmd_t_timings.state)
		{
 			printf("\026\066\040");
			printf("T[ms]: scene %ld (%ld /span), draw %ld, map %ld, cmd(prev frame) %ld ", t_calc, t_calc/32, t_draw, t_draw_map, t_cmd);
			// Move to (0, 0) for pritouts of errors
 			printf("\026\040\040");
			
			timing_start();
		}

		// Update FPS display every second (time included in cmd time to simplify printouts)
		if ((t_now - t_last_fps_upd) > 50L)
		{
			// In tenth of frames per second
			fps_draw_fps((drawn_frames * 500) / (t_now - t_last_fps_upd));
			
			drawn_frames = 0;
			t_last_fps_upd = t_now;
		}

		view_process_cmds();
		if (cmd_t_timings.state)
		{
			t_cmd = timing_elapsed();
		}
	}
}

// needed to use in_GetKey, see input.h
uchar in_KeyDebounce;        // Number of ticks before a keypress is acknowledged. Set to 1 for no debouncing.
uchar in_KeyStartRepeat;     // Number of ticks after first time key is registered (after debouncing) before a key starts repeating.
uchar in_KeyRepeatPeriod;    // Repeat key rate measured in ticks.
uint in_KbdState;            // Reserved variable holds in_GetKey() state

void main()
{
	// needed to use in_GetKey, see input.h
	in_GetKeyReset();

	timing_init();
	fixed_math_init();
	span_init();
	draw_init();
	sectors_init();
	
#ifdef NDEBUG
	cmd_toggle_set(CMD_TOGGLE_MAP, 0);
	cmd_toggle_set(CMD_TOGGLE_TEXT, 0);
#else	
	cmd_toggle_set(CMD_TOGGLE_MAP, 1);
	cmd_toggle_set(CMD_TOGGLE_TEXT, 1);
#endif

	// NOTE: installs ISR. This reduces the time available for main code and reduces the
	// accuracy of timing routines after this point
	cmd_init(keys_map, toggle_keys_map);

	view_init();
	
#ifndef NDEBUG

	debug_printf("Press any key to continue.");
	in_WaitForKey();	
	in_WaitForNoKey();
	
    clg();
	view_show_debug_screens();
	
#endif

    clg();
	view_do_game_loop();
}

