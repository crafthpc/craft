#ifndef __FPANALYSISPOINTER_H
#define __FPANALYSISPOINTER_H

#include <malloc.h>
#include <math.h>

// for garbage collection
//#define USE_GARBAGE_COLLECTOR
//#include "gc.h"
//#include "gc_cpp.h"

// for manual live pointer tracking
//#define USE_LIVE_PTR_LIST
//#ifndef FPLIVEPTRDEF
//#define FPLIVEPTRDEF
//struct FPLivePtr {
    //void* ptr;
    //struct FPLivePtr *next;
//};
//typedef struct FPLivePtr* FPLivePtrList;
//#endif

#include "fpflag.h"

#include "FPConfig.h"
#include "FPAnalysis.h"
#include "FPAnalysisDCancel.h"
#include "FPSV.h"
#include "FPSVPolicy.h"
#include "FPSVSingle.h"
#include "FPSVDouble.h"


namespace FPInst {

void _INST_print_flags2(const char *tag, unsigned long flags);
void _INST_segfault_handler (int /*sig*/);

/**
 * Performs shadow-value analysis via pointer replacement.
 * Replaces floating-point values with pointers to shadow values allocated on
 * the heap. Handles all operations with special care to use the pointer values
 * if they are present. Uses a garbage collector to track which values have been
 * replaced by pointers, and to clean up shadow values are no longer in use.
 * Defers all decisions about pointer replacement and shadow value type to the
 * given FPSVPolicy object. This is a singleton analysis; you can't replace a
 * value by multiple pointers. If you want to replace a value with multiple
 * types of shadow values, use a custom policy and aggregate shadow values.
 */
class FPAnalysisPointer : public FPAnalysis
{

    public:

#ifdef USE_LIVE_PTR_LIST
        static const unsigned long LIVE_PTR_TBL_SIZE = 2621431;   // 10 MB
#endif

        static bool debugPrint;
        static void enableDebugPrint();
        static void disableDebugPrint();
        static bool isDebugPrintEnabled();

        static bool existsInstance();
        static FPAnalysisPointer* getInstance();

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

        void enableCancellationDetection(int minPriority=10);
        void disableCancellationDetection();
        void setShadowEntries(vector<FPShadowEntry*> &entries);
        void getShadowEntries(vector<FPShadowEntry*> &entries);
        void enableReportAllGlobals();
        void disableReportAllGlobals();

        string finalInstReport();

        void registerInstruction(FPSemantics *inst);
        void handlePreInstruction(FPSemantics *inst);
        void handlePostInstruction(FPSemantics *inst);
        void handleReplacement(FPSemantics *inst);

        void handleConvert(FPOperand *output, FPOperand *input);
        void handleZero(FPOperand *output);
        void handleUnaryOp(FPOperationType type, FPOperand *output, FPOperand *input);
        void handleBinaryOp(FPOperationType type, FPOperand *output, FPOperand *input1, FPOperand *input2);

        void handleAndOp(FPSV *output, FPSV *input1, FPSV *input2);
        void handleOrOp(FPSV *output, FPSV *input1, FPSV *input2);
        void handleXorOp(FPSV *output, FPSV *input1, FPSV *input2);
        void handleAm(FPSV *input);
        void handleCom(FPSV *input1, FPSV *input2, bool ordered);
        void handleComI(FPSV *input1, FPSV *input2, bool ordered);
        void handleCmp(FPSV *output, FPSV *input1, FPSV *input2, bool ordered);

        float handleUnaryReplacedFunc(FPOperationType type, float input);
        double handleUnaryReplacedFunc(FPOperationType type, double input);
        long double handleUnaryReplacedFunc(FPOperationType type, long double input);
        float handleBinaryReplacedFunc(FPOperationType type, float input1, float input2);
        double handleBinaryReplacedFunc(FPOperationType type, double input1, double input2);
        long double handleBinaryReplacedFunc(FPOperationType type, long double input1, long double input2);

        void handleReplacedFuncSinCos(float input, float *sin_output, float *cos_output);
        void handleReplacedFuncSinCos(double input, double *sin_output, double *cos_output);
        void handleReplacedFuncSinCos(long double input, long double *sin_output, long double *cos_output);

        float handleReplacedFuncModF(float input, float *int_output);
        double handleReplacedFuncModF(double input, double *int_output);
        long double handleReplacedFuncModF(long double input, long double *int_output);

        float handleReplacedFuncFrExp(float input, int *exp_output);
        double handleReplacedFuncFrExp(double input, int *exp_output);
        long double handleReplacedFuncFrExp(long double input, int *exp_output);

        float handleReplacedFuncLdExp(float sig_input, int exp_input);
        double handleReplacedFuncLdExp(double sig_input, int exp_input);
        long double handleReplacedFuncLdExp(long double sig_input, int exp_input);

        bool isSVPtr(void *ptr);
        FPSV* getOrCreateSV(float value);
        FPSV* getOrCreateSV(double value);
        FPSV* getOrCreateSV(long double value);
        FPSV* getOrCreateSV(FPOperand *op);
        FPSV* createSV(FPSVType type, FPOperandAddress addr);
        void convertSV(FPSV *dest, FPSV *src);
        void saveSVPtrToContext(FPOperand *output, FPSV *value, FPContext *context);
        void saveSVValueToContext(FPOperand *output, FPSV *value, FPContext *context);

        long getNumAllocations();
        long getNumGCAllocations();
        long getNumValueWriteBacks();
        long getNumPtrWriteBacks();
#ifdef USE_LIVE_PTR_LIST
        // for debug
        void printLivePtrTableHealth();
#endif
        void finalOutput();

    private:

        FPAnalysisPointer();

        FPSVPolicy *mainPolicy;
        FPSemantics *currInst;
        FPOperandValue *currFlag;
        unsigned long num_allocations;
        unsigned long num_gcAllocations;
        unsigned long num_valWriteBacks;
        unsigned long num_ptrWriteBacks;

        bool detectCancellations;
        FPAnalysisDCancel *cancelAnalysis;

        bool reportAllGlobals;
        vector<FPShadowEntry*> shadowEntries;

        size_t insnsInstrumented;

        // live pointer table
#ifdef USE_LIVE_PTR_LIST
        FPLivePtrList livePointers[LIVE_PTR_TBL_SIZE];
#endif

        // output helpers
        void printShadowLocation(FPOperandAddress addr);
        void printShadowMatrix(string baseName, FPOperandAddress baseAddr, 
                unsigned long isize, unsigned long rows, unsigned long cols);
        void printShadowEntry(string name, FPOperandAddress addr, unsigned long size);
        vector<FPOperandAddress> doneAlready;
};

}

#endif

