#ifndef __FPFLAG_H
#define __FPFLAG_H

#include <stdio.h>

void _INST_print_flags(const char *tag, unsigned flags);

extern long _INST_DISABLED;
extern long _INST_INACTIVE;
extern long _INST_ACTIVE;
extern long _INST_status;

#endif

