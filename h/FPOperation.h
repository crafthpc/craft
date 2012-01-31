#ifndef __FPOPERATION_H
#define __FPOPERATION_H

#include "FPContext.h"
#include "FPOperand.h"

#include <map>
#include <string>
#include <vector>

using namespace std;

namespace FPInst {

/**
 * An operation must be one of these types.
 * Most operation types correspond to x86/SSE instruction types, but there are
 * also some mathematical operations that are used to encode the semantics of
 * libm functions.
 */
enum FPOperationType {
    OP_INVALID,
    OP_NONE, OP_MOV, OP_CVT,
    OP_COM, OP_COMI, OP_UCOM, OP_UCOMI,
    OP_CMP,
    OP_PUSH_X87, OP_POP_X87,
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, 
    OP_NOT, OP_AND, OP_OR, OP_XOR,
    OP_MIN, OP_MAX,
    OP_ZERO,
    OP_SQRT, OP_CBRT,
    OP_NEG, OP_ABS, OP_AM,
    OP_SIN,   OP_COS,   OP_TAN,
    OP_ASIN,  OP_ACOS,  OP_ATAN,
    OP_SINH,  OP_COSH,  OP_TANH,
    OP_ASINH, OP_ACOSH, OP_ATANH,
    OP_LOG, OP_LOGB, OP_LOG10,
    OP_EXP, OP_EXP2,
    OP_CEIL, OP_FLOOR, OP_TRUNC, OP_ROUND,
    OP_ERF, OP_ERFC,
    OP_ATAN2, OP_COPYSIGN, OP_FMOD, OP_POW,
    OP_CLASS, OP_ISFIN, OP_ISNORM, OP_ISNAN, OP_ISINF
};

/**
 * A combination of input and output operands.
 * Operand sets provide a speedy and compact way of providing an analysis with
 * corresponding input/output operand groups. The FPOperation class  handles all the
 * bookkeeping and management of these operand sets.
 */
typedef struct {
    size_t nIn;
    FPOperand *in[4];
    size_t nOut;
    FPOperand *out[2];
} FPOperandSet;


/**
 * Represents a mathematical operation involving at least one operand.
 * May include multiple inputs and/or outputs, and uses FPOperandSet objects to
 * track which operands correspond to each other. Operands must be added in
 * order; i.e. inputs must be immediately followed by their corresponding
 * outputs. Adding an input operand after an output operand serves to indicate
 * that a new operand set should be created.
 */
class FPOperation {

    public:
        
        static string FPOpType2Str(FPOperationType type);

        FPOperation();
        FPOperation(FPOperationType type);
        ~FPOperation();

        void setType(FPOperationType type);
        FPOperationType getType();

        /**
         * Adds an input FPOperand.
         * If this is the first operand, or if there are already output 
         * operands in the current operand set, this routine starts a new
         * operand set. Otherwise, it adds the operand to the current operand
         * set.
         */
        void addInputOperand(FPOperand *op);

        /**
         * Adds a new output FPOperand.
         * If this is the first operand, this routine starts a new operand set.
         * Otherwise, it adds the operand to the current operand set.
         */
        void addOutputOperand(FPOperand *op);

        void reverseInputOperands();
        void invertFirstInputOperands();

        /**
         * Refresh the addresses and values for input operands, but only the
         * addresses for output operands.
         */
        void refreshAllOperands(FPContext *context);

        /**
         * Refresh the addresses and values for input operands; ignores any
         * output operands.
         */
        void refreshInputOperands(FPContext *context);

        size_t getNumInputOperands();   ///< Returns number of input operands.
        size_t getNumOutputOperands();  ///< Returns number of output operands.

        /**
         * Returns a pointer to the operand set array, and a number representing
         * the number of sets. These are NOT currently copied because of
         * efficiency concerns. Users of the FPOperation class should not
         * directly modify the FPOperandSet objects.
         */
        void getOperandSets(FPOperandSet* &ops, size_t &numOps);

        /**
         * Returns true if any operand set has an operand of the given type.
         */
        bool hasOperandOfType(FPOperandType type);

        /**
         * Fills the given set with all the registers that would be needed to
         * refresh the addresses and values of input registers, as well as to
         * refresh the addresses (but not values) of output registers.
         */
        void getNeededRegisters(set<FPRegister> &regs);

        /**
         * Fills the given set with all the registers that may need to be
         * changed by the operation.
         */
        void getModifiedRegisters(set<FPRegister> &regs);

        FPOperationType type;       ///< operation type; made public for speed
        FPOperandSet opSets[4];     ///< array of operand sets; made public for speed
        size_t numOpSets;           ///< number of operand sets; made public for speed

        string toString();
        string toStringV();

    private:
        
        FPOperand* inputs[8];
        FPOperand* outputs[8];
        size_t numInputs, numOutputs;
};

}

#endif

