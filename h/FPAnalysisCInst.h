#ifndef __FPANALYSISCINST_H
#define __FPANALYSISCINST_H

#include "FPAnalysis.h"

namespace FPInst {

/**
 * Instruction count analysis.
 * This very simple analysis creates a counter for every instruction and
 * increments that counter every time the instruction is executed. The
 * initialization code must call addSemantics with every instruction to be
 * monitored. This is a singleton analysis; you can't have multiple sets of
 * counters going at once.
 */
class FPAnalysisCInst : public FPAnalysis {

    public:

        static bool existsInstance();
        static FPAnalysisCInst* getInstance();

        string getTag();
        string getDescription();

        void configure(FPConfig *config, FPDecoder *decoder,
                FPLog *log, FPContext *context);

        bool shouldPreInstrument(FPSemantics *inst);
        bool shouldPostInstrument(FPSemantics *inst);
        bool shouldReplace(FPSemantics *inst);

        Snippet::Ptr buildPreInstrumentation(FPSemantics *inst,
                BPatch_addressSpace *app, bool &needsRegisters);
        Snippet::Ptr buildPostInstrumentation(FPSemantics *inst,
                BPatch_addressSpace *app, bool &needsRegisters);
        Snippet::Ptr buildReplacementCode(FPSemantics *inst,
                BPatch_addressSpace *app, bool &needsRegisters);

        void expandInstCount(size_t newSize);
        size_t *getCountArrayPtr();

        string finalInstReport();

        void registerInstruction(FPSemantics *inst);
        void handlePreInstruction(FPSemantics *inst);
        void handlePostInstruction(FPSemantics *inst);
        void handleReplacement(FPSemantics *inst);

        void finalOutput();

    private:

        FPAnalysisCInst();

        size_t *instCount;
        size_t instCountSize;

        size_t insnsInstrumented;

};

}

#endif

