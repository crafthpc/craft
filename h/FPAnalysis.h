#ifndef __FPANALYSIS_H
#define __FPANALYSIS_H

#include "BPatch.h"
#include "BPatch_addressSpace.h"
#include "BPatch_snippet.h"

#include "Point.h"
#include "Snippet.h"

#include "FPConfig.h"
#include "FPContext.h"
#include "FPDecoder.h"
#include "FPLog.h"
#include "FPSemantics.h"

using namespace Dyninst::PatchAPI;

namespace FPInst {

/**
 * Base class for a runtime analysis.
 */
class FPAnalysis {

    public:

        /**
         * INSTTIME & RUNTIME: Return a short (<10 chars) alphanum tag to
         * identify the analysis.
         */
        virtual string getTag();

        /**
         * INSTTIME & RUNTIME: Return a 3-6 word full title for the analysis.
         */
        virtual string getDescription();

        /**
         * INSTTIME & RUNTIME: Called once at initialization. The context will
         * be NULL if it is instrumentation time.
         */
        virtual void configure(FPConfig *config, FPDecoder *decoder,
                FPLog *log, FPContext *context);

        /**
         * INSTTIME: Various tests to see if the given instruction needs to be
         * instrumented or replaced. Pre- and post-checks should return false if
         * the replacement check returns true.
         */
        virtual bool shouldPreInstrument(FPSemantics *inst);
        virtual bool shouldPostInstrument(FPSemantics *inst);
        virtual bool shouldReplace(FPSemantics *inst);

        /**
         * INSTTIME: Generate streamlined Dyninst snippet for instrumentation.
         * Returning NULL will generate a call to the heavyweight handler.
         */
        virtual Snippet::Ptr buildPreInstrumentation(FPSemantics *inst,
                BPatch_addressSpace *app, bool &needsRegisters);

        virtual Snippet::Ptr buildPostInstrumentation(FPSemantics *inst,
                BPatch_addressSpace *app, bool &needsRegisters);

        virtual Snippet::Ptr buildReplacementCode(FPSemantics *inst,
                BPatch_addressSpace *app, bool &needsRegisters);

        /**
         * INSTTIME: Called once at end of instrumentation; should return a
         * short summary of instrumentation activity for the logfile.
         */
        virtual string finalInstReport();

        /**
         * RUNTIME: Called once at initialization for every instrumented instruction.
         */
        virtual void registerInstruction(FPSemantics *inst);

        /**
         * RUNTIME: Handle an instruction (heavyweight).
         */
        virtual void handlePreInstruction(FPSemantics *inst);
        virtual void handlePostInstruction(FPSemantics *inst);
        virtual void handleReplacement(FPSemantics *inst);

        /**
         * RUNTIME: Called once at finalization.
         */
        virtual void finalOutput();

        FPAnalysis();
        virtual ~FPAnalysis();

        FPConfig *configuration;
        FPDecoder *decoder;
        FPLog *logFile;
        FPContext *context;

};

}

#endif

