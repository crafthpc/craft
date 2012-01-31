#include <math.h>

#include "util.h"

int32_t i_input = 42;
int32_t i_output_f,
        i_output_d;

int64_t l_input = 42;
int64_t l_output_f,
        l_output_d;

float f_input = 42.7f;
float f_output_d;

float f_tmp_i, f_tmp_l;
double d_tmp_i, d_tmp_l, d_tmp_f;

float f_num = 3.14f;
double d_num = 3.14;
int i_floorf, i_ceilf;
int i_floord, i_ceild;

void do_part1() {
    f_tmp_i = (float) i_input;
    f_tmp_l = (float) l_input;
    d_tmp_i = (double)i_input;
    d_tmp_l = (double)l_input;
    d_tmp_f = (double)f_input;
}

void do_part2() {
    i_output_f = (int32_t)f_tmp_i;
    i_output_d = (int32_t)d_tmp_i;
    l_output_f = (int64_t)f_tmp_l;
    l_output_d = (int64_t)d_tmp_l;
    f_output_d = (float)d_tmp_f;
}

void do_convert() {
    do_part1();
    do_part2();

    f_num *= 2.0f;  // force replacment
    d_num *= 2.0;

    i_floorf = (int)floor(f_num);
    i_floord = (int)floor(d_num);
    i_ceilf = (int)ceil(f_num);
    i_ceild = (int)ceil(d_num);
}

void print_output() {
    printf("i_f_i - %d => %f => %d\n",  i_input, f_tmp_i, i_output_f);
    printf("l_f_l - %ld => %f => %ld\n",  l_input, f_tmp_l, l_output_f);
    printf("i_d_i - %d => %lf => %d\n", i_input, d_tmp_i, i_output_d);
    printf("l_d_l - %ld => %lf => %ld\n", l_input, d_tmp_l, l_output_d);
    printf("f_d_f - %f => %lf => %f\n", f_input, d_tmp_f, f_output_d);
    printf("floor(%f) => %d  ceil(%f) => %d\n", f_num, i_floorf, f_num, i_ceilf);
    printf("floor(%lf) => %d  ceil(%lf) => %d\n", d_num, i_floord, d_num, i_ceild);
}

int check() {
    int status = EXIT_SUCCESS;
    if (i_output_f != i_input) {
        printf("ERROR: i_f_i - %d => %f => %d\n",  i_input, f_tmp_i, i_output_f);
        status = EXIT_FAILURE;
    }
    if (l_output_f != l_input) {
        printf("ERROR: l_f_l - %ld => %f => %ld\n",  l_input, f_tmp_l, l_output_f);
        status = EXIT_FAILURE;
    }
    if (i_output_d != i_input) {
        printf("ERROR: i_d_i - %d => %lf => %d\n", i_input, d_tmp_i, i_output_d);
        status = EXIT_FAILURE;
    }
    if (l_output_d != l_input) {
        printf("ERROR: l_d_l - %ld => %lf => %ld\n", l_input, d_tmp_l, l_output_d);
        status = EXIT_FAILURE;
    }
    if (f_output_d != f_input) {
        printf("ERROR: f_d_f - %f => %lf => %f\n", f_input, d_tmp_f, f_output_d);
        status = EXIT_FAILURE;
    }
    if (i_floorf != 6 || i_ceilf != 7) {
        printf("ERROR: floor(%f) => %d  ceil(%f) => %d\n", f_num, i_floorf, f_num, i_ceilf);
        status = EXIT_FAILURE;
    }
    if (i_floord != 6 || i_ceild != 7) {
        printf("ERROR: floor(%lf) => %d  ceil(%lf) => %d\n", d_num, i_floord, d_num, i_ceild);
        status = EXIT_FAILURE;
    }
    return status;
}

int convert() {
    int status = EXIT_FAILURE;
    do_convert();
#if PRINT_OUTPUT
    printf("== convert ==\n");
    print_output();
#endif
    status = check();
    if (status == EXIT_SUCCESS) {
        printf("== convert: pass\n");
    } else {
        printf("== convert: FAIL!!!\n");
    }
    return status;
}

int main() {
    return convert();
}


