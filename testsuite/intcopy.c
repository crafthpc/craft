#include "util.h"

float fnum = 123.456f;
float fresult;
double dnum = 123.456;
double dresult;
/*
 *long double ldnum = 123.456L;
 *long double ldresult;
 */

unsigned dump[64];

void copy(unsigned *from, unsigned *to, size_t size);
void do_intcopy();
void print_output();
int check();
int intcopy();

void copy(unsigned *from, unsigned *to, size_t size) {
    unsigned i, words;
    unsigned *tmp = dump;
    words = size/INT_SIZE;
    for (i=0; i<words; i++) {
        *tmp++ = *from++;
    }
    tmp = dump;
    for (i=0; i<words; i++) {
        *to++ = *tmp++;
    }
}

void do_intcopy() {
    copy((unsigned*)(&fnum), (unsigned*)(&fresult), sizeof(fnum));
    copy((unsigned*)(&dnum), (unsigned*)(&dresult), sizeof(dnum));
    /*
     *copy((unsigned*)(&ldnum), (unsigned*)(&ldresult), sizeof(ldnum));
     */
}

void print_output() {
    printf("f - %f => %f\n", fnum, fresult);
    printf("d - %lf => %lf\n", dnum, dresult);
    /*
     *printf("ld - %Lf => %Lf\n", ldnum, ldresult);
     */
}

int check() {
    int status = EXIT_SUCCESS;
    if (fnum != fresult) {
        printf("f - %f => %f\n", fnum, fresult);
        status = EXIT_FAILURE;
    }
    if (dnum != dresult) {
        printf("d - %lf => %lf\n", dnum, dresult);
        status = EXIT_FAILURE;
    }
    /*
     *if (ldnum != ldresult) {
     *    printf("ld - %Lf => %Lf\n", ldnum, ldresult);
     *    status = EXIT_FAILURE;
     *}
     */
    return status;
}

int intcopy() {
    int status = EXIT_FAILURE;
    do_intcopy();
#if PRINT_OUTPUT
    printf("== intcopy ==\n");
    print_output();
#endif
    status = check();
    if (status == EXIT_SUCCESS) {
        printf("== intcopy: pass\n");
    } else {
        printf("== intcopy: FAIL!!!\n");
    }
    return status;
}

int main() {
    return intcopy();
}

