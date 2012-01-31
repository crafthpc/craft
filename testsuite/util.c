#include "util.h"

int f_approx_equal(float x, float y) {
    /*printf("f_approx_equal (%g, %g) [0x%x, 0x%x]\n",*/
            /*x, y, *(uint32_t*)(&x), *(uint32_t*)(&y));*/
    float err = ABSF(ABSF(x - y) / MAX(ABSF(x),ABSF(y)));
    int eq = /*finite(err) &&*/ ((x == y) || (err < 0.0001f));
    /*printf("f_approx_equal: rel_error=%g\n", err);*/
    return eq;
}

int d_approx_equal(double x, double y) {
    /*printf("d_approx_equal (%g, %g) [0x%lx, 0x%lx]\n",*/
            /*x, y, *(uint64_t*)(&x), *(uint64_t*)(&y));*/
    double err = ABS(ABS(x - y) / MAX(ABS(x),ABS(y)));
    /*double err = fabs((x - y) / x);*/
    /*printf("d_approx_equal (%g, %g) [0x%lx, 0x%lx]  err=%g\n", */
            /*x, y, *(uint64_t*)(&x), *(uint64_t*)(&y), err);*/
    int eq = /*finite(err) &&*/ ((x == y) || (err < 0.0001));
    /*printf("d_approx_equal (%g, %g) [0x%lx, 0x%lx]  err=%g eq=%d\n",*/
            /*x, y, *(uint64_t*)(&x), *(uint64_t*)(&y), err, eq);*/
    return eq;
}

#if 0
int ld_approx_equal(long double x, long double y) {
    long double err = ABSL(ABSL(x - y) / MAX(ABSL(x),ABSL(y)));
    int eq = isfinite(err) && ((x == y) || (err < 0.0001L));
    /*printf("ld_approx_equal: rel_error=%Lg\n", err);*/
    return eq;
}
#endif

