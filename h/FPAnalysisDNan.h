#ifndef __FPANALYSISDNAN_H
#define __FPANALYSISDNAN_H

#include "FPConfig.h"
#include "FPAnalysis.h"

namespace FPInst {

/**
 * Performs NaN detection analysis.
 */
class FPAnalysisDNan : public FPAnalysis {

    public:

        static bool existsInstance();
        static FPAnalysisDNan* getInstance();

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

        string finalInstReport();

        void registerInstruction(FPSemantics *inst);
        void handlePreInstruction(FPSemantics *inst);
        void handlePostInstruction(FPSemantics *inst);
        void handleReplacement(FPSemantics *inst);

        void finalOutput();

    private:

        FPAnalysisDNan();

        void* rangeAddresses[256];
        size_t numRangeAddresses;

        size_t insnsInstrumented;
};

}

#endif

