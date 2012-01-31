#ifndef __FPFPSVSINGLE_H
#define __FPFPSVSINGLE_H

#include "math.h"

#include "FPAnalysisPointer.h"
#include "FPSV.h"

namespace FPInst {

/**
 * Stores an IEEE single-precision (32-bit) shadow value.
 */
class FPSVSingle : public FPSV {

    public:

        FPSVSingle(FPOperandAddress addr);
        ~FPSVSingle();

        void setSeedData();

        void setValue(FPOperand *op);
        void setValue(FPOperandValue val);
        void setToZero();
        void setToPositiveBitFlag();
        void setToNegativeBitFlag();
        void setToCMPTrue();
        void setToCMPFalse();

        int compare(FPSV *val2);
        bool isBitwiseZero();
        bool isZero();
        bool isNegative();
        bool isPositive();
        bool isNormal();
        bool isNaN();
        bool isInf();

        void doUnaryOp(FPOperationType type, FPSV *op);
        void doBinaryOp(FPOperationType type, FPSV *op1, FPSV* op2);
        void doModF(FPSV *op, FPSV *int_part);
        void doFrExp(FPSV *op, int *exp);
        void doLdExp(FPSV *op, int exp);

        FPOperandValue getValue();
        FPOperandValue getValue(FPOperandType type);

        string toString();
        string toDetailedString();
        string getTypeString();

    public:

        float value;

};

}

#endif

