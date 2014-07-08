// This file is not automatically generated

#define N_DIRECTIONS 256

// f_dir_sin
extern int f_dir_sin[];
// f_dir_cos
extern int f_dir_cos[];
// ctan of direction, saturated
extern  int f_dir_ctan[];
// tan of direction, saturated
extern  int f_dir_tan[];

/*
 * Maximum distance that distance precalculated tables hold values for
 */
#define N_PRECALC_DRAW_DIST 512

#define N_PRECALC_MASK 0xFF

/*
 * Defines the unit of distance that is a resolution of the look-up tables that convert
 * from distance to other values.
 * 
 * By convention, the variables that hold the integer indexes into these tables 
 * are prefixed by "distidx_".
 * 
 * This limits distance because factor*distance must be less than 256
 */
#define N_PRECALC_FACTOR 16

// Must be N_PRECALC_DRAW_DIST/N_PRECALC_FACTOR
#define N_PRECALC_MAX_DIST 32

// Squares of distance, in 16-shifted fixed point, for fast sqrt
extern long f16_sqrs[];
// Heights of given distance index
extern uchar draw_heigths[];
extern uchar draw_heigths1[];
// Color intensities of given distance index
extern uchar draw_intens[];

