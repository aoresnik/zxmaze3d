
// The code is written with these dimensions in mind. It could work with max 16x16 maze.

#define MAZE_X 16
#define MAZE_Y 8

// Must be MAP_X*MAP_Y
#define MAP_SIZE 128

#ifndef _MAP_C_INCLUDED

extern uchar is_filled(uchar x, uchar y);

#define is_wall(x0, y0, x1, y1) (is_filled(x0, y0) != is_filled(x1, y1))

// delta can be negative
extern uchar is_wall_delta(uchar x, uchar y, int delta);

// Is there a wall on the west of the cell?
#define is_wall_w(x0, y0) (is_wall_delta(x0, y0, -1))
// Is there a wall on the north of the cell
#define is_wall_n(x0, y0) (is_wall_delta(x0, y0, -16))

extern int map_x(int maze_x);

extern int map_y(int maze_y);

extern int map_fx(int maze_f_x);

extern int map_fy(int maze_f_y);

extern uchar maze_in_boundary_f(int f_x, int f_y);

extern void map_init(uchar *map);

#endif // defined _MAP_C_INCLUDED
