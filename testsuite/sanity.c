#include <stdint.h>
#include <stdio.h>

double a = 1.2, b = 3.4, c = 0.0;
int64_t d = 0;

void sanity1() {

    c = a + b;

}

void sanity2() {

    d = (int64_t)c;
    d = (int64_t)b;
    d = (int64_t)a;

}

int main() {

    sanity1();

    printf("c = %lf (should be 4.6)\n", c);

    sanity2();

    printf("d = %ld (should be 1)\n", d);
    if (b > a) { printf("test: correct\n");    }
    else       { printf("test: INCORRECT!\n"); }
    if (b < a) { printf("test: INCORRECT!\n"); }
    else       { printf("test: correct\n");    }

    return 0;
}
