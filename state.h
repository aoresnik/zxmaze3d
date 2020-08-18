
#define CLIP_DISTANCE 0x0080

// Expected to be less than or equal to clip distance
#define MOVE_STEP 0x0010
#define MOVE_STEP_SHIFT 4

/**
 * Represents the viewpoint of the player: coordinates and direction.
 * It also encapsulates other precomputed data, that are dependent on them.
 */
struct state 
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

#ifndef _STATE_C_INCLUDED

/*
 * The state is a global variable rather than a pointer passed around. It is accessed
 * a lot and this program is working on a slow computer.
 */
extern struct state view;

extern void state_init();

#endif // defined _STATE_C_INCLUDED

