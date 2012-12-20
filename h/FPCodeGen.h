#ifndef __FPCODEGEN_H
#define __FPCODEGEN_H

#include "FPContext.h"
#include "FPOperand.h"

using namespace std;

namespace FPInst {

/**
 * Performs machine code generation that is independent of blob specifics.
 * Currently only supports x86_64.
 */
class FPCodeGen {

    public:

        FPCodeGen();

        unsigned char getRegModRMId(FPRegister reg);
        unsigned char getSegRegByte(FPRegister reg);

        size_t buildREX(unsigned char *pos,
                bool wide, FPRegister reg, FPRegister index, FPRegister base_rm);

        size_t buildModRM(unsigned char *pos,
                FPRegister reg, FPRegister rm);

        size_t buildModRM(unsigned char *pos,
                unsigned char mod, FPRegister reg, FPRegister rm);

        size_t buildInstruction(unsigned char *pos,
                unsigned char prefix, bool wide_operands, bool two_byte_opcode,
                unsigned char opcode, FPRegister reg);

        size_t buildInstruction(unsigned char *pos,
                unsigned char prefix, bool wide_operands, bool two_byte_opcode,
                unsigned char opcode, FPRegister reg, FPRegister rm,
                bool memory, int32_t disp, FPRegister seg = REG_NONE);

        size_t buildInstruction(unsigned char *pos,
                unsigned char prefix, bool wide_operands, bool two_byte_opcode,
                unsigned char opcode, FPRegister reg, 
                long scale, FPRegister index, FPRegister base, int32_t disp, 
                FPRegister seg = REG_NONE);

        size_t buildPushReg(unsigned char *pos,
                FPRegister reg);

        size_t buildPopReg(unsigned char *pos,
                FPRegister reg);

        size_t buildPushFlags(unsigned char *pos);
        size_t buildPopFlags(unsigned char *pos);

        size_t buildPushFlagsFast(unsigned char *pos);
        size_t buildPopFlagsFast(unsigned char *pos);

        size_t buildSaveFlagsFast(unsigned char *pos, int32_t sp_offset,
                bool fixByteOrder = false);
        size_t buildRestoreFlagsFast(unsigned char *pos, int32_t sp_offset,
                bool fixByteOrder = false);

        size_t buildMovXmmToGPR64(unsigned char *pos,
                FPRegister xmm, FPRegister gpr);

        size_t buildMovGPR64ToXmm(unsigned char *pos,
                FPRegister gpr, FPRegister xmm);

        size_t buildMovGPR64ToGPR64(unsigned char *pos,
                FPRegister gpr_src, FPRegister gpr_dest);

        size_t buildMovGPR64ToStack(unsigned char *pos,
                FPRegister gpr_src, uint32_t offset);

        size_t buildMovStackToGPR64(unsigned char *pos,
                FPRegister gpr_dest, uint32_t offset);

        size_t buildMovXmmToLocal64(unsigned char *pos,
                FPRegister xmm, int8_t disp);

        size_t buildMovLocal64ToXmm(unsigned char *pos,
                int8_t disp, FPRegister xmm);

        size_t buildCvtss2sd(unsigned char *pos,
                FPRegister reg1, FPRegister reg2);

        size_t buildCvtps2pd(unsigned char *pos,
                FPRegister reg1, FPRegister reg2);

        size_t buildCvtsd2ss(unsigned char *pos,
                FPRegister reg1, FPRegister reg2);

        size_t buildCvtpd2ps(unsigned char *pos,
                FPRegister reg1, FPRegister reg2);

        size_t buildUnpcklps(unsigned char *pos,
                FPRegister reg1, FPRegister reg2);

        size_t buildUnpckhps(unsigned char *pos,
                FPRegister reg1, FPRegister reg2);

        size_t buildMovImm32ToGPR32(unsigned char *pos,
                uint32_t val, FPRegister gpr);

        size_t buildMovImm32ToStack(unsigned char *pos,
                uint32_t val, int32_t offset);

        size_t buildMovImm64ToGPR64(unsigned char *pos,
                uint64_t imm, FPRegister gpr);

        size_t buildAndGPR64WithGPR64(unsigned char *pos, FPRegister reg, FPRegister rm);
        size_t buildOrGPR64WithGPR64(unsigned char *pos, FPRegister reg, FPRegister rm);
        size_t buildXorGPR64WithGPR64(unsigned char *pos, FPRegister reg, FPRegister rm);

        size_t buildCmpGPR64WithGPR64(unsigned char *pos, FPRegister rm, FPRegister reg);

        size_t buildAddGPR64ToGPR64(unsigned char *pos, FPRegister reg, FPRegister rm);

        size_t buildIncMem64(unsigned char *pos, int32_t offset);

        size_t buildJumpNear32(unsigned char *pos,
                int32_t offset, int32_t* &offset_pos);

        size_t buildJumpEqualNear32(unsigned char *pos,
                int32_t offset, int32_t* &offset_pos);

        size_t buildJumpNotEqualNear32(unsigned char *pos,
                int32_t offset, int32_t* &offset_pos);

        size_t buildJumpLessNear32(unsigned char *pos,
                int32_t offset, int32_t* &offset_pos);

        size_t buildJumpGreaterEqualNear32(unsigned char *pos,
                int32_t offset, int32_t* &offset_pos);

        size_t buildJumpLessEqualNear32(unsigned char *pos,
                int32_t offset, int32_t* &offset_pos);

        size_t buildNop(unsigned char *pos);

        // PINSR/PEXTR (SSE4) versions
        size_t buildInsertGPR32IntoXMM(unsigned char *pos,
                FPRegister gpr, FPRegister xmm, long tag);
        size_t buildExtractGPR64FromXMM(unsigned char *pos,
                FPRegister gpr, FPRegister xmm, long tag);
        size_t buildInsertGPR64IntoXMM(unsigned char *pos,
                FPRegister gpr, FPRegister xmm, long tag);

        size_t buildSPAdjustment(unsigned char *pos,
                int8_t offset);

        size_t buildSPAdjustment(unsigned char *pos,
                int32_t offset);

        size_t buildTrampExit(unsigned char *pos);

        size_t buildTrampReentrance(unsigned char *pos);

        size_t buildDie(unsigned char *pos);

};

}

#endif

