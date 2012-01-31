#include <stdint.h>
#include <stdio.h>

double a = 1.2, b = 3.4, c = 0.0;
int64_t d = 0;

void sanity() {

    c = a + b;
    printf("c = %lf (should be 4.6)\n", c);

    d = (int64_t)c;
    d = (int64_t)b;
    d = (int64_t)a;
    printf("d = %ld (should be 1)\n", d);

    if (b > a) { printf("test: correct\n");    }
    else       { printf("test: INCORRECT!\n"); }
    if (b < a) { printf("test: INCORRECT!\n"); }
    else       { printf("test: correct\n");    }
}

int main() {
    sanity();
    return 0;
}
