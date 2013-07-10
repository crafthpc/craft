#ifndef __FPANALYSISINPLACE_H
#define __FPANALYSISINPLACE_H

#include <malloc.h>
#include <math.h>

#include "fpflag.h"

#include "FPBinaryBlob.h"
#include "FPCodeGen.h"
#include "FPConfig.h"
#include "FPDecoderXED.h"
#include "FPAnalysis.h"
#include "FPAnalysisDCancel.h"

#include "FPSVPolicy.h"
#include "FPSVConfigPolicy.h"
#include "FPSVMemPolicy.h"


namespace FPInst {

class FPInplaceBlobInputEntry {

    public:

        FPInplaceBlobInputEntry(FPOperand *input, FPSemantics *inst, FPRegister reg, bool packed) {
            this->input = input;
            this->inst = inst;
            this->reg = reg;
            this->packed = packed;
        }

        FPInplaceBlobInputEntry(FPOperand *input,  FPSemantics *inst,FPRegister reg) {
            this->input = input;
            this->inst = inst;
            this->reg = reg;
            this->packed = false;
        }

        FPOperand *input;
        FPSemantics *inst;
        FPRegister reg;
        bool packed;
};

class FPBinaryBlobInplace : public FPBinaryBlob, public Snippet {

    public:

        static const uint64_t IEEE64_FLAG_MASK  = 0x7fffffff00000000;
        static const uint64_t IEEE64_VALUE_MASK = 0x00000000ffffffff;
        static const uint64_t IEEE64_FLAG       = 0x7ff4dead00000000;
        static const uint32_t IEEE32_FLAG       = 0x7ff4dead;

        FPBinaryBlobInplace(FPSemantics *inst, FPSVPolicy *mainPolicy);

        void enableLockPrefix();
        void disableLockPrefix();

        size_t buildSpecialOp(unsigned char *pos,
                FPOperation *op, bool packed, bool &replaced);
        void addBlobInputEntry(vector<FPInplaceBlobInputEntry> &inputs,
                FPOperand *input, bool packed, FPRegister &replacementRM);
        size_t buildReplacedOperation(unsigned char *pos,
                        FPOperation *op, FPSVType replacementType,
                        unsigned char *orig_code, size_t origNumBytes,
                        vector<FPInplaceBlobInputEntry> &inputs, FPRegister replacementRM, 
                        bool packed, bool only_movement, bool mem_output, bool xmm_output,
                        bool &special, bool &replaced);

        bool generate(Point *pt, Buffer &buf);

        size_t buildInitBlobSingle(unsigned char *pos, FPRegister dest, long tag);
        size_t buildInitBlobDouble(unsigned char *pos, FPRegister dest, long tag);
        size_t buildFlagTestBlob(unsigned char *pos, FPRegister dest, long tag);
        size_t buildOperandInitBlob(unsigned char *pos,
                FPInplaceBlobInputEntry entry, FPSVType replacementType);
        size_t buildReplacedInstruction(unsigned char *pos,
                FPSemantics *inst, unsigned char *orig_bytes,
                FPSVType replacementType, FPRegister replacementRM, bool changePrecision);
        size_t buildFixSSEQuadOutput(unsigned char *pos, FPRegister xmm);

        // AND/OR/mask versions
        size_t buildInsertGPR32IntoXMM(unsigned char *pos,
                FPRegister gpr, FPRegister xmm, long tag, FPRegister tmp);
        size_t buildExtractGPR64FromXMM(unsigned char *pos,
                FPRegister gpr, FPRegister xmm, long tag);
        size_t buildInsertGPR64IntoXMM(unsigned char *pos,
                FPRegister gpr, FPRegister xmm, long tag);

        size_t buildSpecialCvtss2sd(unsigned char *pos, unsigned char orig_prefix, unsigned char orig_modrm);
        size_t buildSpecialCvtsd2ss(unsigned char *pos, unsigned char orig_prefix, unsigned char orig_modrm);
        size_t buildSpecialNegate(unsigned char *pos, FPOperand *src, FPOperand *dest);
        size_t buildSpecialZero(unsigned char *pos, FPOperand *dest);

    private:

        FPSVPolicy *mainPolicy;
        FPRegister temp_gpr1, temp_gpr2;    // for initializing/testing operands
        FPRegister temp_gpr3;               // for PINSR/PEXTR equivalents

        bool useLockPrefix;               // add LOCK prefix to INC instructions

        string debug_assembly;
        unsigned char debug_code[256];
        size_t debug_size;

};

/**
 * Performs shadow-value analysis via in-place replacement. Only supports
 * down-casting (ex. replacement of doubles by singles).
 */
class FPAnalysisInplace : public FPAnalysis
{

    public:

        static bool debugPrint;
        static void enableDebugPrint();
        static void disableDebugPrint();
        static bool isDebugPrintEnabled();
        static void debugPrintCompare(double input1, double input2, bool ordered, unsigned long flags);

        static bool existsInstance();
        static FPAnalysisInplace* getInstance();

        string getTag();
        string getDescription();

        void configure(FPConfig *config, FPDecoder *decoder, 
                FPLog *log, FPContext *context);

        void enableCancellationDetection(int minPriority=10);
        void disableCancellationDetection();
        void setShadowEntries(vector<FPShadowEntry*> &entries);
        void getShadowEntries(vector<FPShadowEntry*> &entries);
        void enableReportAllGlobals();
        void disableReportAllGlobals();

        bool shouldPreInstrument(FPSemantics *inst);
        bool shouldPostInstrument(FPSemantics *inst);
        bool shouldReplace(FPSemantics *inst);
        FPSVType getSVType(FPSemantics *inst);
        FPReplaceEntryTag getDefaultRETag(FPSemantics *inst);

        string finalInstReport();

        void registerInstruction(FPSemantics *inst);
        void handlePreInstruction(FPSemantics *inst);
        void handlePostInstruction(FPSemantics *inst);
        void handleReplacement(FPSemantics *inst);

        bool isSupportedOp(FPOperation *op);

        void enableLockPrefix();
        void disableLockPrefix();

        bool buildBinaryBlob(Dyninst::Address addr,
                const BPatch_Vector<Dyninst::MachRegister> &input_regs,
                Dyninst::MachRegister &output_reg, void *handle,
                BPatch_Vector<Dyninst::MachRegister> &used_regs,
                void* &buffer, unsigned long &buffer_size);

        bool canBuildBinaryBlob(FPSemantics *inst);

        Snippet::Ptr buildPreInstrumentation(FPSemantics *inst,
                BPatch_addressSpace *app, bool &needsRegisters);
        Snippet::Ptr buildPostInstrumentation(FPSemantics *inst,
                BPatch_addressSpace *app, bool &needsRegisters);
        Snippet::Ptr buildReplacementCode(FPSemantics *inst,
                BPatch_addressSpace *app, bool &needsRegisters);

        void expandInstCount(size_t newSize);

        void handleConvert(FPOperand *output, FPOperand *input);
        void handleZero(FPOperand *output);
        void handleUnaryOp(FPOperationType type, FPOperand *output, FPOperand *input);
        void handleBinaryOp(FPOperationType type, FPOperand *output, FPOperand *input1, FPOperand *input2);

        void handleComI(float input1, float input2, bool ordered);
        void handleComI(double input1, double input2, bool ordered);
        //unsigned long handleCmp(float input1, float input2);
        //unsigned long handleCmp(double input1, double input2);

        /*
        void handleAm(FPSV *input);
        void handleCom(FPSV *input1, FPSV *input2, bool ordered);
        */

        //float handleUnaryReplacedFunc(FPOperationType type, float input);
        double handleUnaryReplacedFunc(FPOperationType type, double input);
        //long double handleUnaryReplacedFunc(FPOperationType type, long double input);
        //float handleBinaryReplacedFunc(FPOperationType type, float input1, float input2);
        double handleBinaryReplacedFunc(FPOperationType type, double input1, double input2);
        //long double handleBinaryReplacedFunc(FPOperationType type, long double input1, long double input2);

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

        double replaceDouble(float value);
        double replaceDouble(double value);

        bool isReplaced(double value);
        bool isReplaced(void *value);

        float extractFloat(double input);
        float extractFloat(long double input);
        float extractFloat(FPOperand *op);
        double extractDouble(FPOperand *op);

        void saveFloatToContext(FPOperand *output, float value);
        void saveDoubleToContext(FPOperand *output, double value);
        void saveReplacedDoubleToContext(FPOperand *output, float value);

        void finalOutput();

    private:

        FPAnalysisInplace();

        FPSVPolicy *mainPolicy;

        FPSemantics *currInst;
        FPOperandValue *currFlag;

        bool detectCancellations;
        FPAnalysisDCancel *cancelAnalysis;

        bool reportAllGlobals;
        vector<FPShadowEntry*> shadowEntries;

        bool useLockPrefix;               // add LOCK prefix to INC instructions

        size_t *instCountSingle;
        size_t *instCountDouble;
        size_t instCountSize;

        size_t insnsInstrumentedSingle;
        size_t insnsInstrumentedDouble;

        // output helpers
        void printShadowLocation(FPOperandAddress addr);
        void printShadowMatrix(string baseName, FPOperandAddress baseAddr, 
                unsigned long isize, unsigned long rows, unsigned long cols);
        void printShadowEntry(string name, FPOperandAddress addr, unsigned long size);
        vector<FPOperandAddress> doneAlready;
};

}

#endif

