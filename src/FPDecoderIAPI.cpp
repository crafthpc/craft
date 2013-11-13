#include "FPDecoderIAPI.h"

namespace FPInst {

FPDecoderIAPI::FPDecoderIAPI()
{
#ifdef __X86_64__
    decoderArch = Arch_x86_64;
#else
    decoderArch = Arch_x86;
#endif
    // initialize decoded instruction cache
    instCacheSize = 0;
    instCacheArray = NULL;
    expandInstCache(2000);
}

FPDecoderIAPI::~FPDecoderIAPI()
{
    unsigned i;
    for (i = 0; i < instCacheSize; i++) {
        if (instCacheArray[i])
            delete instCacheArray[i];
    }
    delete instCacheArray;
}

void FPDecoderIAPI::printInstBytes(FILE *file, unsigned char *bytes, size_t nbytes)
{
    unsigned char *ptr;
    unsigned i;
    ptr = (unsigned char*)bytes;
    for (i=0; i<nbytes; i++) {
        fprintf(file, "%02x ", *ptr);
        ptr++;
    }
    fprintf(file, "\n");
}

void FPDecoderIAPI::expandInstCache(size_t newSize) {
    FPSemantics **instCacheTemp;
    unsigned i = 0;
    newSize = newSize > instCacheSize*2 ? newSize : instCacheSize*2;
    //printf("expand_inst_cache - old size: %d    new size: %d\n", instCacheSize, newSize);
    instCacheTemp = (FPSemantics**)malloc(newSize * sizeof(FPSemantics*));
    if (!instCacheTemp) {
        fprintf(stderr, "OUT OF MEMORY!\n");
        exit(-1);
    }
    if (instCacheArray != NULL) {
        for (; i < instCacheSize; i++) {
            instCacheTemp[i] = instCacheArray[i];
        }
        free(instCacheArray);
        instCacheArray = NULL;
    }
    for (; i < newSize; i++) {
        instCacheTemp[i] = NULL;
    }
    instCacheArray = instCacheTemp;
    instCacheSize = newSize;
}

bool FPDecoderIAPI::filter(unsigned char *bytes, size_t /*nbytes*/)
{
    bool add = false;

    if (
                (*bytes) == 0xd8 ||   // single ADD/MUL/COMP/SUB/DIV
                (*bytes) == 0xd9 ||   // single LD/ST
                (*bytes) == 0xda ||   // single integer ADD/MUL/COMP/SUB/DIV
                (*bytes) == 0xdb ||   // single integer LD/ST
                (*bytes) == 0xdc ||   // double ADD/MUL/COMP/SUB/DIV
                (*bytes) == 0xdd ||   // double LD/ST
                (*bytes) == 0xde ||   // double integer ADD/MUL/COMP/SUB/DIV
                (*bytes) == 0xdf      // double integer LD/ST
       ) {
        // x87
        //fprintf(stderr, "x87 at 0x%lx\n", bytes);
        add = true;
    }

    unsigned char *ssePos = 0;
    if (*bytes == 0x0f)
        // scalar single (no prefix)
        ssePos = bytes+1;
    else if (
            // packed double
            (*bytes == 0x66 && (*(bytes+1)) == 0x0f) ||
            // scalar double
            (*bytes == 0xf2 && (*(bytes+1)) == 0x0f) ||
            // scalar single
            (*bytes == 0xf3 && (*(bytes+1)) == 0x0f)
        )
        ssePos = bytes+2;
    if (ssePos != 0) {
        if (
                /*
            *ssePos == 0x58 || *ssePos == 0x5c // just add/sub
                */
            (*ssePos >= 0x10 && *ssePos <=  0x17) ||
            (*ssePos >= 0x20 && *ssePos <=  0x2f) ||
            (*ssePos >= 0x50 && *ssePos <=  0x7f) ||
            (*ssePos >= 0xc2 && *ssePos <=  0xc6) ||
            (*ssePos >= 0xd0 && *ssePos <=  0xfe) ||
            *ssePos == 0x38 || *ssePos == 0x3a
           ) {
            // SSE 1/2
            //fprintf(stderr, "SSE at 0x%lx\n", bytes);
            add = true;
        }
    }

    return add;
}

FPSemantics* FPDecoderIAPI::decode(unsigned long iidx, void *addr, unsigned char *bytes, size_t nbytes)
{
    FPSemantics *inst;
    if (instCacheSize <= iidx) {
        expandInstCache(iidx >= instCacheSize*2 ? iidx+1 : instCacheSize*2);
        inst = build(iidx, addr, bytes, nbytes);
        instCacheArray[iidx] = inst;
    } else {
        inst = instCacheArray[iidx];
        if (inst == NULL) {
            inst = build(iidx, addr, bytes, nbytes);
            instCacheArray[iidx] = inst;
        }
    }
    return inst;
}

FPSemantics* FPDecoderIAPI::lookup(unsigned long iidx)
{
    return instCacheArray[iidx];
}

FPSemantics* FPDecoderIAPI::lookupByAddr(void* /*addr*/)
{
    // TODO: implement
    assert(0);
    return NULL;
}

FPSemantics* FPDecoderIAPI::build(unsigned long index, void *addr, unsigned char *bytes, size_t nbytes)
{
    // misc data structures
    FPSemantics *semantics;
    FPOperation *operation;
    string opcode;

    // current operand data
    Operand op;
    Expression::Ptr expr;
    unsigned c_width;
    FPOperandType c_type;
    FPOperand *c_op;
    bool c_read, c_write;

    // counters
    unsigned i;

    // initialize our framework
    semantics = new FPSemantics();
    operation = new FPOperation();

    // call InstructionAPI to build its internal structures
    // and decode the byte string
    InstructionDecoder* iapiDecoder = new InstructionDecoder(
            (const void*)bytes, nbytes, decoderArch);
    Instruction::Ptr iptr = iapiDecoder->decode();
    
    // save disassembly
    opcode = iptr->getOperation().format();
    printf("decoded assembly: %s [%s]\n", iptr->format().c_str(), 
            opcode.c_str());
    semantics->setDisassembly(iptr->format());
    semantics->setIndex(index);
    semantics->setAddress(addr);
    semantics->setBytes(bytes, nbytes);

    // extract type from opcode
    operation->setType(getOpType(opcode));

    // add extra semantics to handle a x87 stack push
    if (opcode == "fld" || opcode == "fld1" || opcode == "fldz") {
        semantics->add(new FPOperation(OP_PUSH_X87));
    }

    // add each operand individually
    vector<Operand> operands;
    iptr->getOperands(operands);
    for (i=0; i<operands.size(); i++) {
        op = operands[i];
        expr = op.getValue();
        c_width = expr->size();
        c_read = op.isRead();
        c_write = op.isWritten();

        // walk operand struction from IAPI to retrieve immediate/register
        // information
        nTempOpData = 0;
        expr->apply(this);
 
        // debug output
        printf("  operand: %s size=%d %s %s\n", 
                op.format(decoderArch, (Address)addr).c_str(), c_width,
                //op.format().c_str(), c_width,
                (c_read ? "[read]" : ""),
                (c_write ? "[write]" : ""));

        // auto-detected size from IAPI
        if (c_width == 4) {
            c_type = IEEE_Single;
        } else if (c_width == 8) {
            c_type = IEEE_Double;
        } else if (c_width == 16) {
            c_type = C99_LongDouble;
        } else {
            c_type = IEEE_Single;
        }

        // IAPI works only in terms of registers not floating-point numbers, so
        // in the case of SSE instructions we must coerce the operand size
        // based on the opcode, and we must create multiple operands for packed
        // registers.
        
        // First, create the "base register." This will be the final operand for
        // x87 operands and for some SSE operands, and it will be copied with
        // new tags and sizes for the packed SSE operands.

        c_op = buildBaseOperand(c_type);

        // Now we need to add the final operands to the operation. This is a
        // fairly large if/then tree, so it's been given its own function.
        
        addOperands(operation, opcode, c_op, c_read, c_write, i);
    }

    // reverse input operands for select instructions
    if (opcode == "fsubr" || opcode == "fsubrp" || opcode == "fdivr" || opcode == "fdivrp") {
        operation->reverseInputOperands();
    }

    // finalize main operation
    semantics->add(operation);

    // add extra semantics to handle a x87 stack pop
    if (opcode == "fstp" || 
            opcode == "faddp" || opcode == "fsubp" || opcode == "fsubrp" ||
            opcode == "fmulp" || opcode == "fdivp" || opcode == "fdivrp") {
        semantics->add(new FPOperation(OP_PUSH_X87));
    }

    printf("%s\n\n", semantics->toString().c_str());
    return semantics;
}

void FPDecoderIAPI::visit(BinaryFunction * b)
{
    if (b->isAdd()) {
        tempOpData[nTempOpData].type = BINARYOP;
        tempOpData[nTempOpData].data.bop = '+';
        nTempOpData++;
    } else if (b->isMultiply()) {
        tempOpData[nTempOpData].type = BINARYOP;
        tempOpData[nTempOpData].data.bop = '*';
        nTempOpData++;
    }
}

void FPDecoderIAPI::visit(Immediate * i)
{
    Result r = i->eval();
    tempOpData[nTempOpData].type = IMMEDIATE;
    if (r.size() == 1) {
        tempOpData[nTempOpData].data.imm = (long)r.val.s8val;
    } else if (r.size() == 2) {
        tempOpData[nTempOpData].data.imm = (long)r.val.s16val;
    } else if (r.size() == 4) {
        tempOpData[nTempOpData].data.imm = (long)r.val.s32val;
    } else if (r.size() == 8) {
        tempOpData[nTempOpData].data.imm = (long)r.val.s64val;
    } else {
        tempOpData[nTempOpData].data.imm = (long)r.val.s64val;
    }
    nTempOpData++;
}

void FPDecoderIAPI::visit(RegisterAST * r)
{
    MachRegister reg = r->getID();
    signed int val = reg.val();
    
    FPRegister fpreg = REG_NONE;
    if (reg.isPC()) {
        fpreg = REG_EIP;
    } else if (reg.isFramePointer()) {
        fpreg = REG_EBP;
    } else if (reg.isStackPointer()) {
        fpreg = REG_ESP;
    } else {
        fpreg = getRegister(val);
    }

    tempOpData[nTempOpData].type = REGISTER;
    tempOpData[nTempOpData].data.reg = fpreg;
    nTempOpData++;
}

void FPDecoderIAPI::visit(Dereference * /*d*/)
{
    tempOpData[nTempOpData].type = DEREFERENCE;
    nTempOpData++;
}

FPOperationType FPDecoderIAPI::getOpType(string opcode)
{
    FPOperationType type = OP_NONE;
    if (opcode == "fld" || opcode == "fld1" || opcode == "fldz" ||
        opcode == "fst" || opcode == "fstp") {
        type = OP_MOV;

    } else if (opcode == "movss" || opcode == "movsd" || opcode == "movups" || 
               opcode == "movaps" || opcode == "movupd" || opcode == "movapd" ||
               opcode == "unpcklps" || opcode == "unpcklpd" ||
               opcode == "unpckhps" || opcode == "unpckhpd") {
        type = OP_MOV;

    } else if (opcode == "fcom" || 
               opcode == "fcomp" || 
               opcode == "fcompp") {
        type = OP_COM;

    } else if (opcode == "fcomi" || 
               opcode == "fcomip") {
        type = OP_COMI;

    } else if (opcode == "fucom" || opcode == "fucomp" || 
               opcode == "fucompp") {
        type = OP_UCOM;

    } else if (opcode == "fucomi" ||
               opcode == "fucomip") {
        type = OP_UCOMI;

    } else if (opcode == "comiss" || opcode == "comisd") {
        type = OP_COMI;

    } else if (opcode == "ucomiss" || opcode == "ucomisd") {
        type = OP_UCOMI;

    } else if (opcode == "cvtsi2ss" || opcode == "cvtsi2sd" || 
               opcode == "cvtss2si" || opcode == "cvtss2sd" || 
               opcode == "cvtsd2si" || opcode == "cvtsd2ss" || 
               opcode == "cvtpi2ps" || opcode == "cvtpi2pd" || 
               opcode == "cvtps2pi" || opcode == "cvtps2pd" || 
               opcode == "cvtpd2pi" || opcode == "cvtpd2ps") {
        type = OP_CVT;

    } else if (opcode == "cvttss2si" || opcode == "cvttsd2si" ||
               opcode == "cvttps2pi" || opcode == "cvttpd2pi") {
        type = OP_CVT;   // TODO: new enum value for convert+truncate: CVTT?
        
    } else if (opcode == "fadd" || opcode == "faddp" || opcode == "addss" || 
               opcode == "addsd" || opcode == "addps" || opcode == "addpd") {
        type = OP_ADD;

    } else if (opcode == "fsub" || opcode == "fsubr" || opcode == "fsubp" || 
               opcode == "fsubrp" || opcode == "subss" || opcode == "subsd" || 
               opcode == "subps" || opcode == "subpd") {
        type = OP_SUB;

    } else if (opcode == "fmul" || opcode == "fmulp" || opcode == "mulss" || 
               opcode == "mulsd" || opcode == "mulps" || opcode == "mulpd") {
        type = OP_MUL;

    } else if (opcode == "fdiv" || opcode == "fdivr" || opcode == "fdivp" || 
               opcode == "fdivrp" || opcode == "divss" || opcode == "divsd" || 
               opcode == "divps" || opcode == "divpd") {
        type = OP_DIV;

    } else if (opcode == "andps" || opcode == "andpd" || opcode == "pand") {
        type = OP_AND;

    } else if (opcode == "orps" || opcode == "orpd" || opcode == "por") {
        type = OP_OR;

    } else if (opcode == "xorps" || opcode == "xorpd" || opcode == "pxor") {
        type = OP_XOR;

    } else if (opcode == "fsqrt" || opcode == "sqrtss" || opcode == "sqrtsd" || 
               opcode == "sqrtps" || opcode == "sqrtpd") {
        type = OP_SQRT;

    } else if (opcode == "fabs") {
        type = OP_ABS;

    } else if (opcode == "fmin" || opcode == "minss" || opcode == "minsd" ||
               opcode == "minps" || opcode == "minpd") {
        type = OP_MIN;

    } else if (opcode == "fmax" || opcode == "maxss" || opcode == "maxsd" ||
               opcode == "maxps" || opcode == "maxpd") {
        type = OP_MAX;

    } else if (opcode == "fchs") {
        type = OP_NEG;

    } else if (opcode == "fxam") {
        type = OP_AM;
    }
    return type;
}

size_t FPDecoderIAPI::getSize(signed int val)
{
    // x86_64 sizes
    if (val & x86_64::OCT)   { return 128; }
    if (val & x86_64::FPDBL) { return 80; }
    if (val & x86_64::FULL)  { return 64; }
    if (val & x86_64::D_REG) { return 32; }
    if (val & x86_64::W_REG) { return 16; }
    if (val & x86_64::L_REG) { return 8; }
    if (val & x86_64::H_REG) { return 8; }
    if (val & x86_64::BIT)   { return 1; }

    // x86 sizes
    if (val & x86::OCT)   { return 128; }
    if (val & x86::FPDBL) { return 80; }
    if (val & x86::QUAD)  { return 64; }
    if (val & x86::FULL)  { return 32; }
    if (val & x86::W_REG) { return 16; }
    if (val & x86::L_REG) { return 8; }
    if (val & x86::H_REG) { return 8; }
    if (val & x86::BIT)   { return 1; }

    return 0;
}

FPRegister FPDecoderIAPI::getRegister(signed int val)
{
    // x86_64 registers

    if (val == x86_64::rip) { return REG_EIP; }
    if (val == x86_64::rax) { return REG_EAX; }
    if (val == x86_64::rbx) { return REG_EBX; }
    if (val == x86_64::rcx) { return REG_ECX; }
    if (val == x86_64::rdx) { return REG_EDX; }
    if (val == x86_64::rsp) { return REG_ESP; }
    if (val == x86_64::rbp) { return REG_EBP; }
    if (val == x86_64::rsi) { return REG_ESI; }
    if (val == x86_64::rdi) { return REG_EDI; }
    if (val == x86_64::eip) { return REG_EIP; }
    if (val == x86_64::eax) { return REG_EAX; }
    if (val == x86_64::ebx) { return REG_EBX; }
    if (val == x86_64::ecx) { return REG_ECX; }
    if (val == x86_64::edx) { return REG_EDX; }
    if (val == x86_64::esp) { return REG_ESP; }
    if (val == x86_64::ebp) { return REG_EBP; }
    if (val == x86_64::esi) { return REG_ESI; }
    if (val == x86_64::edi) { return REG_EDI; }
    if (val == x86_64::mm0) { return REG_ST0; }
    if (val == x86_64::mm1) { return REG_ST1; }
    if (val == x86_64::mm2) { return REG_ST2; }
    if (val == x86_64::mm3) { return REG_ST3; }
    if (val == x86_64::mm4) { return REG_ST4; }
    if (val == x86_64::mm5) { return REG_ST5; }
    if (val == x86_64::mm6) { return REG_ST6; }
    if (val == x86_64::mm7) { return REG_ST7; }
    if (val == x86_64::xmm0) { return REG_XMM0; }
    if (val == x86_64::xmm1) { return REG_XMM1; }
    if (val == x86_64::xmm2) { return REG_XMM2; }
    if (val == x86_64::xmm3) { return REG_XMM3; }
    if (val == x86_64::xmm4) { return REG_XMM4; }
    if (val == x86_64::xmm5) { return REG_XMM5; }
    if (val == x86_64::xmm6) { return REG_XMM6; }
    if (val == x86_64::xmm7) { return REG_XMM7; }

    // x86 registers
    
    if (val == x86::eip) { return REG_EIP; }
    if (val == x86::eax) { return REG_EAX; }
    if (val == x86::ebx) { return REG_EBX; }
    if (val == x86::ecx) { return REG_ECX; }
    if (val == x86::edx) { return REG_EDX; }
    if (val == x86::esp) { return REG_ESP; }
    if (val == x86::ebp) { return REG_EBP; }
    if (val == x86::esi) { return REG_ESI; }
    if (val == x86::edi) { return REG_EDI; }
    if (val == x86::mm0) { return REG_ST0; }
    if (val == x86::mm1) { return REG_ST1; }
    if (val == x86::mm2) { return REG_ST2; }
    if (val == x86::mm3) { return REG_ST3; }
    if (val == x86::mm4) { return REG_ST4; }
    if (val == x86::mm5) { return REG_ST5; }
    if (val == x86::mm6) { return REG_ST6; }
    if (val == x86::mm7) { return REG_ST7; }
    if (val == x86::xmm0) { return REG_XMM0; }
    if (val == x86::xmm1) { return REG_XMM1; }
    if (val == x86::xmm2) { return REG_XMM2; }
    if (val == x86::xmm3) { return REG_XMM3; }
    if (val == x86::xmm4) { return REG_XMM4; }
    if (val == x86::xmm5) { return REG_XMM5; }
    if (val == x86::xmm6) { return REG_XMM6; }
    if (val == x86::xmm7) { return REG_XMM7; }

    return REG_NONE;
}

FPOperand* FPDecoderIAPI::buildBaseOperand(FPOperandType c_type)
{
    FPOperand *c_op = NULL;
    unsigned i;

    // debug output
    for (i=0; i<nTempOpData; i++) {
        switch (tempOpData[i].type) {
            case REGISTER:
                printf("    REG: %s\n", FPContext::FPReg2Str(tempOpData[i].data.reg).c_str());
                if (tempOpData[i].data.reg == REG_NONE) {
                    printf("      ERROR: invalid register!\n");
                }
                break;
            case IMMEDIATE:
                printf("    IMM: %ld\n", tempOpData[i].data.imm);
                break;
            case BINARYOP:
                printf("    BOP: %c\n", tempOpData[i].data.bop);
                break;
            case DEREFERENCE:
                printf("    DEREF\n");
                break;
        }
    }

    if (nTempOpData == 1 && (tempOpData[0].type == IMMEDIATE || 
                             tempOpData[0].type == REGISTER)) {

        if (tempOpData[0].type == IMMEDIATE) {
            // immediate operands
            // TODO: fix long double and signed/unsigned integer immediates
            if (c_type == IEEE_Single) {
                c_op = new FPOperand(*(float*)&tempOpData[0].data.imm);
            } else if (c_type == IEEE_Double) {
                c_op = new FPOperand(*(double*)&tempOpData[0].data.imm);
            } else if (c_type == C99_LongDouble) {
                c_op = new FPOperand(*(long double*)&tempOpData[0].data.imm);
            }
        } else if (tempOpData[0].type == REGISTER) {
            // register operands
            c_op = new FPOperand(c_type, tempOpData[0].data.reg);
        }

    } else if (nTempOpData == 2 && tempOpData[0].type == REGISTER &&    // base
                                   tempOpData[1].type == DEREFERENCE) {
        // base memory operands
        c_op = new FPOperand(c_type, tempOpData[0].data.reg, 
                REG_NONE, 0, 1);

    } else if (nTempOpData == 4 && tempOpData[0].type == REGISTER &&    // base
                                   tempOpData[1].type == REGISTER &&    // index
                                   tempOpData[2].type == BINARYOP && 
                                   tempOpData[2].data.bop == '+' &&
                                   tempOpData[3].type == DEREFERENCE) {
        // base+index memory operands
        c_op = new FPOperand(c_type, tempOpData[0].data.reg, 
                tempOpData[1].data.reg, 0, 1);

    } else if (nTempOpData == 4 && tempOpData[0].type == REGISTER &&
                                   tempOpData[1].type == IMMEDIATE &&
                                   tempOpData[2].type == BINARYOP && 
                                   tempOpData[2].data.bop == '+' &&
                                   tempOpData[3].type == DEREFERENCE) {
        // base+displacement memory operands
        c_op = new FPOperand(c_type, tempOpData[0].data.reg, 
                REG_NONE, tempOpData[1].data.imm, 1);

    } else if (nTempOpData == 6 && tempOpData[0].type == REGISTER &&    // base
                                   tempOpData[1].type == REGISTER &&    // index
                                   tempOpData[2].type == IMMEDIATE &&   // scale
                                   tempOpData[3].type == BINARYOP && 
                                   tempOpData[3].data.bop == '*' &&
                                   tempOpData[4].type == BINARYOP && 
                                   tempOpData[4].data.bop == '+' &&
                                   tempOpData[5].type == DEREFERENCE) {
        // base+index*scale memory operands
        c_op = new FPOperand(c_type, tempOpData[0].data.reg, 
                tempOpData[1].data.reg, 0, tempOpData[2].data.imm);

    } else if (nTempOpData == 6 && tempOpData[0].type == IMMEDIATE &&   // disp
                                   tempOpData[1].type == REGISTER &&    // index
                                   tempOpData[2].type == IMMEDIATE &&   // scale
                                   tempOpData[3].type == BINARYOP && 
                                   tempOpData[3].data.bop == '*' &&
                                   tempOpData[4].type == BINARYOP && 
                                   tempOpData[4].data.bop == '+' &&
                                   tempOpData[5].type == DEREFERENCE) {
        // index*scale+displacement memory operands
        c_op = new FPOperand(c_type, REG_NONE, tempOpData[1].data.reg, 
                tempOpData[0].data.imm, tempOpData[2].data.imm);

    } else if (nTempOpData == 8 && tempOpData[0].type == REGISTER &&    // base
                                   tempOpData[1].type == REGISTER &&    // index
                                   tempOpData[2].type == IMMEDIATE &&   // scale
                                   tempOpData[3].type == BINARYOP && 
                                   tempOpData[3].data.bop == '*' &&
                                   tempOpData[4].type == BINARYOP && 
                                   tempOpData[4].data.bop == '+' &&
                                   tempOpData[5].type == IMMEDIATE &&   // disp
                                   tempOpData[6].type == BINARYOP && 
                                   tempOpData[6].data.bop == '+' &&
                                   tempOpData[7].type == DEREFERENCE) {
        // base+index*scale+disp memory operands
        c_op = new FPOperand(c_type, tempOpData[0].data.reg, tempOpData[1].data.reg, 
                tempOpData[5].data.imm, tempOpData[2].data.imm);

    } else {
        // TODO:
        // base+index+disp memory operands
        //
        printf("ERROR: unsupported operand!\n");
        c_op = new FPOperand(0);
    }
        
    return c_op;
}

void FPDecoderIAPI::addOperands(FPOperation *operation, string opcode, 
        FPOperand *c_op, bool c_read, bool c_write, unsigned /*idx*/)
{
    // normal x87 (use IAPI-detected size)
    if (opcode == "fld" || opcode == "fld1" || opcode == "fldz" ||
            opcode == "fst" || opcode == "fstp" || 
            opcode == "fcom" || opcode == "fcomi" || opcode == "fucom" || opcode == "fucomi" ||
            opcode == "fcomp" || opcode == "fcomip" || opcode == "fucomp" || opcode == "fucomip" ||
            opcode == "fcompp" || opcode == "fucompp" ||
            opcode == "fadd" || opcode == "fsub" || opcode == "fmul" || opcode == "fdiv" ||
            opcode == "faddp" || opcode == "fsubp" || opcode == "fmulp" || opcode == "fdivp" ||
            opcode == "fsubr" || opcode == "fsubrp" || opcode == "fdivr" || opcode == "fdivrp" ||
            opcode == "fabs" || opcode == "fchs" || opcode == "fsqrt" ||
            opcode == "fxch" || opcode == "fxam" ||
            opcode == "fmin" || opcode == "fmax") {

        if (c_read) operation->addInputOperand(c_op);
        // need to ignore implicit outputs for pop operations; just ignore
        // second outputs for now
        if (c_write && operation->getNumOutputOperands() == 0) operation->addOutputOperand(c_op);

    // normal 32-bit SSE
    } else if (opcode == "movss" || opcode == "comiss" || opcode == "ucomiss" ||
            opcode == "addss" || opcode == "subss" || opcode == "mulss" || opcode == "divss" ||
            opcode == "minss" || opcode == "maxss" || opcode == "sqrtss") {

        c_op->setType(IEEE_Single);
        if (c_read) operation->addInputOperand(c_op);
        if (c_write) operation->addOutputOperand(c_op);

    // normal 64-bit SSE
    } else if (opcode == "movsd" || opcode == "comisd" || opcode == "ucomisd" ||
            opcode == "addsd" || opcode == "subsd" || opcode == "mulsd" || opcode == "divsd" ||
            opcode == "minsd" || opcode == "maxsd" || opcode == "sqrtsd") {

        c_op->setType(IEEE_Double);
        if (c_read) operation->addInputOperand(c_op);
        if (c_write) operation->addOutputOperand(c_op);

    // packed 32-bit SSE
    } else if (opcode == "movups" || opcode == "movaps" ||
            opcode == "addps" || opcode == "subps" || opcode == "mulps" || opcode == "divps" ||
            opcode == "andps" || opcode == "orps" || opcode == "xorps" ||
            opcode == "pand" || opcode == "por" || opcode == "pxor" ||
            opcode == "minps" || opcode == "maxps" || opcode == "sqrtps") {

        c_op->setType(IEEE_Single);
        if (c_read) {
            operation->addInputOperand(c_op);
            operation->addInputOperand(new FPOperand(c_op, 1));
            operation->addInputOperand(new FPOperand(c_op, 2));
            operation->addInputOperand(new FPOperand(c_op, 3));
        }
        if (c_write) {
            operation->addOutputOperand(c_op);
            operation->addOutputOperand(new FPOperand(c_op, 1));
            operation->addOutputOperand(new FPOperand(c_op, 2));
            operation->addOutputOperand(new FPOperand(c_op, 3));
        }
    } else if (opcode == "unpcklps") {
        c_op->setType(IEEE_Single);
        if (c_read) {
            operation->addInputOperand(c_op);
            operation->addInputOperand(new FPOperand(c_op, 1));
        }
        if (c_write) {
            operation->addOutputOperand(c_op);
            operation->addOutputOperand(new FPOperand(c_op, 2));
            operation->addOutputOperand(new FPOperand(c_op, 1));
            operation->addOutputOperand(new FPOperand(c_op, 3));
        }
    } else if (opcode == "unpckhps") {
        c_op->setType(IEEE_Single);
        if (c_read) {
            operation->addInputOperand(new FPOperand(c_op, 2));
            operation->addInputOperand(new FPOperand(c_op, 3));
        }
        if (c_write) {
            operation->addOutputOperand(c_op);
            operation->addOutputOperand(new FPOperand(c_op, 2));
            operation->addOutputOperand(new FPOperand(c_op, 1));
            operation->addOutputOperand(new FPOperand(c_op, 3));
        }

    // packed 64-bit SSE
    } else if (opcode == "movupd" || opcode == "movapd" || 
            opcode == "addpd" || opcode == "subpd" || opcode == "mulpd" || opcode == "divpd" ||
            opcode == "andpd" || opcode == "orpd" || opcode == "xorpd" ||
            opcode == "minpd" || opcode == "maxpd" || opcode == "sqrtpd") {

        c_op->setType(IEEE_Double);
        if (c_read) {
            operation->addInputOperand(c_op);
            operation->addInputOperand(new FPOperand(c_op, 2));
        }
        if (c_write) {
            operation->addOutputOperand(c_op);
            operation->addOutputOperand(new FPOperand(c_op, 2));
        }
    } else if (opcode == "unpcklpd") {
        c_op->setType(IEEE_Double);
        if (c_read) {
            operation->addInputOperand(c_op);
        }
        if (c_write) {
            operation->addOutputOperand(c_op);
            operation->addOutputOperand(new FPOperand(c_op, 2));
        }
    } else if (opcode == "unpckhpd") {
        c_op->setType(IEEE_Double);
        if (c_read) {
            operation->addInputOperand(new FPOperand(c_op, 2));
        }
        if (c_write) {
            operation->addOutputOperand(c_op);
            operation->addOutputOperand(new FPOperand(c_op, 2));
        }

    // normal SSE conversions
    } else if (opcode == "cvtsi2ss" && c_read) {
        c_op->setType(c_op->getType() == IEEE_Single ? SignedInt32 : SignedInt64); 
        operation->addInputOperand(c_op);
    } else if (opcode == "cvtsi2ss" && c_write) {
        c_op->setType(IEEE_Single);
        operation->addOutputOperand(c_op);
    } else if (opcode == "cvtsi2sd" && c_read) {
        c_op->setType(c_op->getType() == IEEE_Single ? SignedInt32 : SignedInt64); 
        operation->addInputOperand(c_op);
    } else if (opcode == "cvtsi2sd" && c_write) {
        c_op->setType(IEEE_Double);
        operation->addOutputOperand(c_op);
    } else if (opcode == "cvtss2si" && c_read) {
        c_op->setType(IEEE_Single);
        operation->addInputOperand(c_op);
    } else if (opcode == "cvtss2si" && c_write) {
        c_op->setType(c_op->getType() == IEEE_Single ? SignedInt32 : SignedInt64); 
        operation->addOutputOperand(c_op);
    } else if (opcode == "cvtss2sd" && c_read) {
        c_op->setType(IEEE_Single); 
        operation->addInputOperand(c_op);
    } else if (opcode == "cvtss2sd" && c_write) {
        c_op->setType(IEEE_Double); 
        operation->addOutputOperand(c_op);
    } else if (opcode == "cvtsd2si" && c_read) {
        c_op->setType(IEEE_Double);
        operation->addInputOperand(c_op);
    } else if (opcode == "cvtsd2si" && c_write) {
        c_op->setType(c_op->getType() == IEEE_Single ? SignedInt32 : SignedInt64); 
        operation->addOutputOperand(c_op);
    } else if (opcode == "cvtsd2ss" && c_read) {
        c_op->setType(IEEE_Double); 
        operation->addInputOperand(c_op);
    } else if (opcode == "cvtsd2ss" && c_write) {
        c_op->setType(IEEE_Single);
        operation->addOutputOperand(c_op);

    } else if (opcode == "cvttss2si" && c_read) {
        c_op->setType(IEEE_Single);
        operation->addInputOperand(c_op);
    } else if (opcode == "cvttss2si" && c_write) {
        c_op->setType(c_op->getType() == IEEE_Single ? SignedInt32 : SignedInt64); 
        operation->addOutputOperand(c_op);
    } else if (opcode == "cvttsd2si" && c_read) {
        c_op->setType(IEEE_Double);
        operation->addInputOperand(c_op);
    } else if (opcode == "cvttsd2si" && c_write) {
        c_op->setType(c_op->getType() == IEEE_Single ? SignedInt32 : SignedInt64); 
        operation->addOutputOperand(c_op);


    // TODO: packed SSE conversions
    //} else if (opcode == "cvtpi2ps" && c_read) {
        //c_op->setType(c_op->getType() == IEEE_Single ? SignedInt32 : SignedInt64); 
        //operation->addInputOperand(c_op);
    //} else if (opcode == "cvtpi2ps" && c_write) {
        //c_op->setType(IEEE_Single);
        //operation->addOutputOperand(c_op);
    //} else if (opcode == "cvtpi2pd" && c_read) {
        //c_op->setType(c_op->getType() == IEEE_Single ? SignedInt32 : SignedInt64); 
        //operation->addInputOperand(c_op);
    //} else if (opcode == "cvtpi2pd" && c_write) {
        //c_op->setType(IEEE_Double);
        //operation->addOutputOperand(c_op);
    //} else if (opcode == "cvtps2pi" && c_read) {
        //c_op->setType(IEEE_Single);
        //operation->addInputOperand(c_op);
    //} else if (opcode == "cvtps2pi" && c_write) {
        //c_op->setType(c_op->getType() == IEEE_Single ? SignedInt32 : SignedInt64); 
        //operation->addOutputOperand(c_op);
    } else if (opcode == "cvtps2pd" && c_read) {
        c_op->setType(IEEE_Single); 
        operation->addInputOperand(c_op);
        operation->addInputOperand(new FPOperand(c_op, 1));
    } else if (opcode == "cvtps2pd" && c_write) {
        c_op->setType(IEEE_Double); 
        operation->addOutputOperand(c_op);
        operation->addOutputOperand(new FPOperand(c_op, 2));
    //} else if (opcode == "cvtpd2pi" && c_read) {
        //c_op->setType(IEEE_Double);
        //operation->addInputOperand(c_op);
    //} else if (opcode == "cvtpd2pi" && c_write) {
        //c_op->setType(c_op->getType() == IEEE_Single ? SignedInt32 : SignedInt64); 
        //operation->addOutputOperand(c_op);
    } else if (opcode == "cvtpd2ps" && c_read) {
        c_op->setType(IEEE_Double); 
        operation->addInputOperand(c_op);
        operation->addInputOperand(new FPOperand(c_op, 2));
    } else if (opcode == "cvtpd2ps" && c_write) {
        c_op->setType(IEEE_Single);
        operation->addOutputOperand(c_op);
        operation->addOutputOperand(new FPOperand(c_op, 1));

    //} else if (opcode == "cvttps2pi" && c_read) {
        //c_op->setType(IEEE_Single);
        //operation->addInputOperand(c_op);
    //} else if (opcode == "cvttps2pi" && c_write) {
        //c_op->setType(c_op->getType() == IEEE_Single ? SignedInt32 : SignedInt64); 
        //operation->addOutputOperand(c_op);
    //} else if (opcode == "cvttpd2pi" && c_read) {
        //c_op->setType(IEEE_Double);
        //operation->addInputOperand(c_op);
    //} else if (opcode == "cvttpd2pi" && c_write) {
        //c_op->setType(c_op->getType() == IEEE_Single ? SignedInt32 : SignedInt64); 
        //operation->addOutputOperand(c_op);

    } else {
        printf("ERROR: unsupported instruction: %s\n", opcode.c_str());
    }
}

}

