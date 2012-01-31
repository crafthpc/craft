#include "util.h"

float fnum1 = 1.0f;
float fnum2 = 2.0f;

double dnum1 = 1.0;
double dnum2 = 2.0;

/*
 *long double ldnum1 = 1.0L;
 *long double ldnum2 = 2.0L;
 */

float fadd, fsub, fmul, fdiv;
double dadd, dsub, dmul, ddiv;
/*
 *long double ldadd, ldsub, ldmul, lddiv;
 */

void do_arith();
void print_output();
int check();
int arith();

void do_arith() {
    // single-precision
    fadd = fnum1 + fnum2;
    fsub = fnum1 - fnum2;
    fmul = fnum1 * fnum2;
    fdiv = fnum1 / fnum2;

    // double-precision
    dadd = dnum1 + dnum2;
    dsub = dnum1 - dnum2;
    dmul = dnum1 * dnum2;
    ddiv = dnum1 / dnum2;

    // extended double-precision
    /*
     *ldadd = ldnum1 + ldnum2;
     *ldsub = ldnum1 - ldnum2;
     *ldmul = ldnum1 * ldnum2;
     *lddiv = ldnum1 / ldnum2;
     */
}


void print_output() {
    printf("f - %f + %f = %f\n", fnum1, fnum2, fadd);
    printf("f - %f - %f = %f\n", fnum1, fnum2, fsub);
    printf("f - %f * %f = %f\n", fnum1, fnum2, fmul);
    printf("f - %f / %f = %f\n", fnum1, fnum2, fdiv);
    printf("d - %lf + %lf = %lf\n", dnum1, dnum2, dadd);
    printf("d - %lf - %lf = %lf\n", dnum1, dnum2, dsub);
    printf("d - %lf * %lf = %lf\n", dnum1, dnum2, dmul);
    printf("d - %lf / %lf = %lf\n", dnum1, dnum2, ddiv);
    /*
     *printf("ld - %Lf + %Lf = %Lf\n", ldnum1, ldnum2, ldadd);
     *printf("ld - %Lf - %Lf = %Lf\n", ldnum1, ldnum2, ldsub);
     *printf("ld - %Lf * %Lf = %Lf\n", ldnum1, ldnum2, ldmul);
     *printf("ld - %Lf / %Lf = %Lf\n", ldnum1, ldnum2, lddiv);
     */
}

int check() {
    if (f_approx_equal(fadd,3.0f) && f_approx_equal(fsub,-1.0f) &&
        f_approx_equal(fmul,2.0f) && f_approx_equal(fdiv,0.5f) &&
        d_approx_equal(dadd,3.0) && d_approx_equal(dsub,-1.0) &&
        d_approx_equal(dmul,2.0) && d_approx_equal(ddiv,0.5)) {
        /*
         *ld_approx_equal(ldadd,3.0L) && ld_approx_equal(ldsub,-1.0L) &&
         *ld_approx_equal(ldmul,2.0L) && ld_approx_equal(lddiv,0.5L)) {
         */
        return EXIT_SUCCESS;
    } else {
        return EXIT_FAILURE;
    }
}

int arith() {
    int status = EXIT_FAILURE;
    do_arith();
#if PRINT_OUTPUT
    printf("== arith ==\n");
    print_output();
#endif
    status = check();
    if (status == EXIT_SUCCESS) {
        printf("== arith: pass\n");
    } else {
        printf("== arith: FAIL!!!\n");
    }
    return status;
}

int main() {
    return arith();
}

