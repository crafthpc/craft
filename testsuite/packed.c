#include <xmmintrin.h>
#include <stdio.h>
#include <stdlib.h>
#include "util.h"

__m128 fvec1 = { 1.2, 3.4, 5.6, 7.8 };
__m128 fvec2 = { 2.1, 4.3, 6.5, 8.7 };
__m128 fabsvec = { 0.0, 0.0, 0.0, 0.0 };    // filled in init()
__m128 fnegvec = { -0.0, -0.0, -0.0, -0.0 };
__m128 fsgnvec = { 1.2, -3.4, 5.6, -7.8 };
__m128 faddvec, fsubvec, fmulvec, fdivvec;
__m128 fandvec, forvec, fxorvec, fxorzvec;

__m128d dvec1 = { 1.2, 3.4 };
__m128d dvec2 = { 5.6, 7.8 };
__m128d dabsvec = { 0.0, 0.0 };     // filled in init()
__m128d dnegvec = { -0.0, -0.0 };
__m128d dsgnvec = { 1.2, -3.4 };
__m128d daddvec, dsubvec, dmulvec, ddivvec;
__m128d dandvec, dorvec, dxorvec, dxorzvec;

void init();
void do_packed();
void print_output();
int check();
int packed();

void init() {
    uint32_t *fabsv = (uint32_t*)&fabsvec;
    fabsv[0] = 0x7fffffff;
    fabsv[1] = 0x7fffffff;
    fabsv[2] = 0x7fffffff;
    fabsv[3] = 0x7fffffff;
    uint64_t *dabsv = (uint64_t*)&dabsvec;
    dabsv[0] = 0x7fffffffffffffff;
    dabsv[1] = 0x7fffffffffffffff;
}

void do_packed() {
    faddvec = fvec1 + fvec2;
    fsubvec = fvec1 - fvec2;
    fmulvec = fvec1 * fvec2;
    fdivvec = fvec1 / fvec2;
    asm ( "movups %0, %%xmm0" : : "m" (fsgnvec) );
    asm ( "pand %0, %%xmm0" : : "m" (fabsvec) );
    asm ( "movups %%xmm0, %0" : "=m" (fandvec) );
    asm ( "movups %0, %%xmm0" : : "m" (fsgnvec) );
    asm ( "por %0, %%xmm0" : : "m" (fnegvec) );
    asm ( "movups %%xmm0, %0" : "=m" (forvec) );
    asm ( "movups %0, %%xmm0" : : "m" (fsgnvec) );
    asm ( "pxor %0, %%xmm0" : : "m" (fnegvec) );
    asm ( "movups %%xmm0, %0" : "=m" (fxorvec) );
    asm ( "movups %0, %%xmm0" : : "m" (fsgnvec) );
    asm ( "pxor %0, %%xmm0" : : "m" (fsgnvec) );
    asm ( "movups %%xmm0, %0" : "=m" (fxorzvec) );
    daddvec = dvec1 + dvec2;
    dsubvec = dvec1 - dvec2;
    dmulvec = dvec1 * dvec2;
    ddivvec = dvec1 / dvec2;
    asm ( "movupd %0, %%xmm0" : : "m" (dsgnvec) );
    asm ( "pand %0, %%xmm0" : : "m" (dabsvec) );
    asm ( "movupd %%xmm0, %0" : "=m" (dandvec) );
    asm ( "movupd %0, %%xmm0" : : "m" (dsgnvec) );
    asm ( "por %0, %%xmm0" : : "m" (dnegvec) );
    asm ( "movupd %%xmm0, %0" : "=m" (dorvec) );
    asm ( "movupd %0, %%xmm0" : : "m" (dsgnvec) );
    asm ( "pxor %0, %%xmm0" : : "m" (dnegvec) );
    asm ( "movupd %%xmm0, %0" : "=m" (dxorvec) );
    asm ( "movupd %0, %%xmm0" : : "m" (dsgnvec) );
    asm ( "pxor %0, %%xmm0" : : "m" (dsgnvec) );
    asm ( "movupd %%xmm0, %0" : "=m" (dxorzvec) );
}

void print_output() {
    printf("faddvec:  %8.4f %8.4f %8.4f %8.4f\n", 
            ((float*)&faddvec)[0], ((float*)&faddvec)[1],
            ((float*)&faddvec)[2], ((float*)&faddvec)[3]);
    printf("fsubvec:  %8.4f %8.4f %8.4f %8.4f\n", 
            ((float*)&fsubvec)[0], ((float*)&fsubvec)[1],
            ((float*)&fsubvec)[2], ((float*)&fsubvec)[3]);
    printf("fmulvec:  %8.4f %8.4f %8.4f %8.4f\n", 
            ((float*)&fmulvec)[0], ((float*)&fmulvec)[1],
            ((float*)&fmulvec)[2], ((float*)&fmulvec)[3]);
    printf("fdivvec:  %8.4f %8.4f %8.4f %8.4f\n", 
            ((float*)&fdivvec)[0], ((float*)&fdivvec)[1],
            ((float*)&fdivvec)[2], ((float*)&fdivvec)[3]);
    printf("fandvec:  %8.4f %8.4f %8.4f %8.4f\n", 
            ((float*)&fandvec)[0], ((float*)&fandvec)[1],
            ((float*)&fandvec)[2], ((float*)&fandvec)[3]);
    printf("forvec:   %8.4f %8.4f %8.4f %8.4f\n", 
            ((float*)&forvec)[0], ((float*)&forvec)[1],
            ((float*)&forvec)[2], ((float*)&forvec)[3]);
    printf("fxorvec:  %8.4f %8.4f %8.4f %8.4f\n", 
            ((float*)&fxorvec)[0], ((float*)&fxorvec)[1],
            ((float*)&fxorvec)[2], ((float*)&fxorvec)[3]);
    printf("fxorzvec: %8.4f %8.4f %8.4f %8.4f\n", 
            ((float*)&fxorzvec)[0], ((float*)&fxorzvec)[1],
            ((float*)&fxorzvec)[2], ((float*)&fxorzvec)[3]);
    printf("daddvec:  %8.4f %8.4f\n", ((double*)&daddvec)[0], ((double*)&daddvec)[1]);
    printf("dsubvec:  %8.4f %8.4f\n", ((double*)&dsubvec)[0], ((double*)&dsubvec)[1]);
    printf("dmulvec:  %8.4f %8.4f\n", ((double*)&dmulvec)[0], ((double*)&dmulvec)[1]);
    printf("ddivvec:  %8.4f %8.4f\n", ((double*)&ddivvec)[0], ((double*)&ddivvec)[1]);
    printf("dandvec:  %8.4f %8.4f\n", ((double*)&dandvec)[0], ((double*)&dandvec)[1]);
    printf("dorvec:   %8.4f %8.4f\n", ((double*)&dorvec)[0],  ((double*)&dorvec)[1]);
    printf("dxorvec:  %8.4f %8.4f\n", ((double*)&dxorvec)[0], ((double*)&dxorvec)[1]);
    printf("dxorzvec: %8.4f %8.4f\n", ((double*)&dxorzvec)[0],((double*)&dxorzvec)[1]);
}

int check() {
    int status = EXIT_SUCCESS;
    if ( !( f_approx_equal(((float*)&faddvec)[0],  3.3) &&
            f_approx_equal(((float*)&faddvec)[1],  7.7) &&
            f_approx_equal(((float*)&faddvec)[2], 12.1) &&
            f_approx_equal(((float*)&faddvec)[3], 16.5))) {
        printf("ERROR: fadd\n");
        status = EXIT_FAILURE;
    }

    if ( !( f_approx_equal(((float*)&fsubvec)[0], -0.9) &&
            f_approx_equal(((float*)&fsubvec)[1], -0.9) &&
            f_approx_equal(((float*)&fsubvec)[2], -0.9) &&
            f_approx_equal(((float*)&fsubvec)[3], -0.9))) {
        printf("ERROR: fsub\n");
        status = EXIT_FAILURE;
    }

    if ( !( f_approx_equal(((float*)&fmulvec)[0],  2.52) &&
            f_approx_equal(((float*)&fmulvec)[1], 14.62) &&
            f_approx_equal(((float*)&fmulvec)[2], 36.40) &&
            f_approx_equal(((float*)&fmulvec)[3], 67.86))) {
        printf("ERROR: fmul\n");
        status = EXIT_FAILURE;
    }

    if ( !( f_approx_equal(((float*)&fdivvec)[0], 0.5714) &&
            f_approx_equal(((float*)&fdivvec)[1], 0.7907) &&
            f_approx_equal(((float*)&fdivvec)[2], 0.8615) &&
            f_approx_equal(((float*)&fdivvec)[3], 0.8966))) {
        printf("ERROR: fdiv\n");
        status = EXIT_FAILURE;
    }

    if ( !( f_approx_equal(((float*)&fandvec)[0],  1.2) &&
            f_approx_equal(((float*)&fandvec)[1],  3.4) &&
            f_approx_equal(((float*)&fandvec)[2],  5.6) &&
            f_approx_equal(((float*)&fandvec)[3],  7.8))) {
        printf("ERROR: fand\n");
        status = EXIT_FAILURE;
    }

    if ( !( f_approx_equal(((float*)&forvec)[0],  -1.2) &&
            f_approx_equal(((float*)&forvec)[1],  -3.4) &&
            f_approx_equal(((float*)&forvec)[2],  -5.6) &&
            f_approx_equal(((float*)&forvec)[3],  -7.8))) {
        printf("ERROR: for\n");
        status = EXIT_FAILURE;
    }

    if ( !( f_approx_equal(((float*)&fxorvec)[0], -1.2) &&
            f_approx_equal(((float*)&fxorvec)[1],  3.4) &&
            f_approx_equal(((float*)&fxorvec)[2], -5.6) &&
            f_approx_equal(((float*)&fxorvec)[3],  7.8))) {
        printf("ERROR: fxor\n");
        status = EXIT_FAILURE;
    }

    if ( !( f_approx_equal(((float*)&fxorzvec)[0], 0.0) &&
            f_approx_equal(((float*)&fxorzvec)[1], 0.0) &&
            f_approx_equal(((float*)&fxorzvec)[2], 0.0) &&
            f_approx_equal(((float*)&fxorzvec)[3], 0.0))) {
        printf("ERROR: fxorz\n");
        status = EXIT_FAILURE;
    }

    if ( !( d_approx_equal(((double*)&daddvec)[0],  6.8) &&
            d_approx_equal(((double*)&daddvec)[1], 11.2))) {
        printf("ERROR: dadd\n");
        status = EXIT_FAILURE;
    }

    if ( !( d_approx_equal(((double*)&dsubvec)[0], -4.4) &&
            d_approx_equal(((double*)&dsubvec)[1], -4.4))) {
        printf("ERROR: dsub\n");
        status = EXIT_FAILURE;
    }

    if ( !( d_approx_equal(((double*)&dmulvec)[0],  6.72) &&
            d_approx_equal(((double*)&dmulvec)[1], 26.52))) {
        printf("ERROR: dmul\n");
        status = EXIT_FAILURE;
    }

    if ( !( d_approx_equal(((double*)&ddivvec)[0], 0.2143) &&
            d_approx_equal(((double*)&ddivvec)[1], 0.4359))) {
        printf("ERROR: ddiv\n");
        status = EXIT_FAILURE;
    }

    if ( !( d_approx_equal(((double*)&dandvec)[0],  1.2) &&
            d_approx_equal(((double*)&dandvec)[1],  3.4))) {
        printf("ERROR: dand\n");
        status = EXIT_FAILURE;
    }

    if ( !( d_approx_equal(((double*)&dorvec)[0],  -1.2) &&
            d_approx_equal(((double*)&dorvec)[1],  -3.4))) {
        printf("ERROR: dor\n");
        status = EXIT_FAILURE;
    }

    if ( !( d_approx_equal(((double*)&dxorvec)[0], -1.2) &&
            d_approx_equal(((double*)&dxorvec)[1],  3.4))) {
        printf("ERROR: dxor\n");
        status = EXIT_FAILURE;
    }

    if ( !( d_approx_equal(((double*)&dxorzvec)[0], 0.0) &&
            d_approx_equal(((double*)&dxorzvec)[1], 0.0))) {
        printf("ERROR: dxorz\n");
        status = EXIT_FAILURE;
    }


    return status;
}

int packed() {
    int status = EXIT_FAILURE;
    init();
    do_packed();
#if PRINT_OUTPUT
    printf("== packed ==\n");
    print_output();
#endif
    status = check();
    if (status == EXIT_SUCCESS) {
        printf("== packed: pass\n");
    } else {
        printf("== packed: FAIL!!!\n");
    }
    return status;
}

int main() {
    return packed();
}

