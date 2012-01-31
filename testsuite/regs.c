#include "util.h"

int status = 0;

float dataf[4] = {
    2.0f, 3.0f, 0.0f, 0.0f
};

double datad[4] = {
    2.0, 3.0, 0.0, 0.0
};

void do_regs() {

    // single precision

    asm ("incl %1;" : "=m"(status) : "m"(status));
    asm ("mov $0, %%rax;" : : );

    asm ("incl %1;" : "=m"(status) : "m"(status));
    asm ("movss 0x601840(,%%rax,4), %%xmm0;" : : );

    asm ("incl %1;" : "=m"(status) : "m"(status));
    asm ("mov $1, %%rdx; " : : );

    asm ("incl %1;" : "=m"(status) : "m"(status));
    asm ("addss 0x601840(,%%rdx,4), %%xmm0;" : : );

    asm ("incl %1;" : "=m"(status) : "m"(status));
    asm ("mov $2, %%rax;" : : );

    asm ("incl %1;" : "=m"(status) : "m"(status));
    asm ("movss %%xmm0, 0x601840(,%%rax,4);" : : );

    asm ("incl %1;" : "=m"(status) : "m"(status));
    asm ("mov $0, %%rax;" : : );

    asm ("incl %1;" : "=m"(status) : "m"(status));
    asm ("movss 0x601840(,%%rax,4), %%xmm0;" : : );

    asm ("incl %1;" : "=m"(status) : "m"(status));
    asm ("mov $1, %%r8; " : : );

    asm ("incl %1;" : "=m"(status) : "m"(status));
    asm ("addss 0x601840(,%%r8,4), %%xmm0;" : : );

    asm ("incl %1;" : "=m"(status) : "m"(status));
    asm ("mov $3, %%rax;" : : );

    asm ("incl %1;" : "=m"(status) : "m"(status));
    asm ("movss %%xmm0, 0x601840(,%%rax,4);" : : );
    
    // double precision

    asm ("incl %1;" : "=m"(status) : "m"(status));
    asm ("mov $0, %%rax;" : : );

    asm ("incl %1;" : "=m"(status) : "m"(status));
    asm ("movsd 0x601860(,%%rax,8), %%xmm0;" : : );

    asm ("incl %1;" : "=m"(status) : "m"(status));
    asm ("mov $1, %%rdx; " : : );

    asm ("incl %1;" : "=m"(status) : "m"(status));
    asm ("addsd 0x601860(,%%rdx,8), %%xmm0;" : : );

    asm ("incl %1;" : "=m"(status) : "m"(status));
    asm ("mov $2, %%rax;" : : );

    asm ("incl %1;" : "=m"(status) : "m"(status));
    asm ("movsd %%xmm0, 0x601860(,%%rax,8);" : : );

    asm ("incl %1;" : "=m"(status) : "m"(status));
    asm ("mov $0, %%rax;" : : );

    asm ("incl %1;" : "=m"(status) : "m"(status));
    asm ("movsd 0x601860(,%%rax,8), %%xmm0;" : : );

    asm ("incl %1;" : "=m"(status) : "m"(status));
    asm ("mov $1, %%r8; " : : );

    asm ("incl %1;" : "=m"(status) : "m"(status));
    asm ("addsd 0x601860(,%%r8,8), %%xmm0;" : : );

    asm ("incl %1;" : "=m"(status) : "m"(status));
    asm ("mov $3, %%rax;" : : );

    asm ("incl %1;" : "=m"(status) : "m"(status));
    asm ("movsd %%xmm0, 0x601860(,%%rax,8);" : : );
}

void print_output() {
    printf("&dataf=%p\n", dataf);
    printf("&datad=%p\n", datad);
    printf("single: %g + %g = %g   ", dataf[0], dataf[1], dataf[2]);
    printf("%g + %g = %g\n", dataf[0], dataf[1], dataf[3]);
    printf("double: %g + %g = %g   ", datad[0], datad[1], datad[2]);
    printf("%g + %g = %g\n", datad[0], datad[1], datad[3]);
    printf("status=%d\n", status);
}

int check() {
    if (f_approx_equal(dataf[2], 5.0f) && f_approx_equal(dataf[3], 5.0f) &&
        d_approx_equal(datad[2], 5.0)  && d_approx_equal(datad[3], 5.0)) {
        return EXIT_SUCCESS;
    } else {
        return EXIT_FAILURE;
    }
}

int regs() {
    int status = EXIT_FAILURE;
    do_regs();
#if PRINT_OUTPUT
    printf("== regs ==\n");
    print_output();
#endif
    status = check();
    if (status == EXIT_SUCCESS) {
        printf("== regs: pass\n");
    } else {
        printf("== regs: FAIL!!!\n");
    }
    return status;
}

int main() {
    return regs();
}

