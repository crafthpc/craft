#ifndef __FPCONTEXT_H
#define __FPCONTEXT_H

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <xmmintrin.h>

#include <string>

using namespace std;

namespace FPInst {

/**
 * Structure for data saved/restored with FXSAVE/FXRSTOR SSE 
 * instructions.
 */
struct fxsave_data {
	uint16_t fcw;
	uint16_t fsw;
	uint16_t ftw;
	uint16_t fop;
	uint64_t rip;   // uint32_t ip;
	                // uint32_t cs;
	uint64_t rdp;   // uint32_t dp;
	                // uint32_t ds;
	uint32_t mxcsr;
	uint32_t reserved;
	uint32_t st_space[32];	// 10 bytes for each x87 register (padded to 16 for each)
	uint32_t xmm_space[64];	// 16 bytes for each XMM register
};

/**
 * Local register constants.
 * Note that the general purpose register names with "E" also refer 
 * to the "R" versions on x86_64 platforms.
 */
enum FPRegister {
    // TODO: rename to avoid clashes with constants in ucontext.h
    REG_NONE, REG_EIP, REG_EFLAGS,
    REG_EAX, REG_EBX, REG_ECX, REG_EDX,
    REG_EBP, REG_ESP, REG_ESI, REG_EDI,
    REG_E8,  REG_E9,  REG_E10, REG_E11,
    REG_E12, REG_E13, REG_E14, REG_E15,
    REG_ST0, REG_ST1, REG_ST2, REG_ST3,
    REG_ST4, REG_ST5, REG_ST6, REG_ST7,
    REG_XMM0,  REG_XMM1,  REG_XMM2,  REG_XMM3,
    REG_XMM4,  REG_XMM5,  REG_XMM6,  REG_XMM7,
    REG_XMM8,  REG_XMM9,  REG_XMM10, REG_XMM11,
    REG_XMM12, REG_XMM13, REG_XMM14, REG_XMM15,
};

/**
 * An action stored in the register queue.
 */
typedef struct {
    FPRegister reg;
    size_t size;
    long tag;
    union {
        //uint8_t uint8;
        //uint16_t uint16;
        uint32_t uint32;
        uint64_t uint64;
    } val;
} FPContextRQueueItem;

/**
 * An action stored in the memory queue.
 */
typedef struct {
    void* dest;
    size_t size;
    union {
        uint8_t uint8;
        uint16_t uint16;
        uint32_t uint32;
        uint64_t uint64;
    } val;
} FPContextMQueueItem;


/**
 * Provides methods for accessing cached machine state.
 *
 * Contains a variety of methods for retrieving and modifying registers and
 * memory locations. Retrievals are handled immediately (usually from cached
 * data), while modifications can be executed immediately or stored in an action
 * queue to be finalized when desired. The class generally relies on external
 * sources to refresh the cached values and to save these values back to actual
 * machine state.
 *
 * Thus, the FPContext class serves as a sort of "buffer" for these operations,
 * preventing unnecessary access to system state if library internals perform
 * repeated accesses to the same variable. The use of Dyninst for
 * instrumentation also makes this access pattern very convenient, since
 * snippets can easily be written to refresh values and to save them back to the
 * system.
 *
 * Note that right now this class is used as a singleton. There are several
 * global variables in the source file that act as pointers to the member
 * variables of this object. These must be intialized when the analysis begins.
 * This layer of indirection preserves the ability to use multiple FPContext
 * objects if this functionality is needed in the future, but currently I have
 * only seen the need for a single context.
 */
class FPContext
{
    public:

        static string FPReg2Str(FPRegister reg);

        FPContext();

        void updateIP(unsigned long eip);
        void updateGPRs(unsigned long eax, unsigned long ebx,
                unsigned long ecx, unsigned long edx, unsigned long esp,
                unsigned long ebp, unsigned long esi, unsigned long edi);
        
        void saveAllFPR();
        void restoreAllFPR();

        void clearRegisterCaches();

        void resetQueuedActions();
        void finalizeQueuedActions();

        void getRegisterInt(unsigned long *dest, FPRegister reg);

        void getRegisterValue(void *dest, FPRegister reg);
        void getRegisterValue(void *dest, FPRegister reg, long tag, size_t size);

        void getMemoryAddress(void **addr, FPRegister base, FPRegister index, 
                              long disp, long scale, size_t size);
        void getMemoryValue(void *dest, void *addr, size_t size);
        void getMemoryValue(void *dest, void **addr, FPRegister base, FPRegister index, 
                            long disp, long scale, size_t size);

        void setRegisterValueUInt8(FPRegister reg, uint8_t value, bool queue=false);
        void setRegisterValueUInt8(FPRegister reg, long tag, uint8_t value, bool queue=false);
        void setMemoryValueUInt8(void *addr, uint8_t value, bool queue=false);
        void setRegisterValueUInt16(FPRegister reg, uint16_t value, bool queue=false);
        void setRegisterValueUInt16(FPRegister reg, long tag, uint16_t value, bool queue=false);
        void setMemoryValueUInt16(void *addr, uint16_t value, bool queue=false);
        void setRegisterValueUInt32(FPRegister reg, uint32_t value, bool queue=false);
        void setRegisterValueUInt32(FPRegister reg, long tag, uint32_t value, bool queue=false);
        void setMemoryValueUInt32(void *addr, uint32_t value, bool queue=false);
        void setRegisterValueUInt64(FPRegister reg, uint64_t value, bool queue=false);
        void setRegisterValueUInt64(FPRegister reg, long tag, uint64_t value, bool queue=false);
        void setMemoryValueUInt64(void *addr, uint64_t value, bool queue=false);

        bool isX87StackEmpty();
        void pushX87Stack(bool queue=false);
        void popX87Stack(bool queue=false);

        void dumpFXReg(FPRegister reg);
        void dumpFXFlags();
        void dumpFXSave(bool printMMX, bool printXMM);

    public:

        /* made public for speed */
        struct fxsave_data* fxsave_state;     /* guaranteed to be 16-byte aligned */
        unsigned long reg_eip, reg_eflags;
        unsigned long reg_eax, reg_ebx, reg_ecx, reg_edx, 
                      reg_esp, reg_ebp, reg_esi, reg_edi,
                      reg_r8,  reg_r9,  reg_r10, reg_r11,
                      reg_r12, reg_r13, reg_r14, reg_r15;

    public:

        /* useful constants */

        static const unsigned long FP_X87_FLAG_C0 = 0x0100;
        static const unsigned long FP_X87_FLAG_C1 = 0x0200;
        static const unsigned long FP_X87_FLAG_C2 = 0x0400;
        static const unsigned long FP_X87_FLAG_C3 = 0x4000;

        static const unsigned long FP_X87_CLEAR_FLAGS = 
                FP_X87_FLAG_C0 | FP_X87_FLAG_C1 | FP_X87_FLAG_C2 | FP_X87_FLAG_C3;
        static const unsigned long FP_X87_FLAGS_UNORDERED =
                FP_X87_FLAG_C0 | FP_X87_FLAG_C2 | FP_X87_FLAG_C3;
        static const unsigned long FP_X87_FLAGS_EQUAL = FP_X87_FLAG_C3;
        static const unsigned long FP_X87_FLAGS_GREATER = 0;
        static const unsigned long FP_X87_FLAGS_LESS = FP_X87_FLAG_C0;

#ifdef __X86_64__
        static const unsigned long FP_FLAG_CF  = 0x0001;
        static const unsigned long FP_FLAG_PF  = 0x0004;
        static const unsigned long FP_FLAG_AF  = 0x0010;
        static const unsigned long FP_FLAG_ZF  = 0x0040;
        static const unsigned long FP_FLAG_SF  = 0x0080;
        static const unsigned long FP_FLAG_OF  = 0x0800;
#else
        static const unsigned long FP_FLAG_CF  = 0x0100; // Dyninst preserves flags with lahf/seto
        static const unsigned long FP_FLAG_PF  = 0x0400; // so the constants are different on x86
        static const unsigned long FP_FLAG_AF  = 0x1000;
        static const unsigned long FP_FLAG_ZF  = 0x4000;
        static const unsigned long FP_FLAG_SF  = 0x8000;
        //static const unsigned long FP_FLAG_OF  = 0x0001; // was this incorrect?
        static const unsigned long FP_FLAG_OF  = 0x0008;
#endif

        static const unsigned long FP_CLEAR_FLAGS = 
                FP_FLAG_CF | FP_FLAG_PF | FP_FLAG_AF |
                FP_FLAG_ZF | FP_FLAG_SF | FP_FLAG_OF;
        static const unsigned long FP_FLAGS_UNORDERED = 
                FP_FLAG_ZF | FP_FLAG_PF | FP_FLAG_CF;
        static const unsigned long FP_FLAGS_EQUAL = FP_FLAG_ZF;
        static const unsigned long FP_FLAGS_GREATER = 0;
        static const unsigned long FP_FLAGS_LESS = FP_FLAG_CF;

    private:

        static const unsigned long FXSAVE_DATA_SIZE = 512;
        char* fxsave_state_buffer;

        long double reg_st0, reg_st1, reg_st2, reg_st3, 
                    reg_st4, reg_st5, reg_st6, reg_st7;

        __m128 xmmTemp;

        long highestCachedX87;
        long cachedXMM;


        size_t queuePushes;
        size_t queuePops;
        
        FPContextRQueueItem rqueueItem[128];
        size_t rqueueCount;

        FPContextMQueueItem mqueueItem[128];
        size_t mqueueCount;

};

}

#endif

