#include "FPCodeGen.h"

namespace FPInst {

FPCodeGen::FPCodeGen()
{ }

unsigned char FPCodeGen::getRegModRMId(FPRegister reg)
{
    unsigned char tmp = 0x0;
    switch (reg) {
        case REG_NONE:
        case REG_XMM0:  case REG_EAX:   tmp = 0x0;  break;
        case REG_XMM1:  case REG_ECX:   tmp = 0x1;  break;
        case REG_XMM2:  case REG_EDX:   tmp = 0x2;  break;
        case REG_XMM3:  case REG_EBX:   tmp = 0x3;  break;
        case REG_XMM4:  case REG_ESP:   tmp = 0x4;  break;

        case REG_EIP:   // for %rip-based addressing;
                        // requires special handling in
                        // buildInstruction()
                        
        case REG_XMM5:  case REG_EBP:   tmp = 0x5;  break;
        case REG_XMM6:  case REG_ESI:   tmp = 0x6;  break;
        case REG_XMM7:  case REG_EDI:   tmp = 0x7;  break;
        case REG_XMM8:  case REG_E8:    tmp = 0x8;  break;
        case REG_XMM9:  case REG_E9:    tmp = 0x9;  break;
        case REG_XMM10: case REG_E10:   tmp = 0xA;  break;
        case REG_XMM11: case REG_E11:   tmp = 0xB;  break;
        case REG_XMM12: case REG_E12:   tmp = 0xC;  break;
        case REG_XMM13: case REG_E13:   tmp = 0xD;  break;
        case REG_XMM14: case REG_E14:   tmp = 0xE;  break;
        case REG_XMM15: case REG_E15:   tmp = 0xF;  break;
        default: assert(!"unsupported register");   break;
    }
    return tmp;
}

size_t FPCodeGen::buildREX(unsigned char *pos,
        bool wide, FPRegister reg, FPRegister index, FPRegister base_rm)
{
    /*
     *printf("building REX byte (SIB=%s,%s,%s): ",
     *        FPContext::FPReg2Str(reg).c_str(),
     *        FPContext::FPReg2Str(index).c_str(),
     *        FPContext::FPReg2Str(base_rm).c_str());
     */

    unsigned char rex = 0x0;

    if (wide) {
        rex |= 0x8;         // set REX.W bit
    }
    if (getRegModRMId(reg) > 0x7) {
        rex |= 0x4;         // set REX.R bit
    }
    if (getRegModRMId(index) > 0x7) {
        rex |= 0x2;         // set REX.X bit
    }
    if (getRegModRMId(base_rm) > 0x7) {
        rex |= 0x1;         // set REX.B bit
    }

    if (rex > 0) {
        // emit REX
        rex |= 0x40;
        (*pos++) = rex;
        return 1;
    } else {
        // ignore
        return 0;
    }
}

size_t FPCodeGen::buildModRM(unsigned char *pos,
        FPRegister reg, FPRegister rm)
{
    return buildModRM(pos, 0x3, reg, rm);
}

size_t FPCodeGen::buildModRM(unsigned char *pos,
        unsigned char mod, FPRegister reg, FPRegister rm)
{
    unsigned char modRM = 0x0 | ((mod & 0x3) << 6);

    // high bits are encoded in the REX byte, so strip them
    unsigned char reg_enc = getRegModRMId(reg) & 0x7;
    unsigned char rm_enc =  getRegModRMId(rm) & 0x7;

    modRM |= (reg_enc << 3);
    modRM |= (rm_enc);

    (*pos++) = modRM;
    return 1;
}

size_t FPCodeGen::buildInstruction(unsigned char *pos,
        unsigned char prefix, bool wide_operands, bool two_byte_opcode,
        unsigned char opcode, FPRegister reg)
{
    unsigned char *old_pos = pos;
    unsigned char reg_enc = (getRegModRMId(reg) & 0x7);

    // emit instruction
    if (prefix) {
        (*pos++) = prefix;
    }
    pos += buildREX(pos, wide_operands, REG_NONE, REG_NONE, reg);
    if (two_byte_opcode) {
        (*pos++) = 0x0f;
    }
    (*pos++) = (opcode | reg_enc);

    return (size_t)(pos-old_pos);
}

size_t FPCodeGen::buildInstruction(unsigned char *pos,
        unsigned char prefix, bool wide_operands, bool two_byte_opcode,
        unsigned char opcode, FPRegister reg, FPRegister rm,
        bool memory, int32_t disp)
{
    unsigned char *old_pos = pos;
    unsigned char modrm;
    unsigned char reg_enc = (getRegModRMId(reg) & 0x7);
    unsigned char rm_enc = (getRegModRMId(rm) & 0x7);

    if (memory && (
        rm == REG_NONE ||
        rm == REG_ESP ||
        rm == REG_E12 ||
        rm == REG_EBP ||
        rm == REG_E13)) {
        return buildInstruction(pos, prefix, wide_operands, two_byte_opcode,
                opcode, reg, 1, REG_NONE, rm, disp);
    }

    // assemble Mod/RM byte
    // TODO: check all this....
    if (rm == REG_NONE) {
        modrm = 0x04;
    } else if (rm == REG_ESP) {
        modrm = disp ? 0x84 : 0x04;
    } else if (rm == REG_EIP) {
        modrm = 0x05;
    } else if (rm == REG_E13) {
        modrm = 0x40 | (rm_enc);
    } else {
        modrm = (memory ? (disp ? 0x80 : 0x0) : 0xc0) | 
                   (rm_enc);
    }
    modrm |= (reg_enc << 3);

    // emit instruction
    if (prefix) {
        (*pos++) = prefix;
    }
    pos += buildREX(pos, wide_operands, reg, REG_NONE, rm);
    if (two_byte_opcode) {
        (*pos++) = 0x0f;
    }
    (*pos++) = opcode;
    (*pos++) = modrm;
    if (rm == REG_NONE) {
        // SIB byte for zero-based displacement
        (*pos++) = 0x25;
    } else if (rm == REG_ESP) {
        // SIB byte for RSP-based displacement
        (*pos++) = 0x24;
    }
    if (memory && disp) {
        *(int32_t*)pos = disp;
        pos += 4;
    } else if (modrm == REG_E13) {
        // one-byte zero displacement for (%r13)
        (*pos++) = 0x0;
    }

    return (size_t)(pos-old_pos);
}

size_t FPCodeGen::buildInstruction(unsigned char *pos,
        unsigned char prefix, bool wide_operands, bool two_byte_opcode,
        unsigned char opcode, FPRegister reg, 
        long scale, FPRegister index, FPRegister base, int32_t disp)
{
    unsigned char *old_pos = pos;
    unsigned char modrm, sib;
    unsigned char reg_enc = (getRegModRMId(reg) & 0x7);
    unsigned char index_enc = 
        (index == REG_NONE ? 0x4 : (getRegModRMId(index) & 0x7));
    unsigned char base_enc = 
        (base  == REG_NONE ? 0x5 : (getRegModRMId(base) & 0x7));

    // assemble Mod/RM byte
    if (disp && base != REG_NONE) {
        modrm = 0x84;   // mod=10
    } else if (base == REG_EBP || base == REG_E13) {
        modrm = 0x44;   // mod=01
    } else {
        modrm = 0x04;   // mod=00
    }
    modrm |= (reg_enc << 3);

    // assemble SIB byte
    sib = 0x0;
    switch (scale) {
        case 1:     sib = (0x0 << 6);  break;
        case 2:     sib = (0x1 << 6);  break;
        case 4:     sib = (0x2 << 6);  break;
        case 8:     sib = (0x3 << 6);  break;
        default:    assert(!"unsupported scale");    break;
    }
    sib |= (index_enc << 3);
    sib |= base_enc;

    // emit instruction
    if (prefix) {
        (*pos++) = prefix;
    }
    pos += buildREX(pos, wide_operands, reg, index, base);
    if (two_byte_opcode) {
        (*pos++) = 0x0f;
    }
    (*pos++) = opcode;
    (*pos++) = modrm;
    (*pos++) = sib;
    if (disp) {
        *(int32_t*)pos = disp;
        pos += 4;
    } else if (base == REG_EBP || base == REG_E13) {
        // one-byte zero displacement when RBP/R13 is the base register
        *(int8_t*)pos = 0;
        pos += 1;
    }

    return (size_t)(pos-old_pos);
}

size_t FPCodeGen::buildPushReg(unsigned char *pos,
        FPRegister reg)
{
    assert(reg >= REG_EAX && reg <= REG_EDI);
    (*pos++) = 0x50+getRegModRMId(reg); // push %reg
    return 1;
}

size_t FPCodeGen::buildPopReg(unsigned char *pos,
        FPRegister reg)
{
    assert(reg >= REG_EAX && reg <= REG_EDI);
    (*pos++) = 0x58+getRegModRMId(reg); // pop %reg
    return 1;
}

size_t FPCodeGen::buildPushFlags(unsigned char *pos)
{
    *(pos++) = 0x9c;            // pushf
    return 1;
}

size_t FPCodeGen::buildPopFlags(unsigned char *pos)
{
    *(pos++) = 0x9d;            // popf
    return 1;
}

size_t FPCodeGen::buildPushFlagsFast(unsigned char *pos)
{
    (*pos++) = 0x9f;            // lahf
    (*pos++) = 0x0f;            // seto %al (save OF)
    (*pos++) = 0x90;
    (*pos++) = 0xc0;
    (*pos++) = 0x50;            // push %rax
    return 1+3+1;
}

size_t FPCodeGen::buildPopFlagsFast(unsigned char *pos)
{
    (*pos++) = 0x58;            // pop %rax
    (*pos++) = 0x80;            // add 0x7f, %al (restore OF)
    (*pos++) = 0xc0;
    (*pos++) = 0x7f;
    (*pos++) = 0x9e;            // sahf
    return 1+3+1;
}

size_t FPCodeGen::buildSaveFlagsFast(unsigned char *pos, int32_t sp_offset, bool fixByteOrder)
{
    // NOTE: assumes we can clobber RAX
    //
    unsigned char *old_pos = pos;
    pos += buildMovImm64ToGPR64(pos, 0x0, REG_EAX);
    (*pos++) = 0x9f;            // lahf
    (*pos++) = 0x0f;            // seto %al (save OF)
    (*pos++) = 0x90;
    (*pos++) = 0xc0;
    if (fixByteOrder) {
        (*pos++) = 0x86;        // xchg %al, %ah
        (*pos++) = 0xc4;
    }
    pos += buildInstruction(pos, 0, true, false, 
            0x89, REG_EAX, REG_ESP, true, sp_offset);
    return (size_t)(pos-old_pos);
}

size_t FPCodeGen::buildRestoreFlagsFast(unsigned char *pos, int32_t sp_offset, bool fixByteOrder)
{
    // NOTE: assumes we can clobber RAX
    //
    unsigned char *old_pos = pos;
    pos += buildInstruction(pos, 0, true, false, 
            0x8b, REG_EAX, REG_ESP, true, sp_offset);
    if (fixByteOrder) {
        (*pos++) = 0x86;        // xchg %al, %ah
        (*pos++) = 0xc4;
    }
    (*pos++) = 0x80;            // add 0x7f, %al (restore OF)
    (*pos++) = 0xc0;
    (*pos++) = 0x7f;
    (*pos++) = 0x9e;            // sahf
    return (size_t)(pos-old_pos);
}

size_t FPCodeGen::buildMovXmmToGPR64(unsigned char *pos,
        FPRegister xmm, FPRegister gpr)
{
    return buildInstruction(pos, 0x66, true, true, 0x7e, xmm, gpr, false, 0);
}

size_t FPCodeGen::buildMovGPR64ToXmm(unsigned char *pos,
        FPRegister gpr, FPRegister xmm)
{
    return buildInstruction(pos, 0x66, true, true, 0x6e, xmm, gpr, false, 0);
}

size_t FPCodeGen::buildMovGPR64ToGPR64(unsigned char *pos,
        FPRegister gpr_src, FPRegister gpr_dest)
{
    return buildInstruction(pos, 0x0, true, false, 0x89, gpr_src, gpr_dest, false, 0);
}

size_t FPCodeGen::buildMovGPR64ToStack(unsigned char *pos,
        FPRegister gpr_src, uint32_t offset)
{
    return buildInstruction(pos, 0x0, true, false, 0x89, gpr_src, REG_ESP, true, offset);
}

size_t FPCodeGen::buildMovStackToGPR64(unsigned char *pos,
        FPRegister gpr_dest, uint32_t offset)
{
    return buildInstruction(pos, 0x0, true, false, 0x8b, gpr_dest, REG_ESP, true, offset);
}

size_t FPCodeGen::buildCvtss2sd(unsigned char *pos,
        FPRegister reg1, FPRegister reg2)
{
    return buildInstruction(pos, 0xf3, false, true, 0x5a, reg1, reg2, false, 0);
}

size_t FPCodeGen::buildCvtps2pd(unsigned char *pos,
        FPRegister reg1, FPRegister reg2)
{
    return buildInstruction(pos,    0, false, true, 0x5a, reg1, reg2, false, 0);
}

size_t FPCodeGen::buildCvtsd2ss(unsigned char *pos,
        FPRegister reg1, FPRegister reg2)
{
    return buildInstruction(pos, 0xf2, false, true, 0x5a, reg1, reg2, false, 0);
}

size_t FPCodeGen::buildCvtpd2ps(unsigned char *pos,
        FPRegister reg1, FPRegister reg2)
{
    return buildInstruction(pos, 0x66, false, true, 0x5a, reg1, reg2, false, 0);
}

size_t FPCodeGen::buildUnpcklps(unsigned char *pos,
        FPRegister reg1, FPRegister reg2)
{
    return buildInstruction(pos, 0, false, true, 0x14, reg1, reg2, false, 0);
}

size_t FPCodeGen::buildMovImm32ToGPR32(unsigned char *pos,
        uint32_t imm, FPRegister gpr)
{
    assert(gpr >= REG_EAX && gpr <= REG_EDI);
    (*pos++) = 0xb8 | getRegModRMId(gpr);            // mv $imm,$gpr
    *(uint32_t*)pos = imm;
    pos += 4;
    return 5;
}

size_t FPCodeGen::buildMovImm32ToStack(unsigned char *pos,
        uint32_t imm, int32_t offset)
{
    unsigned char *old_pos = pos;
    pos += buildInstruction(pos, 0, false, false, 0xc7, REG_NONE, REG_ESP, true, offset);
    *(uint32_t*)pos = imm;
    pos += 4;
    return (size_t)(pos-old_pos);
}

size_t FPCodeGen::buildMovImm64ToGPR64(unsigned char *pos,
        uint64_t imm, FPRegister gpr)
{
    assert(gpr >= REG_EAX && gpr <= REG_E15);
    // mv $imm,$gpr
    pos += buildREX(pos, true, REG_NONE, REG_NONE, gpr);
    (*pos++) = 0xb8 | getRegModRMId(gpr);
    *(uint64_t*)pos = imm;
    pos += 8;
    return 10;
}

size_t FPCodeGen::buildAndGPR64WithGPR64(unsigned char *pos, FPRegister reg, FPRegister rm)
{
    // and %rm, %reg   (%rm = %rm & %reg)
    assert(rm >= REG_EAX && rm <= REG_E15);
    assert(reg >= REG_EAX && reg <= REG_E15);
    return buildInstruction(pos, 0, true, false,
            0x21, reg, rm, false, 0);
}

size_t FPCodeGen::buildOrGPR64WithGPR64(unsigned char *pos, FPRegister reg, FPRegister rm)
{
    // and %rm, %reg   (%rm = %rm & %reg)
    assert(rm >= REG_EAX && rm <= REG_E15);
    assert(reg >= REG_EAX && reg <= REG_E15);
    return buildInstruction(pos, 0, true, false,
            0x09, reg, rm, false, 0);
}

size_t FPCodeGen::buildXorGPR64WithGPR64(unsigned char *pos, FPRegister reg, FPRegister rm)
{
    // and %rm, %reg   (%rm = %rm & %reg)
    assert(rm >= REG_EAX && rm <= REG_E15);
    assert(reg >= REG_EAX && reg <= REG_E15);
    return buildInstruction(pos, 0, true, false,
            0x31, reg, rm, false, 0);
}

size_t FPCodeGen::buildCmpGPR64WithGPR64(unsigned char *pos, FPRegister rm, FPRegister reg)
{
    // cmp %rm, %reg
    assert(rm >= REG_EAX && rm <= REG_E15);
    assert(reg >= REG_EAX && reg <= REG_E15);
    return buildInstruction(pos, 0, true, false,
            0x39, reg, rm, false, 0);
}

size_t FPCodeGen::buildAddGPR64ToGPR64(unsigned char *pos, FPRegister reg, FPRegister rm)
{
    // add %rm, %reg   (%rm = %rm + %reg)
    assert(rm >= REG_EAX && rm <= REG_E15);
    assert(reg >= REG_EAX && reg <= REG_E15);
    return buildInstruction(pos, 0, true, false,
            0x01, reg, rm, false, 0);
}

size_t FPCodeGen::buildIncMem64(unsigned char *pos, int32_t offset)
{
    return buildInstruction(pos, 0, true, false,
            0xff, REG_NONE, REG_NONE, true, offset);
}

size_t FPCodeGen::buildJumpNear32(unsigned char *pos,
        int32_t offset, int32_t* &offset_pos)
{
    (*pos++) = 0xe9;            // jmp offset(%rip)
    offset_pos = (int32_t*)pos;
    *offset_pos = offset;
    pos += 1;
    return 5;
}

size_t FPCodeGen::buildJumpEqualNear32(unsigned char *pos,
        int32_t offset, int32_t* &offset_pos)
{
    (*pos++) = 0x0f;            // je offset(%rip)
    (*pos++) = 0x84;
    offset_pos = (int32_t*)pos;
    *offset_pos = offset;
    pos += 1;
    return 6;
}

size_t FPCodeGen::buildJumpNotEqualNear32(unsigned char *pos,
        int32_t offset, int32_t* &offset_pos)
{
    (*pos++) = 0x0f;            // jne offset(%rip)
    (*pos++) = 0x85;
    offset_pos = (int32_t*)pos;
    *offset_pos = offset;
    pos += 1;
    return 6;
}

size_t FPCodeGen::buildJumpLessNear32(unsigned char *pos,
        int32_t offset, int32_t* &offset_pos)
{
    (*pos++) = 0x0f;            // jb offset(%rip)
    (*pos++) = 0x82;
    offset_pos = (int32_t*)pos;
    *offset_pos = offset;
    pos += 1;
    return 6;
}

size_t FPCodeGen::buildJumpGreaterEqualNear32(unsigned char *pos,
        int32_t offset, int32_t* &offset_pos)
{
    (*pos++) = 0x0f;            // jae offset(%rip)
    (*pos++) = 0x83;
    offset_pos = (int32_t*)pos;
    *offset_pos = offset;
    pos += 1;
    return 6;
}

size_t FPCodeGen::buildJumpLessEqualNear32(unsigned char *pos,
        int32_t offset, int32_t* &offset_pos)
{
    (*pos++) = 0x0f;            // jbe offset(%rip)
    (*pos++) = 0x86;
    offset_pos = (int32_t*)pos;
    *offset_pos = offset;
    pos += 1;
    return 6;
}

size_t FPCodeGen::buildNop(unsigned char *pos)
{
    (*pos++) = 0x90;            // nop
    return 1;
}

size_t FPCodeGen::buildInsertGPR32IntoXMM(unsigned char *pos,
        FPRegister gpr, FPRegister xmm, long tag)
{
    assert(gpr >= REG_EAX && gpr <= REG_EDI);
    assert(xmm >= REG_XMM0 && xmm <= REG_XMM15);
    assert(tag >= 0 && tag <= 3);
    unsigned char *old_pos = pos;
    (*pos++) = 0x66;    // pinsrd $tag, %gpr, %xmm
    pos += buildREX(pos, false, xmm, REG_NONE, gpr);
    (*pos++) = 0x0f;
    (*pos++) = 0x3a;
    (*pos++) = 0x22;
    pos += buildModRM(pos, xmm, gpr);
    *(int8_t*)pos = (int8_t)tag;    pos++;
    return (size_t)(pos-old_pos);
}

size_t FPCodeGen::buildExtractGPR64FromXMM(unsigned char *pos,
        FPRegister gpr, FPRegister xmm, long tag)
{
    assert(gpr >= REG_EAX && gpr <= REG_E15);
    assert(xmm >= REG_XMM0 && xmm <= REG_XMM15);
    assert(tag == 0 || tag == 2);
    if (tag == 2) tag = 1;   // fix tag (instruction expects 0 or 1)
    unsigned char *old_pos = pos;
    (*pos++) = 0x66;
    pos += buildREX(pos, true, xmm, REG_NONE, gpr);
    (*pos++) = 0x0f;
    (*pos++) = 0x3a;
    (*pos++) = 0x16;
    pos += buildModRM(pos, xmm, gpr);
    *(int8_t*)pos = (int8_t)tag;    pos++;
    return (size_t)(pos-old_pos);
}

size_t FPCodeGen::buildInsertGPR64IntoXMM(unsigned char *pos,
        FPRegister gpr, FPRegister xmm, long tag)
{
    assert(gpr >= REG_EAX && gpr <= REG_E15);
    assert(xmm >= REG_XMM0 && xmm <= REG_XMM15);
    assert(tag == 0 || tag == 2);
    if (tag == 2) tag = 1;   // fix tag (instruction expects 0 or 1)
    unsigned char *old_pos = pos;
    (*pos++) = 0x66;
    pos += buildREX(pos, true, xmm, REG_NONE, gpr);
    (*pos++) = 0x0f;
    (*pos++) = 0x3a;
    (*pos++) = 0x22;
    pos += buildModRM(pos, xmm, gpr);
    *(int8_t*)pos = (int8_t)tag;    pos++;
    return (size_t)(pos-old_pos);
}

size_t FPCodeGen::buildSPAdjustment(unsigned char *pos,
        int8_t offset)
{
    (*pos++) = 0x48;            // lea offset(%rsp), %rsp
    (*pos++) = 0x8d;
    (*pos++) = 0x64;
    (*pos++) = 0x24;
    *(int8_t*)pos = offset; pos += 1;
    return 5;
}

size_t FPCodeGen::buildSPAdjustment(unsigned char *pos,
        int32_t offset)
{
    (*pos++) = 0x48;            // lea offset(%rsp), %rsp
    (*pos++) = 0x8d;
    (*pos++) = 0xa4;
    (*pos++) = 0x24;
    *(int32_t*)pos = offset; pos += 4;
    return 8;
}

size_t FPCodeGen::buildDie(unsigned char *pos)
{
    (*pos++) = 0xcd;            // int 0x3
    (*pos++) = 0x03;
    return 2;
}

}

