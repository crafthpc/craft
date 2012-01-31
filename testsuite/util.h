#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define PRINT_OUTPUT 1
#define PRODUCE_X87_DUMPS 1

#define ABSF(x) (((x) > 0.0f) ? (x) : (-(x)))
#define ABS(x) (((x) > 0.0) ? (x) : (-(x)))
//#define ABSL(x) (((x) > 0.0L) ? (x) : (-(x)))
#define MAX(x,y) ((x) > (y) ? (x) : (y))

#define FINF (1.0f/0.0f)
#define DINF (1.0/0.0)
//#define LDINF (1.0L/0.0L)

#define INT_SIZE sizeof(unsigned)

int f_approx_equal(float x, float y);
int d_approx_equal(double x, double y);

//int ld_approx_equal(long double x, long double y);

