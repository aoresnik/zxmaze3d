
#include <stdlib.h>
#include <string.h>

#define _STATE_C_INCLUDED

#include "state.h"
#include "common.h"

struct state view;

void state_init_origin()
{
    view.f_pos_x = 0x200;
    view.f_pos_y = 0x200;
    view.direction = 0;
}

void state_init()
{
    state_init_origin();
}
