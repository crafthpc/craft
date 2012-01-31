#include <math.h>
#include "util.h"

// {{{ one-argument macros

#define DECLARE1(func,num,ans) \
    float f_##func##_input = num##f; \
    float f_##func##_result;        float f_##func##_ans = ans##f; \
    double d_##func##_input = num; \
    double d_##func##_result;       double d_##func##_ans = ans;

    /*
     *long double ld_##func##_input = num##L; \
     *long double ld_##func##_result; long double ld_##func##_ans = ans##L
     */

#define EXECUTE1(func) \
    f_##func##_result  = func##f(f_##func##_input); \
    d_##func##_result  = func(d_##func##_input);

    /*
     *printf(#func " result=%g [0x%lx]\n", d_##func##_result, \
     *        *(uint64_t*)(&d_##func##_result));
     */

    /*
     *ld_##func##_result = func##l(ld_##func##_input)
     */

#define PRINT1(func) \
    printf("f - " #func "(%f) = %f [correct=%f]\n", \
            f_##func##_input,  f_##func##_result,  f_##func##_ans); \
    printf("d - " #func "(%lf) = %lf [correct=%lf]\n", \
            d_##func##_input,  d_##func##_result,  d_##func##_ans);

    /*
     *printf("ld - " #func "(%Lf) = %.18Lf [correct=%Lf]\n", \
     *        ld_##func##_input, ld_##func##_result, ld_##func##_ans)
     */

#define CHECK1(func) \
    if (!f_approx_equal(f_##func##_result, f_##func##_ans)) { \
        printf("ERROR: f - " #func "(%f) = %f [correct=%f]\n", \
                f_##func##_input,  f_##func##_result,  f_##func##_ans); \
        status = EXIT_FAILURE; } \
    if (!d_approx_equal(d_##func##_result, d_##func##_ans)) { \
        printf("ERROR: d - " #func "(%lf) = %lf [correct=%lf]\n", \
                d_##func##_input,  d_##func##_result,  d_##func##_ans); \
        status = EXIT_FAILURE; }

    /*
     *if (!ld_approx_equal(ld_##func##_result, ld_##func##_ans)) { \
     *    printf("ERROR: ld - " #func "(%Lf) = %Lf [correct=%Lf]\n", \
     *            ld_##func##_input, ld_##func##_result, ld_##func##_ans); \
     *    status = EXIT_FAILURE; }
     */

// }}}

// {{{ two-argument macros

#define DECLARE2(func,num1,num2,ans) \
    float f_##func##_input1 = num1##f; float f_##func##_input2 = num2##f; \
    float f_##func##_result;           float f_##func##_ans = ans##f; \
    double d_##func##_input1 = num1;   double d_##func##_input2 = num2; \
    double d_##func##_result;          double d_##func##_ans = ans;

    /*
     *long double ld_##func##_input1 = num1##L; \
     *long double ld_##func##_input2 = num2##L; \
     *long double ld_##func##_result; \
     *long double ld_##func##_ans = ans##L
     */

#define EXECUTE2(func) \
    f_##func##_result  = func##f(f_##func##_input1, f_##func##_input2); \
    d_##func##_result  = func(d_##func##_input1, d_##func##_input2);

    /*
     *ld_##func##_result = func##l(ld_##func##_input1, ld_##func##_input2)
     */

#define PRINT2(func) \
    printf("f - " #func "(%f,%f) = %f [correct=%f]\n", f_##func##_input1, \
            f_##func##_input2,  f_##func##_result,  f_##func##_ans); \
    printf("d - " #func "(%lf,%lf) = %lf [correct=%lf]\n", d_##func##_input1, \
            d_##func##_input2,  d_##func##_result,  d_##func##_ans);

    /*
     *printf("ld - " #func "(%Lf,%Lf) = %Lf [correct=%Lf]\n", ld_##func##_input1, \
     *        ld_##func##_input2, ld_##func##_result, ld_##func##_ans)
     */

#define CHECK2(func) \
    if (!f_approx_equal(f_##func##_result, f_##func##_ans)) { \
        printf("ERROR: f - " #func "(%f,%f) = %f [correct=%f]\n", f_##func##_input1, \
                f_##func##_input2,  f_##func##_result,  f_##func##_ans); \
        status = EXIT_FAILURE; } \
    if (!d_approx_equal(d_##func##_result, d_##func##_ans)) { \
        printf("ERROR: d - " #func "(%lf,%lf) = %lf [correct=%lf]\n", d_##func##_input1, \
                d_##func##_input2,  d_##func##_result,  d_##func##_ans); \
        status = EXIT_FAILURE; }

    /*
     *if (!ld_approx_equal(ld_##func##_result, ld_##func##_ans)) { \
     *    printf("ERROR: ld - " #func "(%Lf,%Lf) = %Lf [correct=%Lf]\n", ld_##func##_input1, \
     *            ld_##func##_input2, ld_##func##_result, ld_##func##_ans); \
     *    status = EXIT_FAILURE; }
     */

// }}}

DECLARE1(fabs,    -4.2,   4.2);
DECLARE1(ceil,     4.2,   5.0);
DECLARE1(erf,      1.0,   0.842700792949714869);
DECLARE1(erfc,     1.0,   0.157299207050285131);
DECLARE1(exp,      1.0,   2.718281828459045235);
DECLARE1(exp2,     1.2,   2.297396709994070014);
DECLARE1(floor,    4.2,   4.0);
DECLARE1(log,      42.0,  3.737669618283368306);
DECLARE1(logb,     42.0,  5.0);
DECLARE1(log10,    42.0,  1.623249290397900463);
DECLARE1(sqrt,     4.2,   2.049390153191919677);
DECLARE1(trunc,    4.2,   4.0);
DECLARE1(sin,      0.777, 0.701143517030469999);
DECLARE1(cos,      0.777, 0.713020173996600903);
DECLARE1(tan,      0.777, 0.983343168399345287);
DECLARE1(asin,     0.777, 0.889886016669503242);
DECLARE1(acos,     0.777, 0.680910310125393377);
DECLARE1(atan,     0.777, 0.660558370772454408);
DECLARE1(sinh,     0.777, 0.857577180643534463);
DECLARE1(cosh,     0.777, 1.317360474874099652);
DECLARE1(tanh,     0.777, 0.650981410934993513);
DECLARE1(asinh,    0.777, 0.714607366139231117);
DECLARE1(acosh,    2.777, 1.680399601619593669);
DECLARE1(atanh,    0.777, 1.037755028347266287);

DECLARE2(fmin,     1.4,  1.7,  1.4);
DECLARE2(fmax,     1.4,  1.7,  1.7);
DECLARE2(atan2,    1.2,  1.3,  0.745419476274158268);
DECLARE2(copysign, 4.2, -2.4, -4.2);
DECLARE2(fmod,     4.2,  1.2,  0.6);
DECLARE2(pow,      1.2,  1.30, 1.267463962127109804);

float f_frexp_input = 1337.0f;
float f_frexp_output_sig; float f_frexp_ans_sig = 0.65283203125f;
int f_frexp_output_exp;   int f_frexp_ans_exp = 11;
double d_frexp_input = 1337.0;
double d_frexp_output_sig; double d_frexp_ans_sig = 0.65283203125;
int d_frexp_output_exp;    int d_frexp_ans_exp = 11;
/*
 *long double ld_frexp_input = 1337.0L;
 *long double ld_frexp_output_sig; long double ld_frexp_ans_sig = 0.65283203125L;
 *int ld_frexp_output_exp;         int ld_frexp_ans_exp = 11;
 */

float f_ldexp_input_sig = 1.23456f;        int f_ldexp_input_exp = 6;
float f_ldexp_output;        float f_ldexp_ans = 79.01184f;
double d_ldexp_input_sig = 1.23456;        int d_ldexp_input_exp = 6;
double d_ldexp_output;       double d_ldexp_ans = 79.01184;
/*
 *long double ld_ldexp_input_sig = 1.23456L; int ld_ldexp_input_exp = 6;
 *long double ld_ldexp_output; long double ld_ldexp_ans = 79.01184L;
 */

float f_modf_input = 123.456f;
float f_modf_output_int;  float f_modf_ans_int = 123.0f;
float f_modf_output_frac; float f_modf_ans_frac = 0.456f;
double d_modf_input = 123.456;
double d_modf_output_int;  double d_modf_ans_int = 123.0;
double d_modf_output_frac; double d_modf_ans_frac = 0.456;
/*
 *long double ld_modf_input = 123.456L;
 *long double ld_modf_output_int;  long double ld_modf_ans_int = 123.0L;
 *long double ld_modf_output_frac; long double ld_modf_ans_frac = 0.456L;
 */

float f_sincos_input = 0.777f;
float f_sincos_output_sin; float f_sincos_ans_sin = 0.701143517030469999f;
float f_sincos_output_cos; float f_sincos_ans_cos = 0.713020173996600903f;
double d_sincos_input = 0.777;
double d_sincos_output_sin; double d_sincos_ans_sin = 0.701143517030469999;
double d_sincos_output_cos; double d_sincos_ans_cos = 0.713020173996600903;
/*
 *long double ld_sincos_input = 0.777L;
 *long double ld_sincos_output_sin; long double ld_sincos_ans_sin = 0.701143517030469999L;
 *long double ld_sincos_output_cos; long double ld_sincos_ans_cos = 0.713020173996600903L;
 */

void do_calllib();
void print_output();
int check();
int calllib();

void do_calllib() {
    EXECUTE1(fabs);  EXECUTE1(ceil);  EXECUTE1(erf);   EXECUTE1(erfc);
    EXECUTE1(exp);   EXECUTE1(exp2);  EXECUTE1(floor); EXECUTE1(log);
    EXECUTE1(logb);  EXECUTE1(log10); EXECUTE1(sqrt);  EXECUTE1(trunc);
    EXECUTE1(sin);   EXECUTE1(cos);   EXECUTE1(tan);
    EXECUTE1(asin);  EXECUTE1(acos);  EXECUTE1(atan);
    EXECUTE1(sinh);  EXECUTE1(cosh);  EXECUTE1(tanh);
    EXECUTE1(asinh); EXECUTE1(acosh); EXECUTE1(atanh);
    EXECUTE2(fmin);  EXECUTE2(fmax);
    EXECUTE2(atan2); EXECUTE2(copysign); EXECUTE2(fmod); EXECUTE2(pow);
    // {{{ frexp
    f_frexp_output_sig  = frexpf(f_frexp_input,  &f_frexp_output_exp);
    d_frexp_output_sig  = frexp (d_frexp_input,  &d_frexp_output_exp);
    /*
     *ld_frexp_output_sig = frexpl(ld_frexp_input, &ld_frexp_output_exp);
     */
    // }}}
    // {{{ ldexp
    f_ldexp_output  = ldexpf(f_ldexp_input_sig,  f_ldexp_input_exp);
    d_ldexp_output  = ldexp (d_ldexp_input_sig,  d_ldexp_input_exp);
    /*
     *ld_ldexp_output = ldexpl(ld_ldexp_input_sig, ld_ldexp_input_exp);
     */
    // }}}
    // {{{ modf
    f_modf_output_frac  = modff(f_modf_input,  &f_modf_output_int);
    d_modf_output_frac  = modf (d_modf_input,  &d_modf_output_int);
    /*
     *ld_modf_output_frac = modfl(ld_modf_input, &ld_modf_output_int);
     */
    // }}}
    // {{{ sincos
    sincosf(f_sincos_input,  &f_sincos_output_sin,  &f_sincos_output_cos);
    sincos (d_sincos_input,  &d_sincos_output_sin,  &d_sincos_output_cos);
    /*
     *sincosl(ld_sincos_input, &ld_sincos_output_sin, &ld_sincos_output_cos);
     */
    // }}}
}

void print_output() {
    PRINT1(fabs);  PRINT1(ceil);  PRINT1(erf);   PRINT1(erfc);
    PRINT1(exp);   PRINT1(exp2);  PRINT1(floor); PRINT1(log);
    PRINT1(logb);  PRINT1(log10); PRINT1(sqrt);  PRINT1(trunc);
    PRINT1(sin);   PRINT1(cos);   PRINT1(tan);
    PRINT1(asin);  PRINT1(acos);  PRINT1(atan);
    PRINT1(sinh);  PRINT1(cosh);  PRINT1(tanh);
    PRINT1(asinh); PRINT1(acosh); PRINT1(atanh);
    PRINT2(fmin);  PRINT2(fmax);
    PRINT2(atan2); PRINT2(copysign); PRINT2(fmod); PRINT2(pow);
    // {{{ frexp
    printf("f - frexp(%f) = (%f,%d) [correct=(%f,%d)]\n",
            f_frexp_input, f_frexp_output_sig, f_frexp_output_exp,
            f_frexp_ans_sig, f_frexp_ans_exp);
    printf("d - frexp(%lf) = (%lf,%d) [correct=(%lf,%d)]\n",
            d_frexp_input, d_frexp_output_sig, d_frexp_output_exp,
            d_frexp_ans_sig, d_frexp_ans_exp);
    /*
     *printf("ld - frexp(%Lf) = (%Lf,%d) [correct=(%Lf,%d)]\n",
     *        ld_frexp_input, ld_frexp_output_sig, ld_frexp_output_exp,
     *        ld_frexp_ans_sig, ld_frexp_ans_exp);
     */
    // }}}
    // {{{ ldexp
    printf("f - ldexp(%f,%d) = %f [correct=%f]\n",
            f_ldexp_input_sig, f_ldexp_input_exp,
            f_ldexp_output, f_ldexp_ans);
    printf("d - ldexp(%lf,%d) = %lf [correct=%lf]\n",
            d_ldexp_input_sig, d_ldexp_input_exp,
            d_ldexp_output, d_ldexp_ans);
    /*
     *printf("ld - ldexp(%Lf,%d) = %Lf [correct=%Lf]\n",
     *        ld_ldexp_input_sig, ld_ldexp_input_exp,
     *        ld_ldexp_output, ld_ldexp_ans);
     */
    // }}}
    // {{{ modf
    printf("f - modf(%f) = (%f,%f) [correct=(%f,%f)]\n",
            f_modf_input, f_modf_output_int, f_modf_output_frac,
            f_modf_ans_int, f_modf_ans_frac);
    printf("d - modf(%lf) = (%lf,%lf) [correct=(%lf,%lf)]\n",
            d_modf_input, d_modf_output_int, d_modf_output_frac,
            d_modf_ans_int, d_modf_ans_frac);
    /*
     *printf("ld - modf(%Lf) = (%Lf,%Lf) [correct=(%Lf,%Lf)]\n",
     *        ld_modf_input, ld_modf_output_int, ld_modf_output_frac,
     *        ld_modf_ans_int, ld_modf_ans_frac);
     */
    // }}}
    // {{{ sincos
    printf("f - sincos(%f) = (%f,%f) [correct=(%f,%f)]\n",
            f_sincos_input, f_sincos_output_sin, f_sincos_output_cos,
            f_sincos_ans_sin, f_sincos_ans_cos);
    printf("d - sincos(%lf) = (%lf,%lf) [correct=(%lf,%lf)]\n",
            d_sincos_input, d_sincos_output_sin, d_sincos_output_cos,
            d_sincos_ans_sin, d_sincos_ans_cos);
    /*
     *printf("ld - sincos(%Lf) = (%Lf,%Lf) [correct=(%Lf,%Lf)]\n",
     *        ld_sincos_input, ld_sincos_output_sin, ld_sincos_output_cos,
     *        ld_sincos_ans_sin, ld_sincos_ans_cos);
     */
    // }}}
}

int check() {
    int status = EXIT_SUCCESS;
    CHECK1(fabs);  CHECK1(ceil);  CHECK1(erf);   CHECK1(erfc);
    CHECK1(exp);   CHECK1(exp2);  CHECK1(floor); CHECK1(log);
    CHECK1(logb);  CHECK1(log10); CHECK1(sqrt);  CHECK1(trunc);
    CHECK1(sin);   CHECK1(cos);   CHECK1(tan);
    CHECK1(asin);  CHECK1(acos);  CHECK1(atan);
    CHECK1(sinh);  CHECK1(cosh);  CHECK1(tanh);
    CHECK1(asinh); CHECK1(acosh); CHECK1(atanh);
    CHECK2(fmin);  CHECK2(fmax);
    CHECK2(atan2); CHECK2(copysign); CHECK2(fmod); CHECK2(pow);
    // {{{ frexp
    if (!f_approx_equal(f_frexp_output_sig, f_frexp_ans_sig) || 
        f_frexp_output_exp != f_frexp_ans_exp) {
        printf("ERROR: f - frexp(%f) = (%f,%d) [correct=(%f,%d)]\n",
                f_frexp_input, f_frexp_output_sig, f_frexp_output_exp,
                f_frexp_ans_sig, f_frexp_ans_exp);
        status = EXIT_FAILURE;
    }
    if (!d_approx_equal(d_frexp_output_sig, d_frexp_ans_sig) || 
        d_frexp_output_exp != d_frexp_ans_exp) {
        printf("ERROR: d - frexp(%lf) = (%lf,%d) [correct=(%lf,%d)]\n",
                d_frexp_input, d_frexp_output_sig, d_frexp_output_exp,
                d_frexp_ans_sig, d_frexp_ans_exp);
        status = EXIT_FAILURE;
    }
    /*
     *if (!ld_approx_equal(ld_frexp_output_sig, ld_frexp_ans_sig) || 
     *    ld_frexp_output_exp != ld_frexp_ans_exp) {
     *    printf("ERROR: ld - frexp(%Lf) = (%Lf,%d) [correct=(%Lf,%d)]\n",
     *            ld_frexp_input, ld_frexp_output_sig, ld_frexp_output_exp,
     *            ld_frexp_ans_sig, ld_frexp_ans_exp);
     *    status = EXIT_FAILURE;
     *}
     */
    // }}}
    // {{{ ldexp
    if (!f_approx_equal(f_ldexp_output, f_ldexp_ans)) {
        printf("ERROR: f - ldexp(%f,%d) = %f [correct=%f]\n",
                f_ldexp_input_sig, f_ldexp_input_exp,
                f_ldexp_output, f_ldexp_ans);
        status = EXIT_FAILURE;
    }
    if (!d_approx_equal(d_ldexp_output, d_ldexp_ans)) {
        printf("ERROR: d - ldexp(%lf,%d) = %lf [correct=%lf]\n",
                d_ldexp_input_sig, d_ldexp_input_exp,
                d_ldexp_output, d_ldexp_ans);
        status = EXIT_FAILURE;
    }
    /*
     *if (!ld_approx_equal(ld_ldexp_output, ld_ldexp_ans)) {
     *    printf("ERROR: ld - ldexp(%Lf,%d) = %Lf [correct=%Lf]\n",
     *            ld_ldexp_input_sig, ld_ldexp_input_exp,
     *            ld_ldexp_output, ld_ldexp_ans);
     *    status = EXIT_FAILURE;
     *}
     */
    // }}}
    // {{{ modf
    if (!f_approx_equal(f_modf_output_int, f_modf_ans_int) ||
        !f_approx_equal(f_modf_output_frac, f_modf_ans_frac)) {
        printf("ERROR: f - modf(%f) = (%f,%f) [correct=(%f,%f)]\n",
                f_modf_input, f_modf_output_int, f_modf_output_frac,
                f_modf_ans_int, f_modf_ans_frac);
        status = EXIT_FAILURE;
    }
    if (!d_approx_equal(d_modf_output_int, d_modf_ans_int) ||
        !d_approx_equal(d_modf_output_frac, d_modf_ans_frac)) {
        printf("ERROR: d - modf(%lf) = (%lf,%lf) [correct=(%lf,%lf)]\n",
                d_modf_input, d_modf_output_int, d_modf_output_frac,
                d_modf_ans_int, d_modf_ans_frac);
        status = EXIT_FAILURE;
    }
    /*
     *if (!ld_approx_equal(ld_modf_output_int, ld_modf_ans_int) ||
     *    !ld_approx_equal(ld_modf_output_frac, ld_modf_ans_frac)) {
     *    printf("ERROR: ld - modf(%Lf) = (%Lf,%Lf) [correct=(%Lf,%Lf)]\n",
     *            ld_modf_input, ld_modf_output_int, ld_modf_output_frac,
     *            ld_modf_ans_int, ld_modf_ans_frac);
     *    status = EXIT_FAILURE;
     *}
     */
    // }}}
    // {{{ sincos
    if (!f_approx_equal(f_sincos_output_sin, f_sincos_ans_sin) ||
        !f_approx_equal(f_sincos_output_cos, f_sincos_ans_cos)) {
        printf("ERROR: f - sincos(%f) = (%f,%f) [correct=(%f,%f)]\n",
                f_sincos_input, f_sincos_output_sin, f_sincos_output_cos,
                f_sincos_ans_sin, f_sincos_ans_cos);
        status = EXIT_FAILURE;
    }
    if (!d_approx_equal(d_sincos_output_sin, d_sincos_ans_sin) ||
        !d_approx_equal(d_sincos_output_cos, d_sincos_ans_cos)) {
        printf("ERROR: d - sincos(%lf) = (%lf,%lf) [correct=(%lf,%lf)]\n",
                d_sincos_input, d_sincos_output_sin, d_sincos_output_cos,
                d_sincos_ans_sin, d_sincos_ans_cos);
        status = EXIT_FAILURE;
    }
    /*
     *if (!ld_approx_equal(ld_sincos_output_sin, ld_sincos_ans_sin) ||
     *    !ld_approx_equal(ld_sincos_output_cos, ld_sincos_ans_cos)) {
     *    printf("ERROR: ld - sincos(%Lf) = (%Lf,%Lf) [correct=(%Lf,%Lf)]\n",
     *            ld_sincos_input, ld_sincos_output_sin, ld_sincos_output_cos,
     *            ld_sincos_ans_sin, ld_sincos_ans_cos);
     *    status = EXIT_FAILURE;
     *}
     */
    // }}}
    return status;
}

int calllib() {
    int status = EXIT_FAILURE;
    do_calllib();
#if PRINT_OUTPUT
    printf("== calllib ==\n");
    print_output();
#endif
    status = check();
    if (status == EXIT_SUCCESS) {
        printf("== calllib: pass\n");
    } else {
        printf("== calllib: FAIL!!!\n");
    }
    return status;
}

int main() {
    return calllib();
}

