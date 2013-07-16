#ifndef __FPANALYSISRPREC_H
#define __FPANALYSISRPREC_H

#include "FPBinaryBlob.h"
#include "FPCodeGen.h"
#include "FPConfig.h"
#include "FPAnalysis.h"

namespace FPInst {

/**
 * Stores helper data for a single instruction.
 */
struct FPAnalysisRPrecInstData {
    FPSemantics *inst;
    unsigned long precision;
    unsigned long count;
    void *count_addr;
};

/**
 * Handles code generation for a reduced-precision blob.
 */
class FPBinaryBlobRPrec : public FPBinaryBlob, public Snippet {

    public:

        FPBinaryBlobRPrec(FPSemantics *inst, FPAnalysisRPrecInstData instData);

        void enableLockPrefix();
        void disableLockPrefix();

        bool generate(Point *pt, Buffer &buf);

    private:

        uint64_t rprecConst64[52+1];
        uint32_t rprecConst32[23+1];
        
        FPAnalysisRPrecInstData instData;
        bool useLockPrefix;

};

/**
 * Performs reduced precision analysis by truncating the output of arithmetic
 * instructions to a desired precision level.
 *
 *   half-precision   (16 bits) = 10+1 bits of significant
 *   single-precision (32 bits) = 23+1 bits of significand
 *   double-precision (64 bits) = 52+1 bits of significand
 *
 * The "precision" used here is out of the above numbers (excluding the "+1" for
 * the implicit 1).
 *
 * Thus, a precision level of "52" indicates full double precision, while a
 * precision level of "23" indicates full single precision. A precision level of
 * "51" would indicate double precision with a single bit truncated (set to
 * zero), while a precision level of "22" would indicate either double precision
 * with 52-22 = 30 truncated bits or single precision with a single truncated
 * bit.
 *
 * Precision levels below 4 represent less than a single decimal digit of
 * precision and are probably not useful. The most common range for the analysis
 * of individual instructions will be (23, 52]; in this case, the program runs
 * fine when the instruction runs in double precision but fails when it runs in
 * single precision. Further searching should determine the lowest precision
 * level that causes the program to succeed.
 *
 * This is not meant to be a substitute for the inplace analysis
 * (FPAnalysisInplace), which simulates replacing the entire instruction with
 * all of its operands. Rather, it is intended to be used in conjunction with an
 * automated search to give a cheap approximation of how many bits of precision
 * an instruction needs to output in order for the rest of the program to
 * succeed.
 *
 */
class FPAnalysisRPrec : public FPAnalysis {

    public:

        static bool existsInstance();
        static FPAnalysisRPrec* getInstance();

        string getTag();
        string getDescription();

        void configure(FPConfig *config, FPDecoder *decoder,
                FPLog *log, FPContext *context);

        bool isSupportedOp(FPOperation *op);

        void enableLockPrefix();
        void disableLockPrefix();

        bool shouldPreInstrument(FPSemantics *inst);
        bool shouldPostInstrument(FPSemantics *inst);
        bool shouldReplace(FPSemantics *inst);

        Snippet::Ptr buildPreInstrumentation(FPSemantics *inst,
                BPatch_addressSpace *app, bool &needsRegisters);
        Snippet::Ptr buildPostInstrumentation(FPSemantics *inst,
                BPatch_addressSpace *app, bool &needsRegisters);
        Snippet::Ptr buildReplacementCode(FPSemantics *inst,
                BPatch_addressSpace *app, bool &needsRegisters);

        string finalInstReport();

        void registerInstruction(FPSemantics *inst);
        void handlePreInstruction(FPSemantics *inst);
        void handlePostInstruction(FPSemantics *inst);
        void handleReplacement(FPSemantics *inst);

        void reducePrecision(FPSemantics *inst, FPOperand *op);

        void finalOutput();

    private:

        FPAnalysisRPrec();

        bool useLockPrefix;

        void expandInstData(size_t newSize);
        FPAnalysisRPrecInstData *instData;
        size_t instCount;

        unsigned long defaultPrecision;

        size_t insnsInstrumented;
};

}

#endif

