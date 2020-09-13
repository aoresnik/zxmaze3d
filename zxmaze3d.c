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
#include "draw-utils.h"
#include "map.h"
#include "cmd.h"
#include "state.h"
#include "timing.h"
#include "span.h"
#include "render.h"
#include "fixed-math.h"

#include "tables.h"

// Enables measuring and printing the times of various stages of rendering
#define DEBUG_FRAME_TIMES

// Enables drawing all directions on map (otherwise only first and last)
//#define DEBUG_MAP_DRAW_ALL_DIRECTIONS

// Digital frames per second display (faster; if disabled, analog display is shown)
#define OPTION_DIGITAL_FPS

// Enables the pathwalk for timing when P is pressed
#define ENABLE_TIMING_PATHWALK

#define COLOR_WALLS    INK_BLUE
#define COLOR_BACKGROUND INK_BLACK
#define COLOR_FPS    INK_CYAN

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

#ifdef ENABLE_TIMING_PATHWALK
void view_debug_pathwalk_draw_on_map();
#endif

void render_update_map()
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
#ifdef ENABLE_TIMING_PATHWALK
    view_debug_pathwalk_draw_on_map();
#endif
}

void debug_draw_ray(struct maze_vspan *p_span)
{
    int f_x1, f_y1;
    int dir_ray;
    int f_dist;

//    timing_start();

    f_dist = f_dist_from_distidx(p_span->distidx);
    dir_ray = p_span->direction;

    f_x1 = view.f_pos_x + f_multiply(f_dist, f_dir_cos[dir_ray]);
    f_y1 = view.f_pos_y + f_multiply(f_dist, f_dir_sin[dir_ray]);

    draw_map_line_fixed(view.f_pos_x, view.f_pos_y, f_x1, f_y1);

//    t_draw = timing_elapsed();
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

#ifdef ENABLE_TIMING_PATHWALK

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
    render_update();

    for (i = 0; i < steps; i++)
    {
        view.f_pos_x += step_x;
        view.f_pos_y += step_y;
        view_calc_spans_sectors();
        render_update();
    }

    return steps + 1;
}

void view_debug_pathwalk()
{
    struct state view_save;
    struct pathwalk_segment_t *p_segment;
    int i, frames;
    long t;

    memcpy(&view_save, &view, sizeof(struct state));
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

    printf("\026\040\040");
    printf("Time for path walk %ld ms, %d frames, %ld ms per frame, %ld dFPS\n", t, frames, (t/frames), (10000L * frames/t));
    printf("Press any key to continue...\n");
    in_WaitForKey();
    in_WaitForNoKey();

    spans_force_redraw();

    memcpy(&view, &view_save, sizeof(struct state));

}

void view_debug_pathwalk_draw_on_map()
{
    struct pathwalk_segment_t *p_segment;
    int x0, y0, x1, y1;

    p_segment = pathwalk;

    timing_start();
    do
    {
        draw_map_line_fixed(p_segment[0].x, p_segment[0].y, p_segment[1].x, p_segment[1].y);

        p_segment++;
    }
    while (p_segment[1].x != 0);
}

#endif // ENABLE_TIMING_PATHWALK

// -- Keyboard routines ------------------------------------------------------

// bit masks (one bit set in each)
#define CMD_LEFT    0x08
#define CMD_FORWARD    0x04
#define CMD_BACK    0x02
#define CMD_RIGHT    0x01

#define CMD_PATHWALK    0x80

//http://wordpress.animatez.co.uk/programming/assembly-language/z80/z80-library-routines/keyboard-asm
uchar keys_map[] = {
    0xFB, 0x02, CMD_FORWARD, // 'W' key, row 3 bit 2
    0xFD, 0x01, CMD_LEFT, // 'A' key, row 3 bit 1
    0xFD, 0x02, CMD_BACK, // 'S' key, row 3 bit 2
    0xFD, 0x04, CMD_RIGHT, // 'D' key, row 3 bit 2
#ifdef ENABLE_TIMING_PATHWALK
    0xDF, 0x01, CMD_PATHWALK, // 'P' key, row 3 bit 1
#endif
    0
};

// bit masks (one bit set in each)
#define CMD_TOGGLE_MAP    0x01
#define CMD_TOGGLE_TEXT    0x02

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
#ifdef ENABLE_TIMING_PATHWALK
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

        render_update();

        if (full_redraw_trigger)
        {
            fps_draw_scale();
            if (cmd_t_map.state)
            {
                render_update_map();
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
    clg();
    puts("ZXMAZE3D http://aoresnik.github.io/zxmaze3d/");

    // needed to use in_GetKey, see input.h
    in_GetKeyReset();

    timing_init();
    fixed_math_init();
    span_init();
    draw_init();
    map_init(maze);
    state_init();
    render_init();
    sectors_init();

    cmd_toggle_set(CMD_TOGGLE_MAP, 0);
    cmd_toggle_set(CMD_TOGGLE_TEXT, 0);

    // NOTE: installs ISR. This reduces the time available for main code and reduces the
    // accuracy of timing routines after this point
    cmd_init(keys_map, toggle_keys_map);

    view_init();

#ifndef NDEBUG

    debug_printf("Press any key to continue.");
    in_WaitForKey();
    in_WaitForNoKey();

    clg();
    render_show_debug_screens();

#endif

    clg();
    view_do_game_loop();
}
