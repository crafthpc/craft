#include "FPBinaryBlob.h"

namespace FPInst {

FPBinaryBlob::FPBinaryBlob(FPSemantics *inst)
{
    this->mainGen = new FPCodeGen();
    this->inst = inst;
    this->blobAddress = NULL;
    blobCode = (unsigned char*)malloc(1024);
    assert(blobCode);
    fake_stack_offset = -0xb0;
    //fake_stack_offset = -0x10;
    saved_eax_offset = 0x10;
    saved_flags_offset = 0x8;
    blob_comm_offset = 0x0;
    initialize();
}

unsigned char *FPBinaryBlob::getBlobCode()
{
    return blobCode;
}

void* FPBinaryBlob::getBlobAddress()
{
    return blobAddress;
}

void FPBinaryBlob::setBlobAddress(void *addr)
{
    blobAddress = addr;
}

int32_t FPBinaryBlob::getFakeStackOffset()
{
    return fake_stack_offset;
}

void FPBinaryBlob::setFakeStackOffset(int32_t offset)
{
    fake_stack_offset = offset;
}

void FPBinaryBlob::adjustFakeStackOffset(int32_t offset)
{
    fake_stack_offset += offset;
}

int32_t FPBinaryBlob::getSavedEAXOffset()
{
    return saved_eax_offset;
}

int32_t FPBinaryBlob::getSavedFlagsOffset()
{
    return saved_flags_offset;
}

int32_t FPBinaryBlob::getBlobCommOffset()
{
    return blob_comm_offset;
}

void FPBinaryBlob::adjustDisplacement(long oldDisp, unsigned char *pos)
{
    void *actual_loc = (void*)((long)inst->getAddress() + (long)inst->getNumBytes() + oldDisp);
    void *new_insn_loc = (void*)((long)blobAddress + (long)(pos-blobCode));
    unsigned char *disp_loc = pos - 4;
    long new_disp = (long)actual_loc - (long)new_insn_loc;
    //printf("  actual_loc=%p", actual_loc);
    //printf("  new_insn_loc=%p", new_insn_loc);
    //printf("  new_disp=%d\n", (int32_t)new_disp);
    *(int32_t*)disp_loc = (int32_t)(new_disp);
    //printf("FPBinaryBlob::adjustDisplacement(%ld, %p, %p, %p, %p, %ld)\n",
            //oldDisp, actual_loc, blobAddress, blobCode, new_insn_loc, new_disp);
}

size_t FPBinaryBlob::buildHeader(unsigned char *pos)
{
    unsigned char *old_pos = pos;
    fake_stack_offset = -0xb0;
    pos += mainGen->buildNop(pos);
    adjustFakeStackOffset(-8);
    pos += mainGen->buildInstruction(pos, 0x0, true, false,
            0x89, REG_EAX, REG_ESP, true, getFakeStackOffset());
    saved_eax_offset = getFakeStackOffset();
    adjustFakeStackOffset(-8);
    pos += mainGen->buildSaveFlagsFast(pos, getFakeStackOffset());
    saved_flags_offset = getFakeStackOffset();
    adjustFakeStackOffset(-8);
    blob_comm_offset = getFakeStackOffset();
    return (size_t)(pos - old_pos);
}

size_t FPBinaryBlob::buildFooter(unsigned char *pos)
{
    unsigned char *old_pos = pos;
    assert(getBlobCommOffset() == getFakeStackOffset());
    adjustFakeStackOffset(8);
    assert(getSavedFlagsOffset() == getFakeStackOffset());
    pos += mainGen->buildRestoreFlagsFast(pos, getFakeStackOffset());
    adjustFakeStackOffset(8);
    assert(getSavedEAXOffset() == getFakeStackOffset());
    pos += mainGen->buildInstruction(pos, 0x0, true, false,
            0x8b, REG_EAX, REG_ESP, true, getFakeStackOffset());
    adjustFakeStackOffset(8);
    pos += mainGen->buildNop(pos);
    return (size_t)(pos - old_pos);
}

size_t FPBinaryBlob::buildFakeStackPushGPR64(unsigned char *pos, FPRegister gpr)
{
    unsigned char *old_pos = pos;
    adjustFakeStackOffset(-8);
    pos += mainGen->buildInstruction(pos, 0, true, false,
            0x89, gpr, REG_ESP, true, getFakeStackOffset());
    return (size_t)(pos - old_pos);
}

size_t FPBinaryBlob::buildFakeStackPushImm32(unsigned char *pos, uint32_t imm)
{
    unsigned char *old_pos = pos;
    adjustFakeStackOffset(-8);
    pos += mainGen->buildInstruction(pos, 0, true, false,
            0xc7, (FPRegister)0, REG_ESP, true, getFakeStackOffset());
    *(uint32_t*)pos = imm; pos += 4;
    return (size_t)(pos - old_pos);
}

size_t FPBinaryBlob::buildFakeStackPushXMM(unsigned char *pos, FPRegister xmm)
{
    unsigned char *old_pos = pos;
    adjustFakeStackOffset(-16);
    pos += mainGen->buildInstruction(pos, 0x66, false, true,
            0x11, xmm, REG_ESP, true, getFakeStackOffset());
    return (size_t)(pos - old_pos);
}

size_t FPBinaryBlob::buildFakeStackPopGPR64(unsigned char *pos, FPRegister gpr)
{
    unsigned char *old_pos = pos;
    pos += mainGen->buildInstruction(pos, 0, true, false,
            0x8b, gpr, REG_ESP, true, getFakeStackOffset());
    adjustFakeStackOffset(8);
    return (size_t)(pos - old_pos);
}

size_t FPBinaryBlob::buildFakeStackPopXMM(unsigned char *pos, FPRegister xmm)
{
    unsigned char *old_pos = pos;
    pos += mainGen->buildInstruction(pos, 0x66, false, true,
            0x10, xmm, REG_ESP, true, getFakeStackOffset());
    adjustFakeStackOffset(16);
    return (size_t)(pos - old_pos);
}

size_t FPBinaryBlob::buildOperandLoadGPR(unsigned char *pos,
        FPOperand *src, FPRegister dest_gpr)
{
    assert(dest_gpr >= REG_EAX && dest_gpr <= REG_E15);
    //assert(src->getType() == IEEE_Double);
    unsigned char *old_pos = pos;
    unsigned char prefix = 0x0;
    bool wide_operands = true;
    unsigned char opcode = 0x8b;

    // restore original %rax if necessary
    // (save ours on the fake stack first)
    if (src->getRegister() == REG_EAX ||
        src->getBase() == REG_EAX ||
        src->getIndex() == REG_EAX) {
        adjustFakeStackOffset(-8);
        pos += mainGen->buildInstruction(pos, 0x0, true, false,
                0x89, REG_EAX, REG_ESP, true, getFakeStackOffset());
        pos += mainGen->buildInstruction(pos, 0x0, true, false,
                0x8b, REG_EAX, REG_ESP, true, getSavedEAXOffset());
    }

    // get the operand value
    if (src->getType() == IEEE_Single ||
        src->getType() == SignedInt32 ||
        src->getType() == UnsignedInt32) {
        wide_operands = false;
    } else if (src->getType() != IEEE_Double &&
               src->getType() != SignedInt64 &&
               src->getType() != UnsignedInt64) {
        assert(!"Cannot initialize operand: not 32 or 64 bits");
    }
    if (src->isRegisterSSE()) {
        // mov %xmm, %gpr
        pos += mainGen->buildInstruction(pos, 0x66, wide_operands, true, 
                0x7e, dest_gpr, src->getRegister(), false, 0);
    } else if (src->isRegisterGPR()) {
        // mov %gpr, %gpr
        if (src->getRegister() != dest_gpr) {
            pos += mainGen->buildMovGPR64ToGPR64(pos, src->getRegister(), dest_gpr);
        }
    } else if (src->getBase() != REG_NONE && src->getIndex() == REG_NONE) {
        // mov $disp(%gpr), %gpr
        pos += mainGen->buildInstruction(pos, prefix, wide_operands, false, 
                opcode, dest_gpr, src->getBase(), true, src->getDisp(), src->getSegment());
    } else if (src->isMemory()) {
        // mov $disp(%base,%index,$scale), %gpr
        pos += mainGen->buildInstruction(pos, prefix, wide_operands, false, 
                opcode, dest_gpr, src->getScale(), src->getIndex(),
                src->getBase(), src->getDisp(), src->getSegment());
    } else {
        assert(!"unsupported operand");
    }

    if (src->getBase() == REG_EIP) {
        adjustDisplacement(src->getDisp(), pos);
    }

    // put our own %rax value back
    if (src->getRegister() == REG_EAX ||
        src->getBase() == REG_EAX ||
        src->getIndex() == REG_EAX) {
        pos += mainGen->buildInstruction(pos, 0x0, true, false,
                0x8b, REG_EAX, REG_ESP, true, getFakeStackOffset());
        adjustFakeStackOffset(8);
    }

    return (size_t)(pos - old_pos);
}

size_t FPBinaryBlob::buildOperandLoadXMM(unsigned char *pos,
        FPOperand *src, FPRegister dest_xmm, bool packed)
{
    assert(dest_xmm >= REG_XMM0 && dest_xmm <= REG_XMM15);
    //printf("buildOperandLoadXMM: %s\n", src->toString().c_str());
    unsigned char *old_pos = pos;
    unsigned char prefix = (packed ? 0x66 : 0xf2);
    bool wide_operands = false;
    unsigned char opcode = 0x10;
    if (src->getType() == IEEE_Single ||
        src->getType() == SignedInt32 ||
        src->getType() == UnsignedInt32) {
        prefix = (packed ? 0 : 0xf3);
    } else if (src->getType() != IEEE_Double &&
               src->getType() != SignedInt64 &&
               src->getType() != UnsignedInt64 &&
               src->getType() != SSE_Quad) {
        assert(!"Cannot initialize operand: not 32 or 64 bits");
    }

    // restore original %rax if necessary
    // (save ours on the fake stack first)
    if (src->getRegister() == REG_EAX ||
        src->getBase() == REG_EAX ||
        src->getIndex() == REG_EAX) {
        adjustFakeStackOffset(-8);
        pos += mainGen->buildInstruction(pos, 0x0, true, false,
                0x89, REG_EAX, REG_ESP, true, getFakeStackOffset());
        pos += mainGen->buildInstruction(pos, 0x0, true, false,
                0x8b, REG_EAX, REG_ESP, true, getSavedEAXOffset());
    }

    // get the operand value
    if (src->isRegisterSSE()) {
        // movxx %xmm, %xmm
        if (src->getRegister() != dest_xmm) {
            pos += mainGen->buildInstruction(pos, prefix, wide_operands, true, 
                    opcode, dest_xmm, src->getRegister(), false, 0);
        }
    } else if (src->isRegisterGPR()) {
        // movxx %gpr, %xmm
        pos += mainGen->buildMovGPR64ToXmm(pos, src->getRegister(), dest_xmm);
    } else if (src->getBase() != REG_NONE && src->getIndex() == REG_NONE) {
        // movxx $disp(%gpr), %xmm
        pos += mainGen->buildInstruction(pos, prefix, wide_operands, true, 
                opcode, dest_xmm, src->getBase(), true, src->getDisp(), src->getSegment());
    } else if (src->isMemory()) {
        // movxx $disp(%base,%index,$scale), %xmm
        pos += mainGen->buildInstruction(pos, prefix, wide_operands, true, 
                opcode, dest_xmm, src->getScale(), src->getIndex(),
                src->getBase(), src->getDisp(), src->getSegment());
    } else {
        assert(!"unsupported operand");
    }

    if (src->getBase() == REG_EIP) {
        adjustDisplacement(src->getDisp(), pos);
    }

    // put our own %rax value back
    if (src->getRegister() == REG_EAX ||
        src->getBase() == REG_EAX ||
        src->getIndex() == REG_EAX) {
        pos += mainGen->buildInstruction(pos, 0x0, true, false,
                0x8b, REG_EAX, REG_ESP, true, getFakeStackOffset());
        adjustFakeStackOffset(8);
    }

    return (size_t)(pos - old_pos);
}

size_t FPBinaryBlob::buildOperandStoreGPR(unsigned char *pos,
        FPRegister src, FPOperand *dest)
{
    unsigned char *old_pos = pos;

    // restore original %rax if necessary
    // (save ours on the fake stack first)
    if (dest->getRegister() == REG_EAX ||
        dest->getBase() == REG_EAX ||
        dest->getIndex() == REG_EAX) {
        adjustFakeStackOffset(-8);
        pos += mainGen->buildInstruction(pos, 0x0, true, false,
                0x89, REG_EAX, REG_ESP, true, getFakeStackOffset());
        pos += mainGen->buildInstruction(pos, 0x0, true, false,
                0x8b, REG_EAX, REG_ESP, true, getSavedEAXOffset());
    }

    // set the operand value
    if (dest->isRegisterSSE()) {
        // mov %gpr, %xmm
        pos += mainGen->buildInstruction(pos, 0x66, true, true, 0x6e, 
                dest->getRegister(), src, false, 0);
    } else if (dest->isRegisterGPR()) {
        // TODO: special handling for spilled registers?
        // mov %gpr, %gpr
        pos += mainGen->buildMovGPR64ToGPR64(pos, src, dest->getRegister());
    } else if (dest->isLocalVar()) {
        // mov %gpr, $disp(%gpr)
        pos += mainGen->buildInstruction(pos, 0, true, false, 0x89,
                src, dest->getBase(), true, dest->getDisp());
    } else if (dest->getBase() == REG_EIP) {
        // mov %gpr, $disp(%rip)
        pos += mainGen->buildInstruction(pos, 0, true, false, 0x89,
                src, REG_EIP, true, dest->getDisp());
        adjustDisplacement(dest->getDisp(), pos);
    } else if (dest->isMemory()) {
        // mov %gpr, $disp(%base,%index,$scale)
        pos += mainGen->buildInstruction(pos, 0, true, false, 0x89,
                src, dest->getScale(), dest->getIndex(),
                dest->getBase(), dest->getDisp());
    } else {
        assert(!"unsupported operand");
    }

    // put our own %rax value back
    if (dest->getRegister() == REG_EAX ||
        dest->getBase() == REG_EAX ||
        dest->getIndex() == REG_EAX) {
        pos += mainGen->buildInstruction(pos, 0x0, true, false,
                0x8b, REG_EAX, REG_ESP, true, getFakeStackOffset());
        adjustFakeStackOffset(8);
    }

    return (size_t)(pos-old_pos);
}

FPRegister FPBinaryBlob::getUnusedGPR()
{
    FPRegister reg = REG_NONE;
    /*
     *set<FPRegister>::iterator it;
     *printf("used regs: ");
     *for (it = usedRegs.begin(); it != usedRegs.end(); it++) {
     *   printf("%s ", FPContext::FPReg2Str(*it).c_str());
     *}
     *printf("\n");
     */

         if (usedRegs.find(REG_EAX) == usedRegs.end()) reg = REG_EAX;
    else if (usedRegs.find(REG_EBX) == usedRegs.end()) reg = REG_EBX;
    else if (usedRegs.find(REG_ECX) == usedRegs.end()) reg = REG_ECX;
    else if (usedRegs.find(REG_EDX) == usedRegs.end()) reg = REG_EDX;
    else if (usedRegs.find(REG_ESI) == usedRegs.end()) reg = REG_ESI;
    else if (usedRegs.find(REG_EDI) == usedRegs.end()) reg = REG_EDI;

    /* some places require a non-extended register
     *else if (usedRegs.find(REG_E8)  == usedRegs.end()) reg = REG_E8;
     *else if (usedRegs.find(REG_E9)  == usedRegs.end()) reg = REG_E9;
     *else if (usedRegs.find(REG_E10) == usedRegs.end()) reg = REG_E10;
     *else if (usedRegs.find(REG_E11) == usedRegs.end()) reg = REG_E11;
     *else if (usedRegs.find(REG_E12) == usedRegs.end()) reg = REG_E12;
     *else if (usedRegs.find(REG_E13) == usedRegs.end()) reg = REG_E13;
     *else if (usedRegs.find(REG_E14) == usedRegs.end()) reg = REG_E14;
     *else if (usedRegs.find(REG_E15) == usedRegs.end()) reg = REG_E15;
     */

    if (reg != REG_NONE) {
        usedRegs.insert(reg);
    }
    return reg;
}

FPRegister FPBinaryBlob::getUnusedSSE()
{
    FPRegister reg = REG_NONE;
    /*
     *set<FPRegister>::iterator it;
     *printf("used regs: ");
     *for (it = usedRegs.begin(); it != usedRegs.end(); it++) {
     *   printf("%s ", FPContext::FPReg2Str(*it).c_str());
     *}
     *printf("\n");
     */

         if (usedRegs.find(REG_XMM15) == usedRegs.end()) reg = REG_XMM15;
    else if (usedRegs.find(REG_XMM14) == usedRegs.end()) reg = REG_XMM14;
    else if (usedRegs.find(REG_XMM13) == usedRegs.end()) reg = REG_XMM13;
    else if (usedRegs.find(REG_XMM12) == usedRegs.end()) reg = REG_XMM12;
    else if (usedRegs.find(REG_XMM11) == usedRegs.end()) reg = REG_XMM11;
    else if (usedRegs.find(REG_XMM10) == usedRegs.end()) reg = REG_XMM10;
    else if (usedRegs.find(REG_XMM0) == usedRegs.end())  reg = REG_XMM0;
    else if (usedRegs.find(REG_XMM1) == usedRegs.end())  reg = REG_XMM1;
    else if (usedRegs.find(REG_XMM2) == usedRegs.end())  reg = REG_XMM2;
    else if (usedRegs.find(REG_XMM3) == usedRegs.end())  reg = REG_XMM3;
    else if (usedRegs.find(REG_XMM4) == usedRegs.end())  reg = REG_XMM4;
    else if (usedRegs.find(REG_XMM5) == usedRegs.end())  reg = REG_XMM5;
    else if (usedRegs.find(REG_XMM6) == usedRegs.end())  reg = REG_XMM6;
    else if (usedRegs.find(REG_XMM7) == usedRegs.end())  reg = REG_XMM7;
    else if (usedRegs.find(REG_XMM8) == usedRegs.end())  reg = REG_XMM8;
    else if (usedRegs.find(REG_XMM9) == usedRegs.end())  reg = REG_XMM9;
    if (reg != REG_NONE) {
        usedRegs.insert(reg);
    }
    return reg;
}

bool FPBinaryBlob::isGPR(FPRegister reg)
{
    return (reg >= REG_EAX && reg <= REG_E15);
}

bool FPBinaryBlob::isSSE(FPRegister reg)
{
    return (reg >= REG_XMM0 && reg <= REG_XMM15);
}

void FPBinaryBlob::initialize()
{
    usedRegs.clear();
    inst->getNeededRegisters(usedRegs);
}

void FPBinaryBlob::finalize()
{
}

}

