#ifndef __FPFPSV_H
#define __FPFPSV_H

#include <gc.h>
#include <gc_cpp.h>

#include "FPContext.h"
#include "FPOperand.h"
#include "FPOperation.h"
#include "FPSemantics.h"

namespace FPInst {

/**
 * Every shadow value must be one of these types.
 * An SVT_NONE shadow value is of little use in practice and will cause an
 * error in most situations; it is used only as a placeholder when the type
 * is irrelevant.
 */
enum FPSVType {
    SVT_NONE,
    SVT_IEEE_Single,
    SVT_IEEE_Double,
    SVT_MPFR_Quad,
    SVT_Composite
};


/**
 * Represents a shadow value stored on the heap. The FPAnalysisPointer class
 * generally will not directly instanciate an FPSV object, but will rather use
 * its various subclasses. Most functions in the base FPSV are unimplemented.
 *
 * NOTE: Individual FPSV subclass should NOT be aware of the existence of the
 * other subclasses; any cross-type operations should be handled by FPAnalysisPointer.
 */
#ifdef USE_GARBAGE_COLLECTOR
class FPSV : public gc {
#else
class FPSV {
#endif

    public:

    /** INITIALIZATION **/

        FPSV(FPSVType type, FPOperandAddress addr);
        virtual ~FPSV();

        // set the seed data to that of the given base
        void setSeedData(FPSV *base);

        // set the seed data to the current value
        // (must be derived class!)
        virtual void setSeedData() = 0;

        // set the shadow value to the current value of the given operand,
        // coercing the value into the type of the shadow value
        virtual void setValue(FPOperand *op) = 0;
        virtual void setValue(FPOperandValue val) = 0;

        // set the shadow value to certain significant constant values
        // (to the best of a shadow value's ability to represent these values)
        virtual void setToZero() = 0;
        virtual void setToPositiveBitFlag() = 0;
        virtual void setToNegativeBitFlag() = 0;
        virtual void setToCMPTrue() = 0;
        virtual void setToCMPFalse() = 0;
        

    /** OPERATIONS **/

        // compare to another shadow value (MUST BE OF IDENTICAL TYPE!)
        // returns -1 if less than, 0 if equal to, and 1 if greater than
        // the given value
        virtual int compare(FPSV *val2) = 0;

        // perform various operations, storing the result in this shadow value
        // assumes the operands are of the same type
        virtual void doUnaryOp(FPOperationType type, FPSV *op) = 0;
        virtual void doBinaryOp(FPOperationType type, FPSV *op1, FPSV* op2) = 0;

        // misc operations that don't fit the normal template
        virtual void doModF(FPSV *op, FPSV *int_part) = 0;
        virtual void doFrExp(FPSV *op, int *exp) = 0;
        virtual void doLdExp(FPSV *op, int exp) = 0;


    /** OUTPUT **/

        // check for various conditions
        virtual bool isBitwiseZero() = 0;
        virtual bool isZero() = 0;
        virtual bool isNegative() = 0;
        virtual bool isPositive() = 0;
        virtual bool isNormal() = 0;
        virtual bool isNaN() = 0;
        virtual bool isInf() = 0;

        // get the current shadow value in the operand type most appropriate 
        // to this shadow value type
        virtual FPOperandValue getValue() = 0;

        // get the current shadow value coerced into the given operand type
        virtual FPOperandValue getValue(FPOperandType type) = 0;

        virtual string toString();
        virtual string toDetailedString();
        virtual string getTypeString();


    /** MEMBER VARIABLES **/

        // member variables are public for speed reasons
        
        FPSVType type;
        FPOperandAddress addr;

        // seed data represents the bits of the original value used to set this
        // shadow value; it is used in some bitwise operations and is discarded
        // after a binary operation or a unary operation that changes the value
        size_t   seed_size;     // size is number of 32-bit dwords, and must be <= 4 
        uint32_t seed_data[4];

};

}

#endif

