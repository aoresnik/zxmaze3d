
/*
 * Number of valid intensities for intensity param of draw_span()
 */
#define N_INTENSITIES 17

/*
 * Completely white intensity for draw_span()
 */
#define INTENSITY_WHITE 16

/*
 * Completely black intensity for draw_span()
 */
#define INTENSITY_BLACK 0

struct maze_vspan
{
    uchar direction;
    uchar n;
    uchar x;
    //uchar y0;
    //uchar y1;
    //uchar prev_y0;
    //uchar prev_y1;
    //uchar prev_height;
    uint prev_distidx;
    uint distidx;
    //uint f_distance;
    //uchar intensity;
    //uchar prev_intensity;
};

#ifndef _SPAN_C_INCLUDED

/*
 * Must be called before any other functions to initialize internal state
 */
extern void span_init();

/*
 * Paints the 8-pixels wide vertical span in column cx (0..32), with Bayer halftone pattern
 * of intensity (but could in principle draw any other 8x8 pattern), from line y0 (0..191)
 * to line y1 (0..191).
 *
 * Expects y1 > y0.
 *
 * Fills the specified span with halftone 4x4 pattern of intensity
 * (supported levels are from INTENSITY_BLACK to INTENSITY_WHITE).
 */
extern void draw_span(uchar cx, uchar y0, uchar y1, uchar intensity);

extern void draw_blocks(uchar *p_scr, uchar y0, uchar y1, uchar *p_pat);

/*
 * Updates the span at new heigth (assumes it has been drawn with p_span->prev_distidx)
 */
extern void __FASTCALL__ span_update(struct maze_vspan *p_span);

/*
 * Bitmap of all N_INTENSITIES intensities from black to white, 8-byte each, 8x8 pixels
 */
extern uchar ht_bits[];

#endif // defined _SPAN_C_INCLUDED
