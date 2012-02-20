#include "FPContext.h"

namespace FPInst {

string FPContext::FPReg2Str(FPRegister reg)
{
    switch (reg) {
        case REG_EIP:    return "eip";
        case REG_EFLAGS: return "eflags";
        case REG_EAX: return "eax";
        case REG_EBX: return "ebx";
        case REG_ECX: return "ecx";
        case REG_EDX: return "edx";
        case REG_ESP: return "esp";
        case REG_EBP: return "ebp";
        case REG_ESI: return "esi";
        case REG_EDI: return "edi";
        case REG_E8:  return  "e8";
        case REG_E9:  return  "e9";
        case REG_E10: return "e10";
        case REG_E11: return "e11";
        case REG_E12: return "e12";
        case REG_E13: return "e13";
        case REG_E14: return "e14";
        case REG_E15: return "e15";
        case REG_ST0: return "st0";
        case REG_ST1: return "st1";
        case REG_ST2: return "st2";
        case REG_ST3: return "st3";
        case REG_ST4: return "st4";
        case REG_ST5: return "st5";
        case REG_ST6: return "st6";
        case REG_ST7: return "st7";
        case REG_XMM0:  return  "xmm0";
        case REG_XMM1:  return  "xmm1";
        case REG_XMM2:  return  "xmm2";
        case REG_XMM3:  return  "xmm3";
        case REG_XMM4:  return  "xmm4";
        case REG_XMM5:  return  "xmm5";
        case REG_XMM6:  return  "xmm6";
        case REG_XMM7:  return  "xmm7";
        case REG_XMM8:  return  "xmm8";
        case REG_XMM9:  return  "xmm9";
        case REG_XMM10: return "xmm10";
        case REG_XMM11: return "xmm11";
        case REG_XMM12: return "xmm12";
        case REG_XMM13: return "xmm13";
        case REG_XMM14: return "xmm14";
        case REG_XMM15: return "xmm15";
        case REG_CS:  return "cs";
        case REG_DS:  return "ds";
        case REG_ES:  return "es";
        case REG_FS:  return "fs";
        case REG_GS:  return "gs";
        case REG_SS:  return "ss";
        default: return "none";
    }
}


FPContext::FPContext()
{
    unsigned long offset;
    reg_eip = reg_eflags = 0;
    reg_eax = reg_ebx = reg_ecx = reg_edx = reg_esp = reg_ebp = reg_esi = reg_edi = 0;
    reg_r8  = reg_r9  = reg_r10 = reg_r11 = reg_r12 = reg_r13 = reg_r14 = reg_r15 = 0;
    reg_st0 = reg_st1 = reg_st2 = reg_st3 = reg_st4 = reg_st5 = reg_st6 = reg_st7 = 0.0L;
    highestCachedX87 = -1;
    cachedXMM = -1;

    queuePushes = 0;
    queuePops   = 0;
    rqueueCount = 0;
    mqueueCount = 0;

    // allocate extra space and do manual alignment (enabling optimizations 
    // causes GCC to ignore alignments)
    fxsave_state_buffer = (char*)malloc(FXSAVE_DATA_SIZE+16);
    if (!fxsave_state_buffer) {
        fprintf(stderr, "Error: Out of memory!\n");
        abort();
    }
    offset = (unsigned long)fxsave_state_buffer % 16;
    fxsave_state = (fxsave_data*)((unsigned long)fxsave_state_buffer+offset);
    //printf("allocated at 0x%p - true structure at 0x%p (offset %lu)\n", 
            //fxsave_state_buffer, fxsave_state, offset);
}

void FPContext::updateIP(unsigned long eip)
{
    reg_eip = eip;
}

void FPContext::updateGPRs(unsigned long eax, unsigned long ebx,
        unsigned long ecx, unsigned long edx, unsigned long esp,
        unsigned long ebp, unsigned long esi, unsigned long edi)
{
    //printf("eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x esp=0x%x ebp=0x%x esi=0x%x edi=0x%x\n",
            //eax, ebx, ecx, edx, esp, ebp, esi, edi);
    reg_eax = eax;
    reg_ebx = ebx;
    reg_ecx = ecx;
    reg_edx = edx;
    reg_esp = esp;
    reg_ebp = ebp;
    reg_esi = esi;
    reg_edi = edi;
}

void FPContext::saveAllFPR()
{
    __asm__ ("fxsave %0;" : : "m" (*fxsave_state));
}

void FPContext::restoreAllFPR()
{
    __asm__ ("fxrstor %0;" : : "m" (*fxsave_state));
}

void FPContext::clearRegisterCaches()
{
    highestCachedX87 = -1;
    cachedXMM = -1;
}

void FPContext::resetQueuedActions()
{
    queuePushes = 0;
    queuePops   = 0;
    rqueueCount = 0;
    mqueueCount = 0;
    //printf("cleared action queue\n");
}

void FPContext::finalizeQueuedActions()
{
    unsigned i;
    for (i=0; i<queuePushes; i++) {
        pushX87Stack();
    }
    queuePushes = 0;
    for (i=0; i<rqueueCount; i++) {
        switch (rqueueItem[i].size) {
            // currently no support for 8- or 16-bit registers
            //case 8:  setRegisterValueUInt8 (rqueueItem[i].reg, 
                             //rqueueItem[i].tag, rqueueItem[i].data.uint8, false); break;
            //case 16: setRegisterValueUInt16(rqueueItem[i].reg, 
                             //rqueueItem[i].tag, rqueueItem[i].data.uint16, false); break;
            case 32: setRegisterValueUInt32(rqueueItem[i].reg, 
                             rqueueItem[i].tag, rqueueItem[i].val.uint32, false); break;
            case 64: setRegisterValueUInt64(rqueueItem[i].reg, 
                             rqueueItem[i].tag, rqueueItem[i].val.uint64, false); break;
        }
    }
    rqueueCount = 0;
    for (i=0; i<mqueueCount; i++) {
        switch (mqueueItem[i].size) {
            case 8:  setMemoryValueUInt8 (mqueueItem[i].dest, 
                             mqueueItem[i].val.uint8, false); break;
            case 16: setMemoryValueUInt16(mqueueItem[i].dest, 
                             mqueueItem[i].val.uint16, false); break;
            case 32: setMemoryValueUInt32(mqueueItem[i].dest, 
                             mqueueItem[i].val.uint32, false); break;
            case 64: setMemoryValueUInt64(mqueueItem[i].dest, 
                             mqueueItem[i].val.uint64, false); break;
        }
    }
    mqueueCount = 0;
    for (i=0; i<queuePops; i++) {
        popX87Stack();
    }
    queuePops = 0;
}

void FPContext::getRegisterInt(unsigned long *dest, FPRegister reg)
{
    switch (reg) {
        case REG_EIP: *(dest) = reg_eip; break;
        case REG_EFLAGS: *(dest) = reg_eflags; break;
        case REG_EAX: *(dest) = reg_eax; break;
        case REG_EBX: *(dest) = reg_ebx; break;
        case REG_ECX: *(dest) = reg_ecx; break;
        case REG_EDX: *(dest) = reg_edx; break;
        case REG_ESP: *(dest) = reg_esp; break;
        case REG_EBP: *(dest) = reg_ebp; break;
        case REG_ESI: *(dest) = reg_esi; break;
        case REG_EDI: *(dest) = reg_edi; break;
        case REG_E8:  *(dest) = reg_r8;  break;
        case REG_E9:  *(dest) = reg_r9;  break;
        case REG_E10: *(dest) = reg_r10; break;
        case REG_E11: *(dest) = reg_r11; break;
        case REG_E12: *(dest) = reg_r12; break;
        case REG_E13: *(dest) = reg_r13; break;
        case REG_E14: *(dest) = reg_r14; break;
        case REG_E15: *(dest) = reg_r15; break;
        default: assert(0);
    }
}

void FPContext::getRegisterValue(void *dest, FPRegister reg) 
{
    getRegisterValue(dest, reg, 0, sizeof(unsigned long));
}

void FPContext::getRegisterValue(void *dest, FPRegister reg, long tag, size_t size)
{
    switch (reg) {
        case REG_EIP: *((unsigned long*)dest) = reg_eip; break;
        case REG_EFLAGS: *((unsigned long*)dest) = reg_eflags; break;
        case REG_EAX: *((unsigned long*)dest) = reg_eax; break;
        case REG_EBX: *((unsigned long*)dest) = reg_ebx; break;
        case REG_ECX: *((unsigned long*)dest) = reg_ecx; break;
        case REG_EDX: *((unsigned long*)dest) = reg_edx; break;
        case REG_ESP: *((unsigned long*)dest) = reg_esp; break;
        case REG_EBP: *((unsigned long*)dest) = reg_ebp; break;
        case REG_ESI: *((unsigned long*)dest) = reg_esi; break;
        case REG_EDI: *((unsigned long*)dest) = reg_edi; break;
        case REG_E8:  *((unsigned long*)dest) = reg_r8;  break;
        case REG_E9:  *((unsigned long*)dest) = reg_r9;  break;
        case REG_E10: *((unsigned long*)dest) = reg_r10; break;
        case REG_E11: *((unsigned long*)dest) = reg_r11; break;
        case REG_E12: *((unsigned long*)dest) = reg_r12; break;
        case REG_E13: *((unsigned long*)dest) = reg_r13; break;
        case REG_E14: *((unsigned long*)dest) = reg_r14; break;
        case REG_E15: *((unsigned long*)dest) = reg_r15; break;
        case REG_ST0: *(long double*)dest = * (long double*) &(fxsave_state->st_space[ 0]); break;
        case REG_ST1: *(long double*)dest = * (long double*) &(fxsave_state->st_space[ 4]); break;
        case REG_ST2: *(long double*)dest = * (long double*) &(fxsave_state->st_space[ 8]); break;
        case REG_ST3: *(long double*)dest = * (long double*) &(fxsave_state->st_space[12]); break;
        case REG_ST4: *(long double*)dest = * (long double*) &(fxsave_state->st_space[16]); break;
        case REG_ST5: *(long double*)dest = * (long double*) &(fxsave_state->st_space[20]); break;
        case REG_ST6: *(long double*)dest = * (long double*) &(fxsave_state->st_space[24]); break;
        case REG_ST7: *(long double*)dest = * (long double*) &(fxsave_state->st_space[28]); break;
        case REG_XMM0:  memcpy(dest, (void*) &(fxsave_state->xmm_space[ 0+tag]),  size); break;
        case REG_XMM1:  memcpy(dest, (void*) &(fxsave_state->xmm_space[ 4+tag]),  size); break;
        case REG_XMM2:  memcpy(dest, (void*) &(fxsave_state->xmm_space[ 8+tag]),  size); break;
        case REG_XMM3:  memcpy(dest, (void*) &(fxsave_state->xmm_space[12+tag]),  size); break;
        case REG_XMM4:  memcpy(dest, (void*) &(fxsave_state->xmm_space[16+tag]),  size); break;
        case REG_XMM5:  memcpy(dest, (void*) &(fxsave_state->xmm_space[20+tag]),  size); break;
        case REG_XMM6:  memcpy(dest, (void*) &(fxsave_state->xmm_space[24+tag]),  size); break;
        case REG_XMM7:  memcpy(dest, (void*) &(fxsave_state->xmm_space[28+tag]),  size); break;
        case REG_XMM8:  memcpy(dest, (void*) &(fxsave_state->xmm_space[32+tag]),  size); break;
        case REG_XMM9:  memcpy(dest, (void*) &(fxsave_state->xmm_space[36+tag]),  size); break;
        case REG_XMM10: memcpy(dest, (void*) &(fxsave_state->xmm_space[40+tag]),  size); break;
        case REG_XMM11: memcpy(dest, (void*) &(fxsave_state->xmm_space[44+tag]),  size); break;
        case REG_XMM12: memcpy(dest, (void*) &(fxsave_state->xmm_space[48+tag]),  size); break;
        case REG_XMM13: memcpy(dest, (void*) &(fxsave_state->xmm_space[52+tag]),  size); break;
        case REG_XMM14: memcpy(dest, (void*) &(fxsave_state->xmm_space[60+tag]),  size); break;
        case REG_XMM15: memcpy(dest, (void*) &(fxsave_state->xmm_space[64+tag]),  size); break;
        case REG_NONE: *((unsigned long*)dest) = 0; break;
        default: assert(0);
    }
    //printf("getRegisterValue(%s) = %lu\n", FPReg2Str(reg).c_str(), *((unsigned long*)dest));
}

#if 0 // OLD METHOD OF MANUALLY EXTRACTING REGISTER VALUES
void FPContext::getRegisterValue(void *dest, FPRegister reg, long tag, long size)
{
    //float *num;
    switch (reg) {
        case REG_EAX: *((unsigned long *)dest) = reg_eax; break;
        case REG_EBX: *((unsigned long *)dest) = reg_ebx; break;
        case REG_ECX: *((unsigned long *)dest) = reg_ecx; break;
        case REG_EDX: *((unsigned long *)dest) = reg_edx; break;
        case REG_ESP: *((unsigned long *)dest) = reg_esp; break;
        case REG_EBP: *((unsigned long *)dest) = reg_ebp; break;
        case REG_ESI: *((unsigned long *)dest) = reg_esi; break;
        case REG_EDI: *((unsigned long *)dest) = reg_edi; break;
        case REG_ST0: if (highestCachedX87 >= 0) { *(long double*)dest = reg_st0; }
                      else { __asm__ ("fstpt %0;" "fldt %0;" : : "m" (reg_st0));
                      *(long double*)dest = reg_st0; highestCachedX87 = 0; } break;
        case REG_ST1: if (highestCachedX87 >= 1) { *(long double*)dest = reg_st1; }
                      else { __asm__ ("fstpt %0;" "fstpt %1;" "fldt %1;" "fldt %0;"
                              : : "m" (reg_st0), "m" (reg_st1));
                      *(long double*)dest = reg_st1; highestCachedX87 = 1; } break;
        case REG_ST2: if (highestCachedX87 >= 2) { *(long double*)dest = reg_st2; }
                      else { __asm__ ("fstpt %0;" "fstpt %1;" "fstpt %2;"
                                      "fldt %2;" "fldt %1;" "fldt %0;"
                             : : "m" (reg_st0), "m" (reg_st1), "m" (reg_st2)); 
                      *(long double*)dest = reg_st2; highestCachedX87 = 2; } break;
        case REG_ST3: if (highestCachedX87 >= 3) { *(long double*)dest = reg_st3; }
                      else { __asm__ ("fstpt %0;" "fstpt %1;" "fstpt %2;" "fstpt %3;"
                                      "fldt %3;" "fldt %2;" "fldt %1;" "fldt %0;"
                             : : "m" (reg_st0), "m" (reg_st1), "m" (reg_st2), "m" (reg_st3)); 
                      *(long double*)dest = reg_st3; highestCachedX87 = 3; } break;
        case REG_ST4: if (highestCachedX87 >= 4) { *(long double*)dest = reg_st4; }
                      else { __asm__ ("fstpt %0;" "fstpt %1;" "fstpt %2;" "fstpt %3;"
                                      "fstpt %4;" "fldt %4;" 
                                      "fldt %3;" "fldt %2;" "fldt %1;" "fldt %0;"
                             : : "m" (reg_st0), "m" (reg_st1), "m" (reg_st2), "m" (reg_st3),
                                 "m" (reg_st4)); 
                      *(long double*)dest = reg_st4; highestCachedX87 = 4; } break;
        // these donn't work for some reason... (alignment issues?)
        /*case REG_ST5: if (highestCachedX87 >= 5) { *(long double*)dest = reg_st5; }
                      else { __asm__ ("fstpt %0;" "fstpt %1;" "fstpt %2;" "fstpt %3;"
                                      "fstpt %4;" "fstpt %5;" "fldt %5;" "fldt %4;"
                                      "fldt %3;" "fldt %2;" "fldt %1;" "fldt %0;"
                             : : "m" (reg_st0), "m" (reg_st1), "m" (reg_st2), "m" (reg_st3),
                                 "m" (reg_st4), "m" (reg_st5)); 
                      *(long double*)dest = reg_st5; highestCachedX87 = 5; } break;*/
        /*case REG_ST6: if (highestCachedX87 >= 6) { *(long double*)dest = reg_st6; }
                      else { __asm__ ("fstpt %0;" "fstpt %1;" "fstpt %2;" "fstpt %3;"
                                      "fstpt %4;" "fstpt %5;" "fstpt %6;"
                                      "fldt %6;" "fldt %5;" "fldt %4;"
                                      "fldt %3;" "fldt %2;" "fldt %1;" "fldt %0;"
                             : : "m" (reg_st0), "m" (reg_st1), "m" (reg_st2), "m" (reg_st3),
                                 "m" (reg_st4), "m" (reg_st5), "m" (reg_st6)); 
                      *(long double*)dest = reg_st6; highestCachedX87 = 6; } break;*/
        /*case REG_ST7: if (highestCachedX87 >= 7) { *(long double*)dest = reg_st7; }
                      else { __asm__ ("fstpt %0;" "fstpt %1;" "fstpt %2;" "fstpt %3;"
                                      "fstpt %4;" "fstpt %5;" "fstpt %6;" "fstpt %7;"
                                      "fldt %7;" "fldt %6;" "fldt %5;" "fldt %4;"
                                      "fldt %3;" "fldt %2;" "fldt %1;" "fldt %0;"
                             : : "m" (reg_st0), "m" (reg_st1), "m" (reg_st2), "m" (reg_st3),
                                 "m" (reg_st4), "m" (reg_st5), "m" (reg_st6), "m" (reg_st7)); 
                      *(long double*)dest = reg_st7; highestCachedX87 = 7; } break;*/
        case REG_XMM0: if (cachedXMM != 0) { __asm__ ("movupd %%xmm0,%0" : : "m" (xmmTemp)); cachedXMM = 0; }
                       //for (long k=0; k<16; k++) {
                           //printf("%02x ", ((unsigned char*)&xmmTemp)[k]);
                       //}
                       //printf("\n");
                       //num = (float*)&xmmTemp;
                       //printf("%g %g %g %g\n", ((float*)xmmTemp)[0], ((float*)xmmTemp)[1], ((float*)xmmTemp)[2], ((float*)xmmTemp)[3]);
                       //printf("%g %g %g %g\n", num[0], num[1], num[2], num[3]);
                       //printf("%g %g\n", ((double*)xmmTemp)[0], ((double*)xmmTemp)[1]);
                       memcpy(dest, (void*)(((unsigned long)&xmmTemp)+tag*4),  size); break;
        case REG_XMM1: if (cachedXMM != 1) { __asm__ ("movupd %%xmm1,%0" : : "m" (xmmTemp)); cachedXMM = 1; }
                       memcpy(dest, (void*)(((unsigned long)&xmmTemp)+tag*4),  size); break;
        case REG_XMM2: if (cachedXMM != 2) { __asm__ ("movupd %%xmm2,%0" : : "m" (xmmTemp)); cachedXMM = 2; }
                       memcpy(dest, (void*)(((unsigned long)&xmmTemp)+tag*4),  size); break;
        case REG_XMM3: if (cachedXMM != 3) { __asm__ ("movupd %%xmm3,%0" : : "m" (xmmTemp)); cachedXMM = 3; }
                       memcpy(dest, (void*)(((unsigned long)&xmmTemp)+tag*4),  size); break;
        case REG_XMM4: if (cachedXMM != 4) { __asm__ ("movupd %%xmm4,%0" : : "m" (xmmTemp)); cachedXMM = 4; }
                       memcpy(dest, (void*)(((unsigned long)&xmmTemp)+tag*4),  size); break;
        case REG_XMM5: if (cachedXMM != 5) { __asm__ ("movupd %%xmm5,%0" : : "m" (xmmTemp)); cachedXMM = 5; }
                       memcpy(dest, (void*)(((unsigned long)&xmmTemp)+tag*4),  size); break;
        case REG_XMM6: if (cachedXMM != 6) { __asm__ ("movupd %%xmm6,%0" : : "m" (xmmTemp)); cachedXMM = 6; }
                       memcpy(dest, (void*)(((unsigned long)&xmmTemp)+tag*4),  size); break;
        case REG_XMM7: if (cachedXMM != 7) { __asm__ ("movupd %%xmm7,%0" : : "m" (xmmTemp)); cachedXMM = 7; }
                       memcpy(dest, (void*)(((unsigned long)&xmmTemp)+tag*4),  size); break;
        case REG_NONE: *((unsigned long *)dest) = 0; break;
        default: assert(0);
    }
}
#endif

void FPContext::getMemoryAddress(void **addr, FPRegister base, FPRegister index, long disp, long scale, size_t /*size*/)
{
    unsigned long baseVal=0, indexVal=0;
    if (base != REG_NONE) {
        getRegisterInt(&baseVal, base);
    }
    if (index != REG_NONE) {
        getRegisterInt(&indexVal, index);
    }
    *addr = (void*)(unsigned long)((long)baseVal+((long)indexVal*(long)scale)+(long)disp);
    //printf("getMemoryAddress  0x%lx [%s] + 0x%lx [%s] * %ld + 0x%lx = 0x%p\n", 
            //baseVal, FPReg2Str(base).c_str(), indexVal, FPReg2Str(index).c_str(), scale, disp, *addr);
}

void FPContext::getMemoryValue(void *dest, void *addr, size_t size)
{
    switch (size) {
        case 4:  *(float*)dest       = *(float*)addr;       break;
        case 8:  *(double*)dest      = *(double*)addr;      break;
        case 12: *(long double*)dest = *(long double*)addr; break;
        default: memcpy(dest, addr, size); break;
    }
    //printf("getMemoryValue  addr: 0x%p size=%lu   float: %g\n", 
            //addr, (unsigned long)size, *(float*)(addr));
}

void FPContext::getMemoryValue(void *dest, void **addr, FPRegister base, FPRegister index, long disp, long scale, size_t size)
{
    unsigned long baseVal=0, indexVal=0;
    if (base != REG_NONE) {
        getRegisterInt(&baseVal, base);
    }
    if (index != REG_NONE) {
        getRegisterInt(&indexVal, index);
    }
    *addr = (void*)(unsigned long)((long)baseVal+((long)indexVal*(long)scale)+(long)disp);
    //printf("getMemoryValue  0x%lx [%s] + 0x%lx [%s] * %ld + 0x%lx = %p\n", 
            //baseVal, FPReg2Str(base).c_str(), indexVal, FPReg2Str(index).c_str(), scale, disp, addr);
    memcpy(dest, *addr, size);
}

void FPContext::setRegisterValueUInt8(FPRegister reg, uint8_t value, bool queue)
{
    setRegisterValueUInt32(reg, 0, (uint32_t)value, queue);
}

void FPContext::setRegisterValueUInt8(FPRegister reg, long tag, uint8_t value, bool queue)
{
    setRegisterValueUInt32(reg, tag, (uint32_t)value, queue);
}

void FPContext::setRegisterValueUInt16(FPRegister reg, uint16_t value, bool queue)
{
    setRegisterValueUInt32(reg, 0, (uint32_t)value, queue);
}

void FPContext::setRegisterValueUInt16(FPRegister reg, long tag, uint16_t value, bool queue)
{
    setRegisterValueUInt32(reg, tag, (uint32_t)value, queue);
}

void FPContext::setRegisterValueUInt32(FPRegister reg, long tag, uint32_t value, bool queue)
{
    if (queue) {
        //printf(" queue: setting %s [tag=%ld] = 0x%x\n", FPReg2Str(reg).c_str(), tag, value);
        rqueueItem[rqueueCount].reg = reg;
        rqueueItem[rqueueCount].tag = tag;
        rqueueItem[rqueueCount].size = 32;
        rqueueItem[rqueueCount].val.uint32 = value;
        rqueueCount++;
    } else {
        //printf(" setting %s [tag=%ld] = 0x%x\n", FPReg2Str(reg).c_str(), tag, value);
        long idx;
        switch (reg) {
            case REG_NONE: break;
            case REG_EIP: break; // can't set EIP
            case REG_EFLAGS: reg_eflags = (unsigned long)value; break;
            case REG_EAX: reg_eax = (unsigned long)value; break;
            case REG_EBX: reg_ebx = (unsigned long)value; break;
            case REG_ECX: reg_ecx = (unsigned long)value; break;
            case REG_EDX: reg_edx = (unsigned long)value; break;
            case REG_ESP: reg_esp = (unsigned long)value; break;
            case REG_EBP: reg_ebp = (unsigned long)value; break;
            case REG_ESI: reg_esi = (unsigned long)value; break;
            case REG_EDI: reg_edi = (unsigned long)value; break;
            case REG_E8:  reg_r8  = (unsigned long)value; break;
            case REG_E9:  reg_r9  = (unsigned long)value; break;
            case REG_E10: reg_r10 = (unsigned long)value; break;
            case REG_E11: reg_r11 = (unsigned long)value; break;
            case REG_E12: reg_r12 = (unsigned long)value; break;
            case REG_E13: reg_r13 = (unsigned long)value; break;
            case REG_E14: reg_r14 = (unsigned long)value; break;
            case REG_E15: reg_r15 = (unsigned long)value; break;
            case REG_ST0: case REG_ST1: case REG_ST2: case REG_ST3:
            case REG_ST4: case REG_ST5: case REG_ST6: case REG_ST7:
                idx = ((long)reg - (long)REG_ST0)*4+tag;
                fxsave_state->st_space[idx] = value;
                fxsave_state->st_space[idx+1] = 0;
                fxsave_state->st_space[idx+2] = 0;
                if (tag == 0) {
                    // don't go off the edge if we're saving the second byte of a
                    // 64-bit pointer
                    fxsave_state->st_space[idx+3] = 0;
                }
                break;
            case REG_XMM0:  case REG_XMM1:  case REG_XMM2:  case REG_XMM3:
            case REG_XMM4:  case REG_XMM5:  case REG_XMM6:  case REG_XMM7:
            case REG_XMM8:  case REG_XMM9:  case REG_XMM10: case REG_XMM11:
            case REG_XMM12: case REG_XMM13: case REG_XMM14: case REG_XMM15:
                idx = ((long)reg - (long)REG_XMM0)*4;
                fxsave_state->xmm_space[idx+tag] = value;
                break;
            case REG_CS: case REG_DS: case REG_ES: case REG_FS: case REG_GS: case REG_SS:
                assert(!"Cannot set segment registers!");
                break;
        }
    }
}

void FPContext::setRegisterValueUInt64(FPRegister reg, uint64_t value, bool queue)
{
    setRegisterValueUInt64(reg, 0, value, queue);
}

void FPContext::setRegisterValueUInt64(FPRegister reg, long tag, uint64_t value, bool queue)
{
    if (queue) {
        //printf(" queue: setting %s [tag=%ld] = 0x%lx\n", FPReg2Str(reg).c_str(), tag, value);
        rqueueItem[rqueueCount].reg = reg;
        rqueueItem[rqueueCount].tag = tag;
        rqueueItem[rqueueCount].size = 64;
        rqueueItem[rqueueCount].val.uint64 = value;
        rqueueCount++;
    } else {
        //printf(" setting %s [tag=%ld] = 0x%lx\n", FPReg2Str(reg).c_str(), tag, value);
        long idx;
        switch (reg) {
            case REG_NONE: break;
            case REG_EIP: break; // can't set EIP
            case REG_EFLAGS: reg_eflags = (unsigned long)value; break;
            case REG_EAX: reg_eax = (unsigned long)value; break;
            case REG_EBX: reg_ebx = (unsigned long)value; break;
            case REG_ECX: reg_ecx = (unsigned long)value; break;
            case REG_EDX: reg_edx = (unsigned long)value; break;
            case REG_ESP: reg_esp = (unsigned long)value; break;
            case REG_EBP: reg_ebp = (unsigned long)value; break;
            case REG_ESI: reg_esi = (unsigned long)value; break;
            case REG_EDI: reg_edi = (unsigned long)value; break;
            case REG_E8:  reg_r8  = (unsigned long)value; break;
            case REG_E9:  reg_r9  = (unsigned long)value; break;
            case REG_E10: reg_r10 = (unsigned long)value; break;
            case REG_E11: reg_r11 = (unsigned long)value; break;
            case REG_E12: reg_r12 = (unsigned long)value; break;
            case REG_E13: reg_r13 = (unsigned long)value; break;
            case REG_E14: reg_r14 = (unsigned long)value; break;
            case REG_E15: reg_r15 = (unsigned long)value; break;
            case REG_ST0: case REG_ST1: case REG_ST2: case REG_ST3:
            case REG_ST4: case REG_ST5: case REG_ST6: case REG_ST7:
                idx = ((long)reg - (long)REG_ST0)*4;
                fxsave_state->st_space[idx] = (uint32_t)(value & 0xFFFFFFFF);
                fxsave_state->st_space[idx+1] = (uint32_t)(value >> 32);
                fxsave_state->st_space[idx+2] = 0;
                fxsave_state->st_space[idx+3] = 0;
                break;
            case REG_XMM0:  case REG_XMM1:  case REG_XMM2:  case REG_XMM3:
            case REG_XMM4:  case REG_XMM5:  case REG_XMM6:  case REG_XMM7:
            case REG_XMM8:  case REG_XMM9:  case REG_XMM10: case REG_XMM11:
            case REG_XMM12: case REG_XMM13: case REG_XMM14: case REG_XMM15:
                idx = ((long)reg - (long)REG_XMM0)*4;
                fxsave_state->xmm_space[idx+tag] = (uint32_t)(value & 0xFFFFFFFF);
                fxsave_state->xmm_space[idx+tag+1] = (uint32_t)(value >> 32);
                break;
            case REG_CS: case REG_DS: case REG_ES: case REG_FS: case REG_GS: case REG_SS:
                assert(!"Cannot set segment registers!");
                break;
        }
    }
}

void FPContext::setMemoryValueUInt8(void *addr, uint8_t value, bool queue)
{
    if (queue) {
        mqueueItem[mqueueCount].dest = addr;
        mqueueItem[mqueueCount].size = 8;
        mqueueItem[mqueueCount].val.uint8 = value;
        mqueueCount++;
    } else {
        //printf(" setting 0x%p = 0x%x\n", addr, value);
        *(uint8_t*)addr = value;
    }
}

void FPContext::setMemoryValueUInt16(void *addr, uint16_t value, bool queue)
{
    if (queue) {
        mqueueItem[mqueueCount].dest = addr;
        mqueueItem[mqueueCount].size = 16;
        mqueueItem[mqueueCount].val.uint16 = value;
        mqueueCount++;
    } else {
        //printf(" setting 0x%p = 0x%x\n", addr, value);
        *(uint16_t*)addr = value;
    }
}

void FPContext::setMemoryValueUInt32(void *addr, uint32_t value, bool queue)
{
    if (queue) {
        mqueueItem[mqueueCount].dest = addr;
        mqueueItem[mqueueCount].size = 32;
        mqueueItem[mqueueCount].val.uint32 = value;
        mqueueCount++;
    } else {
        //printf(" setting 0x%p = 0x%x\n", addr, value);
        *(uint32_t*)addr = value;
    }
}

void FPContext::setMemoryValueUInt64(void *addr, uint64_t value, bool queue)
{
    if (queue) {
        mqueueItem[mqueueCount].dest = addr;
        mqueueItem[mqueueCount].size = 64;
        mqueueItem[mqueueCount].val.uint64 = value;
        mqueueCount++;
    } else {
        //printf(" setting 0x%p = 0x%lx\n", addr, value);
        *(uint64_t*)addr = value;
    }
}

bool FPContext::isX87StackEmpty()
{
    unsigned top, ftw_mask;
    top = fxsave_state->fsw & 0x3800;
    top >>= 11;
    ftw_mask = 1 << top;
    if (fxsave_state->ftw & ftw_mask) {
        return false;
    } else {
        return true;
    }
}

void FPContext::pushX87Stack(bool queue)
{
    long idx;
    unsigned top, ftw_mask;
    if (queue) {
        queuePushes++;
        return;
    }
    top = fxsave_state->fsw & 0x3800;
    top >>= 11;
    ftw_mask = 1 << top;
    idx = 0;
    //printf("PUSH: TOP=%u START_FTW_MASK=%x\n", top, ftw_mask);
    while ((fxsave_state->ftw & ftw_mask) && idx < 8) {
        ftw_mask <<= 1;
        if (ftw_mask > 0x80) {
            ftw_mask = 0x1;
        }
        idx++;
    }
    //printf("PUSH: idx=%ld END_FTW_MASK=%x\n", idx, ftw_mask);
    if (idx == 8) {
        printf("PUSH: ERROR - FULL STACK!\n");
    }
    //printf("PUSH: TOP=%u FTW_MASK=%x idx=%ld\n", top, ftw_mask, idx);
    fxsave_state->ftw = fxsave_state->ftw | ftw_mask;
    while (idx > 0) {
        //printf("PUSH: save from ST(%ld) to ST(%ld)\n", idx-1, idx);
        fxsave_state->st_space[idx*4+0] = fxsave_state->st_space[(idx-1)*4+0];
        fxsave_state->st_space[idx*4+1] = fxsave_state->st_space[(idx-1)*4+1];
        fxsave_state->st_space[idx*4+2] = fxsave_state->st_space[(idx-1)*4+2];
        idx--;
    }
    fxsave_state->fsw &= 0x3800; // clear exception and condition codes
}

void FPContext::popX87Stack(bool queue)
{
    long idx;
    unsigned top, ftw_mask;
    if (queue) {
        queuePops++;
        return;
    }
    top = fxsave_state->fsw & 0x3800;
    top >>= 11;
    ftw_mask = 1 << top;
    idx = 0;
    //printf("POP: TOP=%u START_FTW_MASK=%x\n", top, ftw_mask);
    // go down stack moving values up
    // stop if we reach the bottom or move 8 values
    while ((fxsave_state->ftw & ftw_mask) && (idx <= 7)) {
        //printf("POP: save from ST(%ld) to ST(%ld)  -  FTW_MASK=%x\n", idx+1, idx, ftw_mask);
        fxsave_state->st_space[idx*4+0] = fxsave_state->st_space[(idx+1)*4+0];
        fxsave_state->st_space[idx*4+1] = fxsave_state->st_space[(idx+1)*4+1];
        fxsave_state->st_space[idx*4+2] = fxsave_state->st_space[(idx+1)*4+2];
        idx++;
        ftw_mask <<= 1;
        if (ftw_mask == 0x0 || ftw_mask > 0x80) {
            ftw_mask = 0x1;
        }
    }
    ftw_mask >>= 1;
    if (ftw_mask == 0) {
        ftw_mask = 0x80;
    }
    //printf("POP: idx=%ld END_FTW_MASK=%x\n", idx, ftw_mask);
    if (idx == 0) {
        printf("POP: ERROR - EMPTY STACK!\n");
    }
    fxsave_state->ftw = fxsave_state->ftw & (~ftw_mask);
    fxsave_state->fsw &= 0x3800; // clear exception and condition codes
}

void FPContext::dumpFXReg(FPRegister reg)
{
    size_t i, j;
    switch (reg) {
        case REG_EIP: printf("| EIP: %016lx\n", reg_eip); break;
        case REG_EFLAGS: printf("| EFLAGS: %016lx\n", reg_eflags); break;
        case REG_EAX: printf("| EAX: %016lx\n", reg_eax); break;
        case REG_EBX: printf("| EBX: %016lx\n", reg_ebx); break;
        case REG_ECX: printf("| ECX: %016lx\n", reg_ecx); break;
        case REG_EDX: printf("| EDX: %016lx\n", reg_edx); break;
        case REG_ESP: printf("| ESP: %016lx\n", reg_esp); break;
        case REG_EBP: printf("| EBP: %016lx\n", reg_ebp); break;
        case REG_ESI: printf("| ESI: %016lx\n", reg_esi); break;
        case REG_EDI: printf("| EDI: %016lx\n", reg_edi); break;
        case REG_E8:  printf("|  R8: %016lx\n", reg_r8);  break;
        case REG_E9:  printf("|  R9: %016lx\n", reg_r9);  break;
        case REG_E10: printf("| R10: %016lx\n", reg_r10); break;
        case REG_E11: printf("| R11: %016lx\n", reg_r11); break;
        case REG_E12: printf("| R12: %016lx\n", reg_r12); break;
        case REG_E13: printf("| R13: %016lx\n", reg_r13); break;
        case REG_E14: printf("| R14: %016lx\n", reg_r14); break;
        case REG_E15: printf("| R15: %016lx\n", reg_r15); break;
        case REG_ST0: case REG_ST1: case REG_ST2: case REG_ST3:
        case REG_ST4: case REG_ST5: case REG_ST6: case REG_ST7:
            i = ((size_t)reg - (size_t)REG_ST0);
            j = i*4;
            printf("| MMX%lu: %04x %08x %08x\n", i,
                    (uint32_t)(fxsave_state->st_space[j+2]) & 0xFFFF,
                    (uint32_t)fxsave_state->st_space[j+1],
                    (uint32_t)fxsave_state->st_space[j+0]);
            break;
        case REG_XMM0:  case REG_XMM1:  case REG_XMM2:  case REG_XMM3:
        case REG_XMM4:  case REG_XMM5:  case REG_XMM6:  case REG_XMM7:
        case REG_XMM8:  case REG_XMM9:  case REG_XMM10: case REG_XMM11:
        case REG_XMM12: case REG_XMM13: case REG_XMM14: case REG_XMM15:
            i = ((size_t)reg - (size_t)REG_XMM0);
            j = i*4;
            printf("| XMM%lu: %08x %08x %08x %08x\n", i,
                    (uint32_t)fxsave_state->xmm_space[j+3],
                    (uint32_t)fxsave_state->xmm_space[j+2],
                    (uint32_t)fxsave_state->xmm_space[j+1],
                    (uint32_t)fxsave_state->xmm_space[j+0]);
            break;
        default:
            break;
    }
}

void FPContext::dumpFXFlags()
{
    unsigned top;
    printf("| FCW: RC=");
    switch ((fxsave_state->fcw & 0xC00) >> 10) {
        case 0: printf("NEAR "); break;
        case 1: printf("DOWN "); break;
        case 2: printf("UP "); break;
        case 3: printf("ZERO "); break;
    }
    printf("PC=");
    switch ((fxsave_state->fcw & 0x300) >> 8) {
        case 0: printf("FLT "); break;
        case 1: printf("N/A "); break;
        case 2: printf("DBL "); break;
        case 3: printf("LDBL "); break;
    }
    if (fxsave_state->fcw & 0x1000) printf("[Y] ");
    if (fxsave_state->fcw & 0x20)   printf("[PM] ");
    if (fxsave_state->fcw & 0x10)   printf("[UM] ");
    if (fxsave_state->fcw & 0x8)    printf("[OM] ");
    if (fxsave_state->fcw & 0x4)    printf("[ZM] ");
    if (fxsave_state->fcw & 0x2)    printf("[DM] ");
    if (fxsave_state->fcw & 0x1)    printf("[IM] ");
    top = fxsave_state->fsw & 0x3800;
    top >>= 11;
    printf("\n| FSW: TOP=%u ", (fxsave_state->fsw & 0x3800) >> 11);
    if (fxsave_state->fsw & 0x8000) printf("[B] ");
    if (fxsave_state->fsw & 0x4000) printf("[C3] ");
    if (fxsave_state->fsw & 0x400)  printf("[C2] ");
    if (fxsave_state->fsw & 0x200)  printf("[C1] ");
    if (fxsave_state->fsw & 0x100)  printf("[C0] ");
    if (fxsave_state->fsw & 0x80)   printf("[ES] ");
    if (fxsave_state->fsw & 0x40)   printf("[SF] ");
    if (fxsave_state->fsw & 0x20)   printf("[PE] ");
    if (fxsave_state->fsw & 0x10)   printf("[UE] ");
    if (fxsave_state->fsw & 0x8)    printf("[OE] ");
    if (fxsave_state->fsw & 0x4)    printf("[ZE] ");
    if (fxsave_state->fsw & 0x2)    printf("[DE] ");
    if (fxsave_state->fsw & 0x1)    printf("[IE] ");
    printf("\n| FTW: ");
    if (fxsave_state->ftw & 0x1)    printf("[FPR0] ");
    if (fxsave_state->ftw & 0x2)    printf("[FPR1] ");
    if (fxsave_state->ftw & 0x4)    printf("[FPR2] ");
    if (fxsave_state->ftw & 0x8)    printf("[FPR3] ");
    if (fxsave_state->ftw & 0x10)   printf("[FPR4] ");
    if (fxsave_state->ftw & 0x20)   printf("[FPR5] ");
    if (fxsave_state->ftw & 0x40)   printf("[FPR6] ");
    if (fxsave_state->ftw & 0x80)   printf("[FPR7] ");
    printf("\n");
}

void FPContext::dumpFXSave(bool printMMX, bool printXMM)
{
    unsigned i, j;
    dumpFXFlags();
    if (printMMX) {
        for (i=0; i<8; i++) {
            j = i*4;
            printf("| MMX%u: %04x %08x %08x\n", i,
            //printf("| MMX%u: %04x %08x %08x  (%8.2Le)\n", i,
                    (uint32_t)(fxsave_state->st_space[j+2]) & 0xFFFF,
                    (uint32_t)fxsave_state->st_space[j+1],
                    (uint32_t)fxsave_state->st_space[j+0]);
                    //*(long double*)(&fxsave_state->st_space[j+0]));
        }
    }
    if (printXMM) {
        for (i=0; i<8; i++) {
           j = i*4;
           printf("| XMM%u: %08x %08x %08x %08x\n", i,
                  (uint32_t)fxsave_state->xmm_space[j+3],
                  (uint32_t)fxsave_state->xmm_space[j+2],
                  (uint32_t)fxsave_state->xmm_space[j+1],
                  (uint32_t)fxsave_state->xmm_space[j+0]);
           //printf(" (%8.2Le, %8.2Le)\n",
                 //(long double) (*(double*)(&fxsave_state->xmm_space[j+2])),
                 //(long double) (*(double*)(&fxsave_state->xmm_space[j+0])));
           //__asm__ ("emms;");
           //printf("|      (%8.2e, %8.2e, %8.2e, %8.2e)\n",
                 //*(float*)(&fxsave_state->xmm_space[j+3]),
                 //*(float*)(&fxsave_state->xmm_space[j+2]),
                 //*(float*)(&fxsave_state->xmm_space[j+1]),
                 //*(float*)(&fxsave_state->xmm_space[j+0]));
        }
    }
}

}

