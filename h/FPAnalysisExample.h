// NEWMODE: change this label
//
#ifndef __FPANALYSISEXAMPLE_H
#define __FPANALYSISEXAMPLE_H

#include "FPConfig.h"
#include "FPAnalysis.h"

namespace FPInst {

/**
 * Example analysis. Prints the assembly for each floating-point instruction's
 * when it is executed.
 *
 * This analysis is not intended to be useful on its own; rather, it is intended
 * to serve as an example for those who wish to write their own analysis mode.
 *
 * Places where the code will need to be modified are tagged with "NEWMODE".
 *
 * You will also need to modify /Makefile and /src/fpinst.cpp to add the new
 * analysis.
 */
class FPAnalysisExample : public FPAnalysis {

    public:

        static bool existsInstance();
        static FPAnalysisExample* getInstance();

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

        string finalInstReport();

        void registerInstruction(FPSemantics *inst);
        void handlePreInstruction(FPSemantics *inst);
        void handlePostInstruction(FPSemantics *inst);
        void handleReplacement(FPSemantics *inst);

        void finalOutput();

    private:

        FPAnalysisExample();
       
        string msgTag;

        size_t insnsInstrumented;
        size_t insnsExecuted;
};

}

#endif

