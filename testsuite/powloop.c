#include <math.h>
#include "util.h"

#define ITERATIONS 8

float f_num = 2.0f;
double d_num = 2.0;
/*
 *long double ld_num = 2.0L;
 */

float f_nums[ITERATIONS];
double d_nums[ITERATIONS];
/*
 *long double ld_nums[ITERATIONS];
 */

void do_powloop();
void print_output();
int check();
int powloop();

void do_powloop() {
    int i;
    for (i=0; i<ITERATIONS; i++) {
        f_nums[i] = pow(f_num,i);
        d_nums[i] = pow(d_num,i);
        /*
         *ld_nums[i] = pow(ld_num,i);
         */
    }
}

void print_output() {
    int i, p;
    for (i=0,p=1; i<ITERATIONS; i++) {
       printf("f - pow(%f,%d) = %f [correct=%d]\n",
               f_num, i, f_nums[i], p);
       p *= 2;
    }
    for (i=0,p=1; i<ITERATIONS; i++) {
        printf("d - pow(%lf,%d) = %lf [correct=%d]\n",
                d_num, i, d_nums[i], p);
        p *= 2;
    }
    /*
     *for (i=0,p=1; i<ITERATIONS; i++) {
     *    printf("ld - pow(%Lf,%d) = %Lf [correct=%d]\n",
     *            ld_num, i, ld_nums[i], p);
     *    p *= 2;
     *}
     */
}

int check() {
    int i, p, status = EXIT_SUCCESS;
    for (i=0,p=1; i<ITERATIONS; i++) {
       if (!f_approx_equal(f_nums[i], (float)p)) {
           printf("ERROR: f - pow(%f,%d) = %f [correct=%d]\n",
                   f_num, i, f_nums[i], p);
           status = EXIT_FAILURE;
       }
       p *= 2;
    }
    for (i=0,p=1; i<ITERATIONS; i++) {
        if (!d_approx_equal(d_nums[i], (double)p)) {
            printf("ERROR: d - pow(%lf,%d) = %lf [correct=%d]\n",
                    d_num, i, d_nums[i], p);
            status = EXIT_FAILURE;
        }
        p *= 2;
    }
    /*
     *for (i=0,p=1; i<ITERATIONS; i++) {
     *    if (!ld_approx_equal(ld_nums[i], (long double)p)) {
     *        printf("ERROR: ld - pow(%Lf,%d) = %Lf [correct=%d]\n",
     *                ld_num, i, ld_nums[i], p);
     *        status = EXIT_FAILURE;
     *    }
     *    p *= 2;
     *}
     */
    return status;
}

int powloop() {
    int status = EXIT_FAILURE;
    do_powloop();
#if PRINT_OUTPUT
    printf("== powloop ==\n");
    print_output();
#endif
    status = check();
    if (status == EXIT_SUCCESS) {
        printf("== powloop: pass\n");
    } else {
        printf("== powloop: FAIL!!!\n");
    }
    return status;

}

int main() {
    return powloop();
}

