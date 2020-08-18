
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <input.h>
#include <graphics.h>
#include <spectrum.h>

#define _RENDER_C_INCLUDED

#include "state.h"
#include "fixed-math.h"
#include "tables.h"
#include "span.h"
#include "map.h"
#include "render.h"
#include "timing.h"
#include "common.h"

// If enabled, a colored map of sectors is drawn after init
//#define DEBUG_DRAW_SECTORS_MAP

struct maze_vspan spans[32];

// -- Maze sector routines ---------------------------------------------------

#define BOUND_TYPE_HORIZONTAL    0
#define BOUND_TYPE_VERTICAL    1

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
struct t_maze_sector *sectors_on_blocks[MAP_SIZE];

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
                p_bound->neighbour_boundary    = NULL;
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
//    p_span->y0 = 90;
//    p_span->y1 = 102;
//    p_span->intensity = 0;
}

#define BOUNDARY_OVERHANG    8

/*
 * Returns the distidx (distance index) between two points if
 * the second point has f_b difference in one coordinate and f_a_squared is a squared distance
 * in other coordinate. f_b can be negative.
 */
// int distidx_hypot_for_a2_b(long f_a_squared, int f_b)
// {
//     return distidx_sqrt(f_sqr_approx(f_b) + f_a_squared);
// }

int __CALLEE__ distidx_hypot_for_a2_b(long f_a_squared, int f_b);

#asm
_distidx_hypot_for_a2_b:

    pop ix        ; ret address
    pop hl        ; f_b

        // f_sqr(i) f16_sqrs[distidx_from_f_dist(abs(i))]
    ; abs
    bit 7,h
    jr z,_distidx_hypot_for_a2_b_pos

    ; negate hl
    and a        ; clear C
    ld    b,h
    ld    c,l
    ld    hl,0
    sbc    hl,bc

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
    
    pop hl        ; f_a_squared LSB
    add hl,bc
    ld b,h
    ld c,l
    
    
    pop hl        ; f_a_squared MSB
    adc hl,de    ; add carry of previous add
    ex  de,hl       ; only need to move HL to DE

    ; DE:BC now contains f_b^2+f_a_squared
    
    ; __FASTCALL__ param in DE:HL
    ld h,b
    ld l,c
    
    push ix        ; ret address

    jp _distidx_sqrt    ; save a ret instruction (tail call)

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
    f_dx = f_multiply(f_dy, f_dir_ctan[p_span->direction]);
    
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
                    f_dx = f_multiply(f_dy, f_dir_ctan[p_span->direction]);
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
                    f_dx = f_multiply(f_dy, f_dir_ctan[p_end_span->direction]);
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
    f_dx = f_multiply(f_dy, f_dir_ctan[p_span->direction]);
    
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
                    f_dx = f_multiply(f_dy, f_dir_ctan[p_span->direction]);
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
                    f_dx = f_multiply(f_dy, f_dir_ctan[p_end_span->direction]);
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
    f_dy = f_multiply(f_dx, f_dir_tan[p_span->direction]);
    
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
                    f_dy = f_multiply(f_dx, f_dir_tan[p_span->direction]);
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
                    f_dy = f_multiply(f_dx, f_dir_tan[p_end_span->direction]);
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
    f_dy = f_multiply(f_dx, f_dir_tan[p_span->direction]);
    
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
                    f_dy = f_multiply(f_dx, f_dir_tan[p_span->direction]);
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
                    f_dy = f_multiply(f_dx, f_dir_tan[p_end_span->direction]);
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

void render_show_debug_screens()
{
#ifdef DEBUG_DRAW_SECTORS_MAP
    render_update_map();
    sectors_draw_on_map();

    in_WaitForKey();    
    in_WaitForNoKey();
#endif    
}

// -- View draw routines------------------------------------------------------

/*
 * Redraws the maze from the point of view of the current position and direction.
 * This routine should be tuned with worst case times in mind.
 */
void render_update()
{
//    struct maze_vspan *p_span; // use global variable - not nice, but improves speed
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

void render_init()
{
    state_init_origin();
}
