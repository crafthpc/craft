#include "util.h"

#define NUM_COMPARES 16
#define CORRECT 0
#define INCORRECT 1

float fnum3 = 42.0f;
float fnum4 = 44.0f;
float fnum5 = 42.0f;
float fnum6 = 42.0f;
float finf = FINF;
float fnan = FINF / FINF;
int f_record[NUM_COMPARES];
int f_ncorrect = 0;
int f_ndone = 0;

double dnum3 = 42.0;
double dnum4 = 44.0;
double dnum5 = 42.0;
double dnum6 = 42.0;
double dinf = DINF;
double dnan = DINF / DINF;
int d_record[NUM_COMPARES];
int d_ncorrect = 0;
int d_ndone = 0;

/*
 *long double ldnum3 = 42.0L;
 *long double ldnum4 = 44.0L;
 *long double ldnum5 = 42.0L;
 *long double ldnum6 = 42.0L;
 *long double ldinf = LDINF;
 *long double ldnan = LDINF / LDINF;
 *int ld_record[NUM_COMPARES];
 *int ld_ncorrect = 0;
 *int ld_ndone = 0;
 */

void f_correct();
void f_incorrect();
void d_correct();
void d_incorrect();
/*
 *void ld_correct();
 *void ld_incorrect();
 */

void do_compare();
void print_output();
int check();
int compare();

void f_correct() {
    f_ncorrect++;
    f_record[f_ndone++] = CORRECT;
}
void f_incorrect() {
    f_record[f_ndone++] = INCORRECT;
}

void d_correct() {
    d_ncorrect++;
    d_record[d_ndone++] = CORRECT;
}
void d_incorrect() {
    d_record[d_ndone++] = INCORRECT;
}

/*
 *void ld_correct() {
 *    ld_ncorrect++;
 *    ld_record[ld_ndone++] = CORRECT;
 *}
 *void ld_incorrect() {
 *    ld_record[ld_ndone++] = INCORRECT;
 *}
 */

void do_compare() {
    // single-precision
    if (fnum3 < fnum4) { f_correct(); } else { f_incorrect(); }
    if (fnum3 > fnum4) { f_incorrect(); } else { f_correct(); }
    if (fnum3 <= fnum4) { f_correct(); } else { f_incorrect(); }
    if (fnum3 >= fnum4) { f_incorrect(); } else { f_correct(); }
    if (fnum4 < fnum3) { f_incorrect(); } else { f_correct(); }
    if (fnum4 > fnum3) { f_correct(); } else { f_incorrect(); }
    if (fnum4 <= fnum3) { f_incorrect(); } else { f_correct(); }
    if (fnum4 >= fnum3) { f_correct(); } else { f_incorrect(); }
    if (fnum5 < fnum6) { f_incorrect(); } else { f_correct(); }
    if (fnum5 <= fnum6) { f_correct(); } else { f_incorrect(); }
    if (fnum5 > fnum6) { f_incorrect(); } else { f_correct(); }
    if (fnum5 >= fnum6) { f_correct(); } else { f_incorrect(); }
    if (finf > fnum3) { f_correct(); } else { f_incorrect(); }
    if (finf < fnum3) { f_incorrect(); } else { f_correct(); }
    if (fnan > fnum3) { f_incorrect(); } else { f_correct(); }
    if (fnan < fnum3) { f_incorrect(); } else { f_correct(); }

    // double-precision
    if (dnum3 < dnum4) { d_correct(); } else { d_incorrect(); }
    if (dnum3 > dnum4) { d_incorrect(); } else { d_correct(); }
    if (dnum3 <= dnum4) { d_correct(); } else { d_incorrect(); }
    if (dnum3 >= dnum4) { d_incorrect(); } else { d_correct(); }
    if (dnum4 < dnum3) { d_incorrect(); } else { d_correct(); }
    if (dnum4 > dnum3) { d_correct(); } else { d_incorrect(); }
    if (dnum4 <= dnum3) { d_incorrect(); } else { d_correct(); }
    if (dnum4 >= dnum3) { d_correct(); } else { d_incorrect(); }
    if (dnum5 < dnum6) { d_incorrect(); } else { d_correct(); }
    if (dnum5 <= dnum6) { d_correct(); } else { d_incorrect(); }
    if (dnum5 > dnum6) { d_incorrect(); } else { d_correct(); }
    if (dnum5 >= dnum6) { d_correct(); } else { d_incorrect(); }
    if (dinf > dnum3) { d_correct(); } else { d_incorrect(); }
    if (dinf < dnum3) { d_incorrect(); } else { d_correct(); }
    if (dnan > dnum3) { d_incorrect(); } else { d_correct(); }
    if (dnan < dnum3) { d_incorrect(); } else { d_correct(); }

    // extended double-precision
    /*
     *if (ldnum3 < ldnum4) { ld_correct(); } else { ld_incorrect(); }
     *if (ldnum3 > ldnum4) { ld_incorrect(); } else { ld_correct(); }
     *if (ldnum3 <= ldnum4) { ld_correct(); } else { ld_incorrect(); }
     *if (ldnum3 >= ldnum4) { ld_incorrect(); } else { ld_correct(); }
     *if (ldnum4 < ldnum3) { ld_incorrect(); } else { ld_correct(); }
     *if (ldnum4 > ldnum3) { ld_correct(); } else { ld_incorrect(); }
     *if (ldnum4 <= ldnum3) { ld_incorrect(); } else { ld_correct(); }
     *if (ldnum4 >= ldnum3) { ld_correct(); } else { ld_incorrect(); }
     *if (ldnum5 < ldnum6) { ld_incorrect(); } else { ld_correct(); }
     *if (ldnum5 <= ldnum6) { ld_correct(); } else { ld_incorrect(); }
     *if (ldnum5 > ldnum6) { ld_incorrect(); } else { ld_correct(); }
     *if (ldnum5 >= ldnum6) { ld_correct(); } else { ld_incorrect(); }
     *if (ldinf > ldnum3) { ld_correct(); } else { ld_incorrect(); }
     *if (ldinf < ldnum3) { ld_incorrect(); } else { ld_correct(); }
     *if (ldnan > ldnum3) { ld_incorrect(); } else { ld_correct(); }
     *if (ldnan < ldnum3) { ld_incorrect(); } else { ld_correct(); }
     */
}

void print_output() {
    int i;

    // single-precision
    printf("f - inf=%f nan=%f\n", finf, fnan);
    for (i=0; i<NUM_COMPARES; i++) {
        /*if (f_record[i] != CORRECT) {*/
            printf("f - %02d: %s\n", i, (f_record[i] == CORRECT ? 
                        "correct" : "INCORRECT"));
        /*}*/
    }
    printf("f - %d comparisons: %d correct, %d incorrect\n", 
            f_ndone, f_ncorrect, f_ndone-f_ncorrect);

    // double-precision
    printf("d - inf=%lf nan=%lf\n", dinf, dnan);
    for (i=0; i<NUM_COMPARES; i++) {
        /*if (d_record[i] != CORRECT) {*/
            printf("d - %02d: %s\n", i, (d_record[i] == CORRECT ? 
                        "correct" : "INCORRECT"));
        /*}*/
    }
    printf("d - %d comparisons: %d correct, %d incorrect\n", 
            d_ndone, d_ncorrect, d_ndone-d_ncorrect);

    // extended double-precision
    /*
     *printf("ld - inf=%Lf nan=%Lf\n", ldinf, ldnan);
     *for (i=0; i<NUM_COMPARES; i++) {
     *    [>if (f_record[i] != CORRECT) {<]
     *        printf("ld - %02d: %s\n", i, (ld_record[i] == CORRECT ? 
     *                    "correct" : "INCORRECT"));
     *    [>}<]
     *}
     *printf("ld - %d comparisons: %d correct, %d incorrect\n", 
     *        ld_ndone, ld_ncorrect, ld_ndone-ld_ncorrect);
     */
}

int check() {
    int i;
    if (f_ndone == NUM_COMPARES && f_ncorrect == NUM_COMPARES &&
        d_ndone == NUM_COMPARES && d_ncorrect == NUM_COMPARES) {
        /*
         *ld_ndone == NUM_COMPARES && ld_ncorrect == NUM_COMPARES) {
         */
        return EXIT_SUCCESS;
    } else {
        for (i=0; i<NUM_COMPARES; i++) {
            if (f_record[i] != CORRECT) {
                printf("f - %02d: %s\n", i, (f_record[i] == CORRECT ? 
                            "correct" : "INCORRECT"));
            }
        }
        for (i=0; i<NUM_COMPARES; i++) {
            if (d_record[i] != CORRECT) {
                printf("d - %02d: %s\n", i, (d_record[i] == CORRECT ? 
                            "correct" : "INCORRECT"));
            }
        }
        /*
         *for (i=0; i<NUM_COMPARES; i++) {
         *    if (ld_record[i] != CORRECT) {
         *        printf("ld - %02d: %s\n", i, (ld_record[i] == CORRECT ? 
         *                    "correct" : "INCORRECT"));
         *    }
         *}
         */
        return EXIT_FAILURE;
    }
}

int compare() {
    int status = EXIT_FAILURE;
    do_compare();
#if PRINT_OUTPUT
    printf("== compare ==\n");
    print_output();
#endif
    status = check();
    if (status == EXIT_SUCCESS) {
        printf("== compare: pass\n");
    } else {
        printf("== compare: FAIL!!!\n");
    }
    return status;
}

int main() {
    return compare();
}

