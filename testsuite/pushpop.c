#include "util.h"

struct fxsave_data {
	unsigned short fcw;
	unsigned short fsw;
	unsigned short ftw;
	unsigned short fop;
	long ip;
	long cs;
	long dp;
	long ds;
	long mxcsr;
	long reserved;
	long st_space[32];	// 10 bytes for each x87 register (padded to 16 for each)
	long xmm_space[32];	// 16 bytes for each XMM register TODO: expand for 64-bit?
};

#if PRODUCE_X87_DUMPS
struct fxsave_data pre_state;
struct fxsave_data mid_state1;
struct fxsave_data mid_state2;
struct fxsave_data mid_state3;
struct fxsave_data post_state;
void dump_state(struct fxsave_data* state);
void dump_state(struct fxsave_data* state) {
    unsigned i, j, top;
    printf("fcw: RC=");
    switch ((state->fcw & 0xC00) >> 10) {
        case 0: printf("NEAR "); break;
        case 1: printf("DOWN "); break;
        case 2: printf("UP "); break;
        case 3: printf("ZERO "); break;
    }
    printf("PC=");
    switch ((state->fcw & 0x300) >> 8) {
        case 0: printf("FLT "); break;
        case 1: printf("N/A "); break;
        case 2: printf("DBL "); break;
        case 3: printf("LDBL "); break;
    }
    if (state->fcw & 0x1000) printf("[Y] ");
    if (state->fcw & 0x20)   printf("[PM] ");
    if (state->fcw & 0x10)   printf("[UM] ");
    if (state->fcw & 0x8)    printf("[OM] ");
    if (state->fcw & 0x4)    printf("[ZM] ");
    if (state->fcw & 0x2)    printf("[DM] ");
    if (state->fcw & 0x1)    printf("[IM] ");
    top = state->fsw & 0x3800;
    top >>= 11;
    printf("\nfsw: TOP=%u ", (state->fsw & 0x3800) >> 11);
    if (state->fsw & 0x8000) printf("[B] ");
    if (state->fsw & 0x4000) printf("[C3] ");
    if (state->fsw & 0x400)  printf("[C2] ");
    if (state->fsw & 0x200)  printf("[C1] ");
    if (state->fsw & 0x100)  printf("[C0] ");
    if (state->fsw & 0x80)   printf("[ES] ");
    if (state->fsw & 0x40)   printf("[SF] ");
    if (state->fsw & 0x20)   printf("[PE] ");
    if (state->fsw & 0x10)   printf("[UE] ");
    if (state->fsw & 0x8)    printf("[OE] ");
    if (state->fsw & 0x4)    printf("[ZE] ");
    if (state->fsw & 0x2)    printf("[DE] ");
    if (state->fsw & 0x1)    printf("[IE] ");
    printf("\nftw: ");
    if (state->ftw & 0x1)    printf("[FPR0] ");
    if (state->ftw & 0x2)    printf("[FPR1] ");
    if (state->ftw & 0x4)    printf("[FPR2] ");   if (state->ftw & 0x8)    printf("[FPR3] ");
    if (state->ftw & 0x10)   printf("[FPR4] ");
    if (state->ftw & 0x20)   printf("[FPR5] ");
    if (state->ftw & 0x40)   printf("[FPR6] ");
    if (state->ftw & 0x80)   printf("[FPR7] ");
    printf("\n");
	for (i=0; i<4; i++) {
	    j = i*4;
	    printf("| MMX%u: %04x %08x %08x  (%8.2Le)\n", i,
		    (uint32_t)(state->st_space[j+2]) & 0xFFFF,
		    (uint32_t)state->st_space[j+1],
		    (uint32_t)state->st_space[j+0],
		    *(long double*)(&state->st_space[j+0]));
	}
    for (i=0; i<3; i++) {
       j = i*4;
       printf("| XMM%u: %08x %08x %08x %08x (%8.2le, %8.2le)\n"
             "|      (%8.2e, %8.2e, %8.2e, %8.2e)\n", i,
              (uint32_t)state->xmm_space[j+3],
              (uint32_t)state->xmm_space[j+2],
              (uint32_t)state->xmm_space[j+1],
              (uint32_t)state->xmm_space[j+0],
              *(double*)(&state->xmm_space[j+2]),
              *(double*)(&state->xmm_space[j+0]),
              *(float*)(&state->xmm_space[j+3]),
              *(float*)(&state->xmm_space[j+2]),
              *(float*)(&state->xmm_space[j+1]),
              *(float*)(&state->xmm_space[j+0]));
    }
}
#endif

float fvalue1 = 1234.5678f;
float fvalue2 = 1337.1337f;
float fresult1, fresult2;

double dvalue1 = 1234.5678;
double dvalue2 = 1337.1337;
double dresult1, dresult2;

long double ldvalue1 = 1234.5678L;
long double ldvalue2 = 1337.1337L;
long double ldresult1, ldresult2;

void do_pushpop();
void print_output();
int check();
int pushpop();

void do_pushpop() {
    // single-precision
    __asm__ ("fld %0;" : : "m" (fvalue1));
    __asm__ ("fld %0;" : : "m" (fvalue2));
    __asm__ ("fstp %0;" : : "m" (fresult2));
    __asm__ ("fstp %0;" : : "m" (fresult1));

    // double-precision (detailed)
#if PRODUCE_X87_DUMPS
    __asm__ ("fxsave %0;" : : "m" (pre_state));
#endif
    __asm__ ("fldl %0;" : : "m" (dvalue1));
#if PRODUCE_X87_DUMPS
    __asm__ ("fxsave %0;" : : "m" (mid_state1));
#endif
    __asm__ ("fldl %0;" : : "m" (dvalue2));
#if PRODUCE_X87_DUMPS
    __asm__ ("fxsave %0;" : : "m" (mid_state2));
#endif
    __asm__ ("fstpl %0;" : : "m" (dresult2));
#if PRODUCE_X87_DUMPS
    __asm__ ("fxsave %0;" : : "m" (mid_state3));
#endif
    __asm__ ("fstpl %0;" : : "m" (dresult1));
#if PRODUCE_X87_DUMPS
    __asm__ ("fxsave %0;" : : "m" (post_state));
#endif

    // extended double-precision
    __asm__ ("fldt %0;" : : "m" (ldvalue1));
    __asm__ ("fldt %0;" : : "m" (ldvalue2));
    __asm__ ("fstpt %0;" : : "m" (ldresult2));
    __asm__ ("fstpt %0;" : : "m" (ldresult1));
}

void print_output() {
    /*printf("f - pushed %f\n", fvalue1);*/
    /*printf("f - pushed %f\n", fvalue2);*/
    /*printf("f - popped %f\n", fresult2);*/
    /*printf("f - popped %f\n", fresult1);*/
#if PRODUCE_X87_DUMPS
    dump_state(&pre_state);
#endif
    /*printf("d - pushed %lf\n", dvalue1);*/
#if PRODUCE_X87_DUMPS
    dump_state(&mid_state1);
#endif
    /*printf("d - pushed %lf\n", dvalue2);*/
#if PRODUCE_X87_DUMPS
    dump_state(&mid_state2);
#endif
    /*printf("d - popped %lf\n", dresult2);*/
#if PRODUCE_X87_DUMPS
    dump_state(&mid_state3);
#endif
    /*printf("d - popped %lf\n", dresult1);*/
#if PRODUCE_X87_DUMPS
    dump_state(&post_state);
#endif
    /*printf("ld - pushed %Lf\n", ldvalue1);*/
    /*printf("ld - pushed %Lf\n", ldvalue2);*/
    /*printf("ld - popped %Lf\n", ldresult2);*/
    /*printf("ld - popped %Lf\n", ldresult1);*/
}

int check() {
    if (fvalue1 == fresult1 && fvalue2 == fresult2 &&
        dvalue1 == dresult1 && dvalue2 == dresult2 &&
        ldvalue1 == ldresult1 && ldvalue2 == ldresult2) {
        return EXIT_SUCCESS;
    } else {
        return EXIT_FAILURE;
    }
}

int pushpop() {
    int status = EXIT_FAILURE;
    do_pushpop();
/*#if PRINT_OUTPUT*/
    printf("== pushpop ==\n");
    print_output();
/*#endif*/
    status = check();
    if (status == EXIT_SUCCESS) {
       printf("== pushpop: pass\n");
    } else {
       printf("== pushpop: FAIL!!!\n");
    }
    return status;
}

int main() {
    return pushpop();
}

