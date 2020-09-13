
#include <stdio.h>

#include <conio.h>
#include <graphics.h>
#include <games.h>
#include <sound.h>
#include <input.h>

#include "timing.h"
#include "fixed-math.h"
#include "tables.h"

int f_multiply_reference(int f_a, int f_b)
{
    long tmp;
    int expected;

    tmp = ((long) f_a * f_b) >> 8;
    if (tmp > 0x7FFFL)
    {
        expected = 0x7FFF;
    }
    else if (tmp < -0x8000L)
    {
        expected = -0x8000;
    }
    else
    {
        expected = tmp;
    }
    return expected;
}

void check_f_multiply(int f_a, int f_b)
{
    int expected, res;
    long t;

    expected = f_multiply_reference(f_a, f_b);
    timing_start();
    res = f_multiply(f_a, f_b);
    t = timing_elapsed_us();
    printf(" (%d, %d)", f_a, f_b);
    if (res == expected)
    {
        printf("=%d OK", res);
    }
    else
    {
        printf(" ERROR: result: %d expected: %d", res, expected);
    }
    printf(" %ldus |", t);
}

void print_cell_hex(int color_text, int val)
{
    if (val >= 0)
    {
      //printf("\020%d\021%d", COLOR_WALLS, COLOR_BACKGROUND)
        printf("\020%d +%4x\020%d", color_text, val, 0);
    }
    else
    {
        printf("\020%d -%4x\020%d", color_text, (-val), 0);
    }
}

void print_cell_hex_bg(int color_bg, int color_text, int val)
{
    if (val >= 0)
    {
      //printf("\020%d\021%d", COLOR_WALLS, COLOR_BACKGROUND)
        printf("\021%d\020%d +%4x\021%d\020%d", color_bg, color_text, val, INK_WHITE, INK_BLACK);
    }
    else
    {
        printf("\021%d\020%d -%4x\021%d\020%d", color_bg, color_text, (-val), INK_WHITE, INK_BLACK);
    }
}

void print_cell_empty()
{
    printf("      ");
}

void print_cell_text(char *text)
{
    printf("%6s", text);
}

void check_f_multiply_simple(int f_a, int f_b, int type)
{
    int expected, res;
    uchar color_text;
    long tmp;
    long t;

    tmp = ((long) f_a * f_b) >> 8;
    if (tmp > 0x7FFFL)
    {
        expected = 0x7FFFL;
    }
    else if (tmp < -0x8000L)
    {
        expected = -0x8000L;
    }
    else
    {
        expected = tmp;
    }
    if (type == 1)
    {
        timing_start();
    }

    res = f_multiply(f_a, f_b);

    if (type == 1)
    {
        t = timing_elapsed_us();
    }
    if (type == 0)
    {
        if (res == expected)
        {
            color_text = INK_GREEN;
        }
        else
        {
            color_text = INK_RED;

        }
        print_cell_hex(color_text, res);
    }
    if (type == 1)
    {
         printf(" %5ld", t);
    }
}

void force_include_full_printf()
{
    printf("%f", 0.0);
}

int test_args[] = {0, 256, -256, 0x7FFF, -0x8000, 0x0800, -0x0800, 2, -2};

#define TEST_ARGS_SIZE ((sizeof(test_args))/ sizeof(int))

void test_f_multiply_1(int test_type)
{
    int i, j;

      print_cell_text("\\");
      print_cell_text("arg2");

      switch (test_type)
      {
        case 0:
          printf("  test results and correctnes (green OK, red wrong)");
          break;
        case 1:
          printf("  execution time in microseconds");
          break;
      }
      printf("\n");

      print_cell_text("arg1");

      for (j = 0; j < TEST_ARGS_SIZE ; j++)
      {
        print_cell_hex_bg(INK_BLACK, INK_WHITE, test_args[j]);
      }
      printf("\n");
    for (i = 0; i < TEST_ARGS_SIZE ; i++)
    {
        print_cell_hex_bg(INK_BLACK, INK_WHITE, test_args[i]);
      for (j = 0; j < TEST_ARGS_SIZE ; j++)
      {
        check_f_multiply_simple(test_args[i], test_args[j], test_type);
      }
      printf("\n");
    }

}

void test_distidx_sqrt()
{
    int i;
    unsigned long param;
    uint res;
    uint expected;
    long t;


    printf("Testing distidx_sqrt ... ");

    for (i = 0; i < 10; i++)
    {
        expected = i;
        param = f16_sqrs[i];
        timing_start();
        res = distidx_sqrt(param);
        t = timing_elapsed_us();
        printf("(%ld) = %d, ex. %d; %ldus |", param, res, expected, t);
    }

    printf("\n");
}

uint distidx_sqrt_timed(unsigned long f16_l)
{
    uint result;
    long t;

    timing_start();

    result = distidx_sqrt(f16_l);

    t = timing_elapsed_us();
    printf(" t_sqrt=%ld us |", t);

    return result;
}


main()
{
    timing_init();

    test_f_multiply_1(0);
    test_f_multiply_1(1);

    printf("Press any key to continue...\n");

    in_WaitForKey();
    in_WaitForNoKey();

    test_distidx_sqrt();
}
