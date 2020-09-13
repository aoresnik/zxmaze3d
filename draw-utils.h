
#ifndef _DRAW_C_INCLUDED

extern uchar __CALLEE__ *draw_char(uchar *p_scr, uchar *p_pat);

extern void draw_gray_area(uchar x0, uchar y0, uchar x1, uchar y1);

// Note: this is not the cleanest - uses map (move there)
extern void draw_map_line_fixed(int maze_x0, int maze_y0, int maze_x1, int maze_y1);

extern void draw_stipled_block_horiz(uchar x0, uchar x1, uchar y);

extern void draw_stipled_block_vert(uchar x, uchar y0, uchar y1);

extern void draw_attr_colored_area(uchar x0, uchar y0, uchar x1, uchar y1, uchar attr);

extern void draw_init();

#endif // defined _DRAW_C_INCLUDED
