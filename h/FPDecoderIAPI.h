#ifndef __FPDECODERIAPI_H
#define __FPDECODERIAPI_H

#include "InstructionDecoder.h"
#include "Instruction.h"
#include "BinaryFunction.h"
#include "Immediate.h"
#include "Register.h"
#include "Dereference.h"

#include "FPContext.h"
#include "FPDecoder.h"
#include "FPOperation.h"

using namespace std;
using namespace Dyninst;
using namespace Dyninst::InstructionAPI;

namespace FPInst {

class FPDecoderIAPI : public FPDecoder, public Visitor {

    public:

        FPDecoderIAPI();
        ~FPDecoderIAPI();

        bool filter(unsigned char *bytes, size_t nbytes);
        FPSemantics* decode(unsigned long iidx, void *addr, unsigned char *bytes, size_t nbytes);
        FPSemantics* lookup(unsigned long iidx);

        void visit(BinaryFunction *b);
        void visit(Immediate *i);
        void visit(RegisterAST *r);
        void visit(Dereference *d);

    private:

        Architecture decoderArch;
        enum OperandDataType {REGISTER, IMMEDIATE, BINARYOP, DEREFERENCE};

        struct OperandData {
            OperandDataType type;
            union {
                FPRegister reg;
                long imm;
                char bop;
            } data;
        } tempOpData[16];
        size_t nTempOpData;

        FPSemantics* build(unsigned long index, void *addr, unsigned char *bytes, size_t nbytes);

        FPOperationType getOpType(string opcode);
        size_t getSize(signed int val);
        FPRegister getRegister(signed int val);
        FPOperand* buildBaseOperand(FPOperandType c_type);
        void addOperands(FPOperation *operation, string opcode, 
                FPOperand *c_op, bool c_read, bool c_write, unsigned idx);

        void printInstBytes(FILE *file, unsigned char *bytes, size_t nbytes);

        void expandInstCache(size_t newSize);

        FPSemantics** instCacheArray;
        size_t instCacheSize;

};

}

#endif

