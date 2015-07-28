// Memory to reserve for sectors (at some point it s printed how much is used)
#define MAX_SECTORS 10

// Memory to reserve for boundaries (at some point it s printed how much is used)
#define MAX_BOUNDARIES 40


#ifndef _RENDER_C_INCLUDED

extern struct maze_vspan spans[32];

void render_show_debug_screens();

extern void render_update();

#endif // defined _RENDER_C_INCLUDED

