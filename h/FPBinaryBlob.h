#ifndef __FPBINARYBLOB_H
#define __FPBINARYBLOB_H

#include "FPCodeGen.h"
#include "FPContext.h"
#include "FPSemantics.h"

using namespace std;

namespace FPInst {

/**
 * Assembles binary blobs of code for instrumentation. Generates machine code
 * for blob-specific functions (i.e. stack-relative operands). Currently only
 * supports x86_64.
 */
class FPBinaryBlob {

    public:

        //static const int32_t DYNINST_STACK_OFFSET = 0x88;
        static const int32_t DYNINST_STACK_OFFSET = 0x90;

        FPBinaryBlob(FPSemantics *inst);

        unsigned char *getBlobCode();

        void* getBlobAddress();
        void setBlobAddress(void *addr);

        int32_t getFakeStackOffset();
        void setFakeStackOffset(int32_t offset);
        void adjustFakeStackOffset(int32_t offset);

        int32_t getSavedEAXOffset();
        int32_t getSavedFlagsOffset();
        int32_t getBlobCommOffset();

        void adjustDisplacement(long oldDisp, unsigned char *pos);

        size_t buildDyninstHeader(unsigned char *pos);
        size_t buildHeader(unsigned char *pos);
        size_t buildDyninstFooter(unsigned char *pos);
        size_t buildFooter(unsigned char *pos);

        size_t buildFakeStackPushGPR64(unsigned char *pos, FPRegister gpr);
        size_t buildFakeStackPushImm32(unsigned char *pos, uint32_t imm);
        size_t buildFakeStackPushXMM(unsigned char *pos, FPRegister xmm);
        size_t buildFakeStackPopGPR64(unsigned char *pos, FPRegister gpr);
        size_t buildFakeStackPopXMM(unsigned char *pos, FPRegister xmm);

        size_t buildOperandLoadGPR(unsigned char *pos, FPOperand *src, FPRegister dest_gpr);
        size_t buildOperandLoadXMM(unsigned char *pos, FPOperand *src, FPRegister dest_xmm, bool packed);
        size_t buildOperandStoreGPR(unsigned char *pos, FPRegister src, FPOperand *dest);

        FPRegister getUnusedGPR();
        FPRegister getUnusedSSE();

        bool isGPR(FPRegister reg);
        bool isSSE(FPRegister reg);

        void initialize();
        void finalize();

    protected:

        FPCodeGen *mainGen;
        FPSemantics *inst;
        void *blobAddress;
        unsigned char *blobCode;
        int32_t fake_stack_offset;
        int32_t saved_eax_offset;
        int32_t saved_flags_offset;
        int32_t blob_comm_offset;
        set<FPRegister> usedRegs;
};

}

#endif

