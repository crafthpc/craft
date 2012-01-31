#include "util.h"

float  fglobal = 3.0f;
double dglobal = 3.0;

float  fglobal_vec[3] = { 3.0f, 5.0f, 7.0f };
double dglobal_vec[3] = { 3.0 , 5.0 , 7.0  };

float  *fheap = NULL;
double *dheap = NULL;

void do_modes();
void print_output();
int check();
int modes();

void do_modes() {
    float  flocal = 5.0f;
    double dlocal = 5.0;

    // single-precision
    fheap = (float*)malloc(sizeof(float));
    fglobal = fglobal * flocal;
    *fheap = fglobal_vec[2] * flocal;

    
    // double-precision
    dheap = (double*)malloc(sizeof(double));
    dglobal = dglobal * dlocal;
    *dheap = dglobal_vec[2] * dlocal;
}


void print_output() {
    printf("fglobal = %f\n",  fglobal);
    printf("fheap   = %f\n",  *fheap);
    printf("dglobal = %lf\n", dglobal);
    printf("dheap   = %lf\n", *dheap);
}

int check() {
    if (f_approx_equal(fglobal,15.0f) && f_approx_equal(*fheap,35.0f) &&
        d_approx_equal(dglobal,15.0) && d_approx_equal(*dheap,35.0)) {
        return EXIT_SUCCESS;
    } else {
        return EXIT_FAILURE;
    }
}

int modes() {
    int status = EXIT_FAILURE;
    do_modes();
#if PRINT_OUTPUT
    printf("== modes ==\n");
    print_output();
#endif
    status = check();
    if (status == EXIT_SUCCESS) {
        printf("== modes: pass\n");
    } else {
        printf("== modes: FAIL!!!\n");
    }
    return status;
}

int main() {
    return modes();
}

