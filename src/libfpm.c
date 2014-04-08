#include "libfpm.h"

#define CHECK_FPSTSW \
    unsigned fpstsw = 0; \
    __asm__ ("fnstsw %0;" : : "m" (fpstsw)); \
    if (fpstsw & 0x40) { \
        puts("FP STACK OVERFLOW\n"); \
    }

        /*double rr = _ORIG_##of(rx); \*/
        /*printf(#of " %lx %f = %f\n", *(uint64_t*)(&x), rx, rr); \*/

#define LIBM_WRAPPER_DBL_ONEARG(of,fn) \
    extern "C" double _ORIG_##of(double) {} \
    extern "C" double fn(double x) \
    { \
        CHECK_FPSTSW \
        double rx = (FPFLAG(x) ? FPCONV(x) : x); \
        return _ORIG_##of(rx); \
    }

#define LIBM_WRAPPER_VAR_ONEARG(of,fn,rt,xt) \
    extern "C" rt _ORIG_##of(xt) {} \
    extern "C" rt fn(xt x) \
    { \
        CHECK_FPSTSW \
        xt rx = (FPFLAG(x) ? FPCONV(x) : x); \
        return (rt)_ORIG_##of(rx); \
    }

        /*extern "C" double _ORIG_##of(double, double) __attribute__((weak)); \*/
        /*printf(#of " %lx %lx", *(uint64_t*)(&x), *(uint64_t*)(&y)); \*/
        /*printf(" %f %f\n", rx, ry); \*/

#define LIBM_WRAPPER_DBL_TWOARG(of,fn) \
    extern "C" double _ORIG_##of(double, double) {} \
    extern "C" double fn(double x, double y) \
    { \
        CHECK_FPSTSW \
        double rx = (FPFLAG(x) ? FPCONV(x) : x); \
        double ry = (FPFLAG(y) ? FPCONV(y) : y); \
        return _ORIG_##of(rx,ry); \
    }


LIBM_WRAPPER_DBL_ONEARG(fabs,  _INST_fabs);
LIBM_WRAPPER_DBL_ONEARG(ceil,  _INST_ceil);
LIBM_WRAPPER_DBL_ONEARG(erf,   _INST_erf);
LIBM_WRAPPER_DBL_ONEARG(erfc,  _INST_erfc);
LIBM_WRAPPER_DBL_ONEARG(exp,   _INST_exp);
LIBM_WRAPPER_DBL_ONEARG(exp2,  _INST_exp2);
LIBM_WRAPPER_DBL_ONEARG(floor, _INST_floor);
LIBM_WRAPPER_DBL_ONEARG(log,   _INST_log);
LIBM_WRAPPER_DBL_ONEARG(logb,  _INST_logb);
LIBM_WRAPPER_DBL_ONEARG(log10, _INST_log10);
LIBM_WRAPPER_DBL_ONEARG(sqrt,  _INST_sqrt);
LIBM_WRAPPER_DBL_ONEARG(cbrt,  _INST_cbrt);
LIBM_WRAPPER_DBL_ONEARG(trunc, _INST_trunc);

LIBM_WRAPPER_DBL_ONEARG(sin,   _INST_sin);
LIBM_WRAPPER_DBL_ONEARG(cos,   _INST_cos);
LIBM_WRAPPER_DBL_ONEARG(tan,   _INST_tan);
LIBM_WRAPPER_DBL_ONEARG(asin,  _INST_asin);
LIBM_WRAPPER_DBL_ONEARG(acos,  _INST_acos);
LIBM_WRAPPER_DBL_ONEARG(atan,  _INST_atan);
LIBM_WRAPPER_DBL_ONEARG(sinh,  _INST_sinh);
LIBM_WRAPPER_DBL_ONEARG(cosh,  _INST_cosh);
LIBM_WRAPPER_DBL_ONEARG(tanh,  _INST_tanh);
LIBM_WRAPPER_DBL_ONEARG(asinh, _INST_asinh);
LIBM_WRAPPER_DBL_ONEARG(acosh, _INST_acosh);
LIBM_WRAPPER_DBL_ONEARG(atanh, _INST_atanh);

LIBM_WRAPPER_DBL_TWOARG(atan2, _INST_atan2);
LIBM_WRAPPER_DBL_TWOARG(fmod,  _INST_fmod);
LIBM_WRAPPER_DBL_TWOARG(pow,   _INST_pow);

LIBM_WRAPPER_VAR_ONEARG(fpclassify, _INST_fpclassify, int, double);
LIBM_WRAPPER_VAR_ONEARG(isnormal,   _INST_isnormal,   int, double);

LIBM_WRAPPER_DBL_ONEARG(__ceil_sse41,  _INST_ceil_sse);
LIBM_WRAPPER_DBL_ONEARG(__floor_sse41, _INST_floor_sse);
LIBM_WRAPPER_DBL_ONEARG(__sin_sse2,   _INST_sin_sse);
LIBM_WRAPPER_DBL_ONEARG(__cos_sse2,   _INST_cos_sse);
LIBM_WRAPPER_DBL_ONEARG(__tan_sse2,   _INST_tan_sse);
LIBM_WRAPPER_DBL_ONEARG(__atan_sse2,  _INST_atan_sse);
LIBM_WRAPPER_VAR_ONEARG(__fpclassify, _INST_fpclassify_, int, double);

extern "C" void _ORIG_sincos(double, double*, double*) {}
extern "C" void _INST_sincos(double x, double *sin, double *cos)
{
    CHECK_FPSTSW
    double rx = (FPFLAG(x) ? FPCONV(x) : x);
    _ORIG_sincos(rx, sin,cos);
}

