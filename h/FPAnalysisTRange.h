#ifndef __FPANALYSISTRANGE_H
#define __FPANALYSISTRANGE_H

#include "FPBinaryBlob.h"
#include "FPCodeGen.h"
#include "FPConfig.h"
#include "FPAnalysis.h"

namespace FPInst {

/**
 * Stores range data for a single instruction.
 */
struct FPAnalysisTRangeInstData {
    FPSemantics *inst;
    long double min, max;
    unsigned long count;
    void *min_addr, *max_addr, *count_addr;
};

class FPBinaryBlobTRange : public FPBinaryBlob, public Snippet {

    public:

        FPBinaryBlobTRange(FPSemantics *inst, FPAnalysisTRangeInstData instData, bool preInsn);

        bool generate(Point *pt, Buffer &buf);

    private:

        FPAnalysisTRangeInstData instData;
        bool preInsn;
};

/**
 * Performs range tracking analysis.
 */
class FPAnalysisTRange : public FPAnalysis {

    public:

        static bool existsInstance();
        static FPAnalysisTRange* getInstance();

        string getTag();
        string getDescription();

        void configure(FPConfig *config, FPDecoder *decoder,
                FPLog *log, FPContext *context);

        void configAddresses(FPConfig *config);
        bool isRestrictedByAddress();
        string listAddresses();

        bool shouldPreInstrument(FPSemantics *inst);
        bool shouldPostInstrument(FPSemantics *inst);
        bool shouldReplace(FPSemantics *inst);

        Snippet::Ptr buildPreInstrumentation(FPSemantics *inst,
                BPatch_addressSpace *app, bool &needsRegisters);
        Snippet::Ptr buildPostInstrumentation(FPSemantics *inst,
                BPatch_addressSpace *app, bool &needsRegisters);
        Snippet::Ptr buildReplacementCode(FPSemantics *inst,
                BPatch_addressSpace *app, bool &needsRegisters);

        void registerInstruction(FPSemantics *inst);
        void handlePreInstruction(FPSemantics *inst);
        void handlePostInstruction(FPSemantics *inst);
        void handleReplacement(FPSemantics *inst);

        void checkRange(FPSemantics *inst, FPOperand *op);

        void finalOutput();

    private:

        FPAnalysisTRange();

        void expandInstData(size_t newSize);
        FPAnalysisTRangeInstData *instData;
        size_t instCount;

        void* rangeAddresses[256];
        size_t numRangeAddresses;
};

}

#endif

