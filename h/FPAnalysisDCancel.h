#ifndef __FPANALYSISDCANCEL_H
#define __FPANALYSISDCANCEL_H

#include "FPConfig.h"
#include "FPAnalysis.h"

namespace FPInst {

/**
 * Stores cancellation data for a single instruction.
 */
struct FPAnalysisDCancelInstData {
    FPSemantics *inst;
    unsigned long count;
    unsigned long total_cancels;
    unsigned long total_digits;
};

/**
 * Performs cancellation detection analysis.  Examines the operands for every
 * addition or subtraction instruction to determine whether digits have been
 * canceled. If the number of digits cancelled exceeds the given minPriority
 * threshold, the incident is logged.  If sampling is enabled, only a selection
 * of cancellations are stored with detailed messages, although the count and
 * digit number information is stored for every cancellation regardless of
 * sampling. This is a singleton analysis; you can't have multiple detectors
 * going at once.
 */
class FPAnalysisDCancel : public FPAnalysis {

    public:

        static bool existsInstance();
        static FPAnalysisDCancel* getInstance();

        string getTag();
        string getDescription();

        void configure(FPConfig *config, FPDecoder *decoder,
                FPLog *log, FPContext *context);

        void setMinPriority(long minPriority);
        long getMinPriority();

        bool isSamplingEnabled();
        void enableSampling();
        void disableSampling();
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

        void checkForCancellation(FPSemantics *inst, FPOperationType opt, FPOperand *op1, FPOperand *op2);

        void finalOutput();

    private:

        FPAnalysisDCancel();
        
        long min_priority;
        bool enable_sampling;

        unsigned incrementCount(FPSemantics *inst);
        unsigned addCancellation(FPSemantics *inst, long digits);
        void expandInstData(size_t newSize);
        FPAnalysisDCancelInstData *instData;
        size_t instCount;

        void* cancelAddresses[256];
        size_t numCancelAddresses;

        size_t insnsInstrumented;
};

}

#endif

