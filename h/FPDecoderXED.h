#ifndef __FPDECODERXED_H
#define __FPDECODERXED_H

extern "C" {
#include "xed-interface.h"
}

#include "FPDecoder.h"
#include "FPOperation.h"

#include <map>
#include <sstream>

using namespace std;

namespace FPInst {

class FPDecoderXED : public FPDecoder {

    public:

        static FPRegister getX86Reg(FPSemantics *inst);
        static FPRegister getX86RM(FPSemantics *inst);
        static bool readsFlags(FPSemantics *inst);
        static bool writesFlags(FPSemantics *inst);
        static string disassemble(unsigned char *bytes, size_t nbytes);
        static void printInstBytes(FILE *file, unsigned char *bytes, size_t nbytes);

        static string Bytes2Str(unsigned char *bytes, size_t nbytes);

        FPDecoderXED();
        ~FPDecoderXED();

        bool filter(unsigned char *bytes, size_t nbytes);
        FPSemantics* decode(unsigned long iidx, void *addr, unsigned char *bytes, size_t nbytes);
        FPSemantics* lookup(unsigned long iidx);
        FPSemantics* lookupByAddr(void* addr);

    private:

        FPSemantics* build(unsigned long index, void *addr, unsigned char *bytes, size_t nbytes);

        static FPRegister xedReg2FPReg(xed_reg_enum_t reg);

        void expandInstCache(size_t newSize);
        FPSemantics** instCacheArray;
        size_t instCacheSize;

        std::map<void *,unsigned long> iidxByAddr;

};

}

#endif

