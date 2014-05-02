/**
 * sum2pi_x.c
 *
 * CRAFT demo app. Calculates pi*x in a computationally-heavy way that
 * demonstrates how to use CRAFT without being too complicated.
 *
 * September 2013
 */

#include <stdio.h>
#include <stdlib.h>

/* macros */
#define ABS(x) ( ((x) < 0.0) ? (-(x)) : (x) )

/* constants */
#define PI     3.14159265359
#define EPS    1e-7

/* loop  iterations; OUTER is X */
#define INNER    30
#define OUTER    2000

/* 'real' is double-precision if not pre-defined */
#ifndef real
#define real double
#endif

/* sum type is the same as 'real' if not pre-defined */
#ifndef sum_type
#define sum_type real
#endif

int sum2pi_x() {

    int i, j, k;
    real x, y, z;
    real err;
    real acc;
    sum_type sum;

    real final = (real)OUTER * PI;  /* correct answer */

    printf("===  Sum2PI_X  ===\n");
    printf("sizeof(real)=%lu\n",     sizeof(real));
    printf("sizeof(sum_type)=%lu\n", sizeof(sum_type));

    sum = 0.0;
    for (i=0; i<OUTER; i++) {
        acc = 0.0;
        for (j=1; j<INNER; j++) {

            /* calculate 2^j */
            x = 1.0;
            for (k=0; k<j; k++) {       
                x *= 2.0;
            }

            /* accumulatively calculate pi */
            y = (real)PI / x;
            acc += y;                   

            /*printf("  ACC%03d: %.16e\n", j, acc);*/
        }
        sum += acc;
        /*printf("  SUM%03d: %.16e\n", i, sum);*/
    }
    
    /* final should be PI*OUTER */
    err = ABS((double)final-(double)sum)/ABS((double)final);

    /*printf("-------------------------------\n");*/
    printf("  RESULT:  %.16e\n", sum);
    printf("  CORRECT: %.16e\n", final);
    printf("  ERROR:   %.16e\n", err);
    printf("  THRESH:  %.16e\n", EPS);
    if ((double)err < (double)EPS) {
        printf("SUM2PI_X - SUCCESSFUL!\n");
    } else {
        printf("SUM2PI_X - FAILED!!!\n");
    }
    return 0;
}

int main() {
    return sum2pi_x();
}

