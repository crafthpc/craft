#include "util.h"

float fnum1 = 1.0f;
float fnum2 = 2.0f;

double dnum1 = 1.0;
double dnum2 = 2.0;

float fsum;
double dsum_single, dsum_double, dsum_ignore;

void do_mixed();
void print_output();
int check();
int mixed();

void do_mixed() {

    // single-precision
    fsum = fnum1 + fnum2;

    // double-precision (single-replaced)
    dsum_single = dnum1 + dnum2;

    // double-precision (double)
    dsum_double = dnum1 + dnum2;

    // double-precision (ignore)
    dsum_ignore = dnum1 + dnum2;

}


void print_output() {
    printf("f - %f + %f = %f\n", fnum1, fnum2, fsum);
    printf("d - %lf + %lf = %lf  (single-precision)\n",
            dnum1, dnum2, dsum_single);
    printf("d - %lf + %lf = %lf  (double-precision)\n",
            dnum1, dnum2, dsum_double);
    printf("d - %lf + %lf = %lf  (ignored)\n",
            dnum1, dnum2, dsum_ignore);
}

int check() {
    if (f_approx_equal(fsum,3.0f) &&
        d_approx_equal(dsum_single,3.0) &&
        d_approx_equal(dsum_double,3.0) &&
        d_approx_equal(dsum_ignore,3.0)) {
        return EXIT_SUCCESS;
    } else {
        return EXIT_FAILURE;
    }
}

int mixed() {
    int status = EXIT_FAILURE;
    do_mixed();
#if PRINT_OUTPUT
    printf("== mixed ==\n");
    print_output();
#endif
    status = check();
    if (status == EXIT_SUCCESS) {
        printf("== mixed: pass\n");
    } else {
        printf("== mixed: FAIL!!!\n");
    }
    return status;
}

int main() {
    return mixed();
}

