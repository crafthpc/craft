#ifndef __FPLIBC_H
#define __FPLIBC_H

#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

// check a 64-bit void* location for replacement flag
#define FPFLAG(X)  (((*(uint64_t*)(&(X))) & 0x7fffffff00000000) == 0x7ff4dead00000000)

// upcast a replaced double-precision number
#define FPCONV(X)   (double)(*(float*)(&(X)))

int _INST_fprintf(FILE* fd, const char *fmt, ...);
int _INST_printf(const char *fmt, ...);
int _INST_sprintf(char *str, const char *fmt, ...);

#endif

