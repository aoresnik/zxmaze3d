
#include <stdio.h>
#include <input.h>
#include <stdlib.h>
#include <graphics.h>
#include <games.h>
#include <sound.h>
#include <im2.h>
#include <conio.h> 
#include <string.h>

#include "timing.h"

#include "span.h"

struct t_span {
	uchar x, y0, y1;
	uchar intensity;
};



#define N_TEST_SPANS (32+N_INTENSITIES)

void draw_span_test_patterns()
{
	int i;
	uchar j;
	uchar x0, x1, x2;
	uchar y0, y1;
	int center;
    long duration;
	struct t_span spans[N_TEST_SPANS];
	struct t_span *p_span, *p_span_end;
	int bytes;
	int kbps;
	
	center = 17;
	p_span = spans;
	
	for (i = 0; i <= N_INTENSITIES-1; i++)
	{
		if (center + i < 32)
		{
			p_span->x = center + i;
			p_span->y0 = 67 + i;
			p_span->y1 = 176 - (i*3/2);
			p_span->intensity = N_INTENSITIES - 1 - i;
			p_span++;
		}
		if (center - i >= 0)
		{
			p_span->x = center - i;
			p_span->y0 = 67 + i;
			p_span->y1 = 176 - (i*3/2);
			p_span->intensity = N_INTENSITIES - 1 - i;
			p_span++;
		}
	}
	for (i = 0; i < N_INTENSITIES; i++)
	{
		p_span->x = 31 - i;
		p_span->y0 = 47-i;
		p_span->y1 = 48;
		p_span->intensity = i;
		
		p_span++;
	}
	p_span_end = p_span;

	// Count the number of bytes in order to calculate fill rate
	bytes = 0;
	for (p_span = spans; p_span != p_span_end; p_span++)
	{
		bytes += p_span->y1 - p_span->y0;
	}

	timing_start();
	for (p_span = spans; p_span != p_span_end; p_span++)
	{
		draw_span(p_span->x, p_span->y0, p_span->y1, p_span->intensity);
	}
	duration = timing_elapsed_us();
	
	kbps = 1000L * bytes / duration;
	
	printf("Drawn span test patterns - %d bytes in %ld us, %d kB/s\n", bytes, duration, kbps);
}

// Limit to not cross 8 boundaries
void draw_span_test_patterns2()
{
	int i;
	uchar j;
	uchar x0, x1, x2;
	uchar y0, y1;
	int center;
    long duration;
	struct t_span spans[N_TEST_SPANS];
	struct t_span *p_span, *p_span_end;
	int bytes;
	int kbps;
	
	center = 17;
	p_span = spans;
	
	for (i = 0; i <= N_INTENSITIES-1; i++)
	{
		if (center + i < 32)
		{
			p_span->x = center + i;
			//p_span->y0 = 67 + i;
			//p_span->y1 = 176 - (i*3/2);
			p_span->y0 = 73;
			p_span->y1 = 118;
			p_span->intensity = N_INTENSITIES - 1 - i;
			p_span++;
		}
		if (center - i >= 0)
		{
			p_span->x = center - i;
			//p_span->y0 = 67 + i;
			//p_span->y1 = 176 - (i*3/2);
			p_span->y0 = 73;
			p_span->y1 = 118;
			p_span->intensity = N_INTENSITIES - 1 - i;
			p_span++;
		}
	}
	for (i = 0; i < N_INTENSITIES; i++)
	{
		p_span->x = 31 - i;
		p_span->y0 = 47-i;
		p_span->y1 = 48;
		p_span->intensity = i;
		
		p_span++;
	}
	p_span_end = p_span;

	// Count the number of bytes in order to calculate fill rate
	bytes = 0;
	for (p_span = spans; p_span != p_span_end; p_span++)
	{
		bytes += p_span->y1 - p_span->y0;
	}

	timing_start();
	for (p_span = spans; p_span != p_span_end; p_span++)
	{
		draw_span(p_span->x, p_span->y0, p_span->y1, p_span->intensity);
	}
	duration = timing_elapsed();
	
	kbps = bytes / duration;
	
	printf("Drawn span test patterns - %d bytes in %ld ms, %d kB/s\n", bytes, duration, kbps);
}

// Directly call span draw function
void draw_span_test_patterns3()
{
    long duration;
	int bytes;
	int kbps;
	
	bytes = 8 * 32 * 24;

	timing_start();
	draw_blocks(16384, 0, 192, &ht_bits[1 * 8]);
	draw_blocks(16385, 0, 192, &ht_bits[2 * 8]);
	draw_blocks(16386, 0, 192, &ht_bits[3 * 8]);
	draw_blocks(16387, 0, 192, &ht_bits[4 * 8]);
	draw_blocks(16388, 0, 192, &ht_bits[5 * 8]);
	draw_blocks(16389, 0, 192, &ht_bits[6 * 8]);
	draw_blocks(16390, 0, 192, &ht_bits[7 * 8]);
	draw_blocks(16391, 0, 192, &ht_bits[8 * 8]);
	draw_blocks(16392, 0, 192, &ht_bits[9 * 8]);
	draw_blocks(16393, 0, 192, &ht_bits[10 * 8]);
	draw_blocks(16394, 0, 192, &ht_bits[11 * 8]);
	draw_blocks(16395, 0, 192, &ht_bits[12 * 8]);
	draw_blocks(16396, 0, 192, &ht_bits[13 * 8]);
	draw_blocks(16397, 0, 192, &ht_bits[14 * 8]);
	draw_blocks(16398, 0, 192, &ht_bits[15 * 8]);
	draw_blocks(16399, 0, 192, &ht_bits[16 * 8]);
	draw_blocks(16400, 0, 192, &ht_bits[15 * 8]);
	draw_blocks(16401, 0, 192, &ht_bits[14 * 8]);
	draw_blocks(16402, 0, 192, &ht_bits[13 * 8]);
	draw_blocks(16403, 0, 192, &ht_bits[12 * 8]);
	draw_blocks(16404, 0, 192, &ht_bits[11 * 8]);
	draw_blocks(16405, 0, 192, &ht_bits[10 * 8]);
	draw_blocks(16406, 0, 192, &ht_bits[9 * 8]);
	draw_blocks(16407, 0, 192, &ht_bits[8 * 8]);
	draw_blocks(16408, 0, 192, &ht_bits[7 * 8]);
	draw_blocks(16409, 0, 192, &ht_bits[6 * 8]);
	draw_blocks(16410, 0, 192, &ht_bits[5 * 8]);
	draw_blocks(16411, 0, 192, &ht_bits[4 * 8]);
	draw_blocks(16412, 0, 192, &ht_bits[3 * 8]);
	draw_blocks(16413, 0, 192, &ht_bits[2 * 8]);
	draw_blocks(16414, 0, 192, &ht_bits[1 * 8]);
	draw_blocks(16415, 0, 192, &ht_bits[0 * 8]);
	duration = timing_elapsed();
	
	kbps = bytes / duration;
	
	printf("Drawn span test patterns - %d bytes in %ld ms, %d kB/s\n", bytes, duration, kbps);
}

void draw_span_test_animation2()
{
	int i;
	int shift;
	int nshown;
	int fps;
	long time, prev_time;
	int pos;
	int intensity;
	void *patterns[64];
	
	shift = 0;
	nshown = 0;
	
	// Scale for FPS meter
	for (i = 0; i < 50; i++)
	{
		plot(i * 2, 190);
		
		// every 10 ticks
		if (i % 10 == 0)
		{
			plot(i * 2, 191);
		}
	}

	for (i = 0; i < 32; i++)
	{
		pos = (i + shift) % ((N_INTENSITIES*2)-2);
		intensity = (pos < N_INTENSITIES) ? pos : (((N_INTENSITIES*2)-2) - pos);
		patterns[i] = patterns[i + 32] = &ht_bits[intensity * 8];
	}

	prev_time = clock();
	while (in_Inkey() == 0)
	{
		draw_blocks(16384, 0, 184, patterns[0 + shift]);
		draw_blocks(16385, 0, 184, patterns[1 + shift]);
		draw_blocks(16386, 0, 184, patterns[2 + shift]);
		draw_blocks(16387, 0, 184, patterns[3 + shift]);
		draw_blocks(16388, 0, 184, patterns[4 + shift]);
		draw_blocks(16389, 0, 184, patterns[5 + shift]);
		draw_blocks(16390, 0, 184, patterns[6 + shift]);
		draw_blocks(16391, 0, 184, patterns[7 + shift]);
		draw_blocks(16392, 0, 184, patterns[8 + shift]);
		draw_blocks(16393, 0, 184, patterns[9 + shift]);
		draw_blocks(16394, 0, 184, patterns[10 + shift]);
		draw_blocks(16395, 0, 184, patterns[11 + shift]);
		draw_blocks(16396, 0, 184, patterns[12 + shift]);
		draw_blocks(16397, 0, 184, patterns[13 + shift]);
		draw_blocks(16398, 0, 184, patterns[14 + shift]);
		draw_blocks(16399, 0, 184, patterns[15 + shift]);
		draw_blocks(16400, 0, 184, patterns[16 + shift]);
		draw_blocks(16401, 0, 184, patterns[17 + shift]);
		draw_blocks(16402, 0, 184, patterns[18 + shift]);
		draw_blocks(16403, 0, 184, patterns[19 + shift]);
		draw_blocks(16404, 0, 184, patterns[20 + shift]);
		draw_blocks(16405, 0, 184, patterns[21 + shift]);
		draw_blocks(16406, 0, 184, patterns[22 + shift]);
		draw_blocks(16407, 0, 184, patterns[23 + shift]);
		draw_blocks(16408, 0, 184, patterns[24 + shift]);
		draw_blocks(16409, 0, 184, patterns[25 + shift]);
		draw_blocks(16410, 0, 184, patterns[26 + shift]);
		draw_blocks(16411, 0, 184, patterns[27 + shift]);
		draw_blocks(16412, 0, 184, patterns[28 + shift]);
		draw_blocks(16413, 0, 184, patterns[29 + shift]);
		draw_blocks(16414, 0, 184, patterns[30 + shift]);
		draw_blocks(16415, 0, 184, patterns[31 + shift]);
		
		shift = (shift + 1) % 32;
		nshown++;
		if (nshown == 10)
		{
			time = clock();
			fps = (nshown * 50) / (time-prev_time);
			prev_time = time;
			
			// FPS meter
			for (i = 0; i <= fps; i++)
			{
				plot(i * 2, 189);
			}
			for (i = fps+1; i <= 50; i++)
			{
				unplot(i * 2, 189);
			}
			nshown = 0;
		}
	}
	printf("Last FPS: %d", fps);
}

void draw_span_test_animation()
{
	int i;
	int shift;
	int nshown;
	int fps;
	long time, prev_time;
	int pos;
	
	shift = 0;
	nshown = 0;
	
	// Scale for FPS meter
	for (i = 0; i < 50; i++)
	{
		plot(i * 2, 172);
		
		// every 10 ticks
		if (i % 10 == 0)
		{
			plot(i * 2, 173);
		}
	}
				
	prev_time = clock();
	while (in_Inkey() == 0)
	{
		for (i = 0; i < 32; i++)
		{
			pos = (i + shift) % ((N_INTENSITIES*2)-2);
			draw_span(i, 20, 150-(i/2), (pos < N_INTENSITIES) ? pos : (((N_INTENSITIES*2)-2) - pos));
		}
		shift++;
		nshown++;
		if (nshown == 10)
		{
			time = clock();
			fps = (nshown * 50) / (time-prev_time);
			prev_time = time;
			
			// FPS meter
			for (i = 0; i <= fps; i++)
			{
				plot(i * 2, 170);
			}
			for (i = fps+1; i <= 50; i++)
			{
				unplot(i * 2, 170);
			}
			nshown = 0;
		}
	}
	printf("Last FPS: %d", fps);
}

main()
{
    long duration;
	int kbps;
	
    clg();

	span_init();
	timing_init();

	timing_start();
    clg();
	duration = timing_elapsed();
	
	// 6144 is the number of bytes of screen
	// ms and kb cancel out: (6144  / 1000) / (duration / 1000)
	kbps = 6144  / (int) duration;
	
	printf("clg() in %ld ms; fill rate %d kB/s\n", duration, kbps);
	
//	draw_precompiled_span_test_patterns();

	draw_span_test_patterns();

	in_WaitForKey();	
	in_WaitForNoKey();
	
	draw_span_test_patterns3();

	
	
	in_WaitForKey();	
	in_WaitForNoKey();
	

    clg();

	draw_span_test_animation();

}

