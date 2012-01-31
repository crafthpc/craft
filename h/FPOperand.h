#ifndef __FPOPERAND_H
#define __FPOPERAND_H

#include <stdint.h>
#include <math.h>

#include <iomanip>
#include <sstream>
#include <string>
#include <set>

#include "FPContext.h"

using namespace std;

namespace FPInst {

/**
 * An operand may be any of these types.
 * The SSE_Quad type represents an entire 128-bit SSE register.
 */
enum FPOperandType {
    IEEE_Single, IEEE_Double, C99_LongDouble, 
    SignedInt8, SignedInt16, SignedInt32, SignedInt64,
    UnsignedInt8, UnsignedInt16, UnsignedInt32, UnsignedInt64,
    SSE_Quad
};

/**
 * Stores the value of an operand.
 * Uses a union to store the actual data, so the structure will be at least as
 * large as the largest storable type (currently 16 bytes).
 */
typedef struct {
    FPOperandType type;
    union {
        void* ptr;
        float flt;
        double dbl;
        long double ldbl;
        int8_t sint8;
        int16_t sint16;
        int32_t sint32;
        int64_t sint64;
        uint8_t uint8;
        uint16_t uint16;
        uint32_t uint32;
        uint64_t uint64;
        uint32_t sse_quad[4];
    } data;
} FPOperandValue;

typedef void * FPOperandAddress;

/**
 * Represents a single operand in a machine-code instruction.
 * An operand may or may not have an associated value, depending on whether it
 * represents an input or output operand, and whether the current analysis cares
 * about actual values. Uses the FPContext class to refresh values from the 
 * system and to write them back. 
 */
class FPOperand {

    public:

        static FPOperandAddress FPRegTag2Addr(FPRegister reg, long tag);
        static string FPOpType2Str(FPOperandType val);
        static string FPOpVal2Str(FPOperandValue *val);
        static void* FPOpValPtr(FPOperandValue *val);
        static float FPOpValF(FPOperandValue *val);
        static double FPOpValD(FPOperandValue *val);
        static long double FPOpValLD(FPOperandValue *val);
        static int8_t FPOpValSInt8(FPOperandValue *val);
        static int16_t FPOpValSInt16(FPOperandValue *val);
        static int32_t FPOpValSInt32(FPOperandValue *val);
        static int64_t FPOpValSInt64(FPOperandValue *val);
        static uint8_t FPOpValUInt8(FPOperandValue *val);
        static uint16_t FPOpValUInt16(FPOperandValue *val);
        static uint32_t FPOpValUInt32(FPOperandValue *val);
        static uint64_t FPOpValUInt64(FPOperandValue *val);
        static long FPOpValExp(FPOperandValue *val);

        static void FPInvBytes(void *op, void *ret, long size);
        static void FPAndBytes(void *op1, void *op2, void *ret, long size);
        static void FPOrBytes (void *op1, void *op2, void *ret, long size);
        static void FPXorBytes(void *op1, void *op2, void *ret, long size);

        FPOperand();
        FPOperand(FPOperand *baseOp, long tag);

        FPOperand(FPOperandValue value);
        FPOperand(float value);
        FPOperand(double value);
        FPOperand(long double value);
        FPOperand(int8_t value);
        FPOperand(int16_t value);
        FPOperand(int32_t value);
        FPOperand(int64_t value);
        FPOperand(uint8_t value);
        FPOperand(uint16_t value);
        FPOperand(uint32_t value);
        FPOperand(uint64_t value);
        FPOperand(FPOperandType type);
        FPOperand(FPOperandType type, FPRegister reg);
        FPOperand(FPOperandType type, FPRegister reg, long tag);
        FPOperand(FPOperandType type, FPRegister base, FPRegister index, long disp, long scale);
        FPOperand(FPOperandType type, FPRegister base, FPRegister index, long disp, long scale, long tag);

        void refresh(FPContext *context);
        void refreshAddress(FPContext *context);
        void refreshValue(FPContext *context);

        void setType(FPOperandType type);

        FPOperandType getType();

        FPRegister getRegister();
        FPRegister getBase();
        FPRegister getIndex();
        long getDisp();
        long getScale();
        long getTag();

        FPOperandAddress getCurrentAddress();
        FPOperandValue getCurrentValue();
        long getCurrentValueExp();
        void* getCurrentValuePtr();
        float getCurrentValueF();
        double getCurrentValueD();
        long double getCurrentValueLD();
        int8_t getCurrentValueSInt8();
        int16_t getCurrentValueSInt16();
        int32_t getCurrentValueSInt32();
        int64_t getCurrentValueSInt64();
        uint8_t getCurrentValueUInt8();
        uint16_t getCurrentValueUInt16();
        uint32_t getCurrentValueUInt32();
        uint64_t getCurrentValueUInt64();

        void setCurrentValueZero(FPContext *context, bool writeBack = true, bool queue = false);
        void setCurrentValuePtr(void* val, FPContext *context, bool writeBack = true, bool queue = false);
        void setCurrentValueF(float val, FPContext *context, bool writeBack = true, bool queue = false);
        void setCurrentValueD(double val, FPContext *context, bool writeBack = true, bool queue = false);
        void setCurrentValueLD(long double val, FPContext *context, bool writeBack = true, bool queue = false);
        void setCurrentValueSInt8(int8_t val, FPContext *context, bool writeBack = true, bool queue = false);
        void setCurrentValueSInt16(int16_t val, FPContext *context, bool writeBack = true, bool queue = false);
        void setCurrentValueSInt32(int32_t val, FPContext *context, bool writeBack = true, bool queue = false);
        void setCurrentValueSInt64(int64_t val, FPContext *context, bool writeBack = true, bool queue = false);
        void setCurrentValueUInt8(uint8_t val, FPContext *context, bool writeBack = true, bool queue = false);
        void setCurrentValueUInt16(uint16_t val, FPContext *context, bool writeBack = true, bool queue = false);
        void setCurrentValueUInt32(uint32_t val, FPContext *context, bool writeBack = true, bool queue = false);
        void setCurrentValueUInt64(uint64_t val, FPContext *context, bool writeBack = true, bool queue = false);

        bool isInverted();
        void setInverted(bool invert);

        bool isMemory();
        bool isRegister();
        bool isImmediate();
        bool isStack();
        bool isLocalVar();
        bool isRegisterSSE();
        bool isRegisterST();
        bool isRegisterGPR();
        bool isGlobalVar();
        bool isGlobalVarInRange(FPOperandAddress lbound, FPOperandAddress ubound);
        long getDisplacement();

        void getNeededRegisters(set<FPRegister> &regs);
        void getModifiedRegisters(set<FPRegister> &regs);

        FPOperandType leastCommonPrecision(FPOperand* op);

        void add(FPOperand* op, FPOperandValue *dest);
        void sub(FPOperand* op, FPOperandValue *dest);
        void mul(FPOperand* op, FPOperandValue *dest);
        void div(FPOperand* op, FPOperandValue *dest);

        string toString();
        string toStringV();

        // public for speed
        FPOperandType type;
        FPOperandAddress currentAddress;
        FPOperandValue currentValue;

    private:

        FPRegister reg;
        long tag;
        FPRegister base, index;
        long disp, scale;
        bool immediate, inverted;

        void updateAttributes();
        bool attrIsMemory, attrIsRegister, attrIsImmediate, attrIsStack, attrIsLocalVar;
        bool attrIsRegisterSSE, attrIsRegisterST, attrIsRegisterGPR, attrIsGlobalVar;
};

}

#endif

