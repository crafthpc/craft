#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
    int i;
    double d, e, acc;
    double init = 1e9;

    srand(42);
    acc = init;

    for (i=0; i<10000; i++) {
        d = (double)rand()/(double)INT_MAX;
        e = (double)rand()/(double)INT_MAX;
        d = (e * e) + (d * 6.0);
        acc += d;
    }

    printf("  RESULT: %e", acc);
    if (acc == init) {
        printf("  FAILED!!!\n");
    } else {
        printf("  SUCCESSFUL!\n");
    }
    return 0;
}

