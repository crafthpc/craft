#include "FPOperation.h"

namespace FPInst {

string FPOperation::FPOpType2Str(FPOperationType type)
{
    switch (type) {
        case OP_NONE: return " none";
        case OP_MOV:  return " mov"; case OP_CVT:  return " cvt";
        case OP_COM:  return  " com"; case OP_COMI:  return  " comi";
        case OP_UCOM: return " ucom"; case OP_UCOMI: return " ucomi";
        case OP_CMP:  return  " cmp";
        case OP_PUSH_X87: return " push"; case OP_POP_X87:  return " pop";
        case OP_ADD:  return " add"; case OP_SUB:  return " sub";
        case OP_MUL:  return " mul"; case OP_DIV:  return " div";
        case OP_AND:  return " and"; case OP_OR:   return " or";
        case OP_XOR:  return " xor"; case OP_NOT:  return " not";
        case OP_MIN:  return " min"; case OP_MAX:  return " max";
        case OP_ZERO: return " zero";
        case OP_SQRT: return " sqrt"; case OP_CBRT: return " cbrt";
        case OP_NEG:  return " neg"; case OP_ABS:  return " abs";
        case OP_AM:   return " am";
        case OP_SIN:   return   " sin";  case OP_COS:   return   " cos";  case OP_TAN:   return   " tan";
        case OP_ASIN:  return  " asin";  case OP_ACOS:  return  " acos";  case OP_ATAN:  return  " atan";
        case OP_SINH:  return  " sinh";  case OP_COSH:  return  " cosh";  case OP_TANH:  return  " tanh";
        case OP_ASINH: return " asinh";  case OP_ACOSH: return " acosh";  case OP_ATANH: return " atanh";
        case OP_LOG:  return " log"; case OP_LOGB:  return " logb"; case OP_LOG10:  return " log10";
        case OP_EXP:  return " exp"; case OP_EXP2:  return " exp2";
        case OP_CEIL: return " ceil"; case OP_FLOOR: return " floor";
        case OP_TRUNC: return " trunc"; case OP_ROUND: return " round";
        case OP_ERF: return "erf"; case OP_ERFC: return " erfc";
        case OP_ATAN2: return " atan2"; case OP_COPYSIGN: return " copysign";
        case OP_FMOD: return " fmod"; case OP_POW: return " pow";
        case OP_CLASS: return " classify"; case OP_ISFIN: return " isfinite";
        case OP_ISNORM: return " isnormal"; case OP_ISNAN: return " isnan";
        case OP_ISINF: return " isinf";
        default:      return " invalid";
    }
}

FPOperation::FPOperation()
{
    type = OP_NONE;
    numInputs = 0;
    numOutputs = 0;
    numOpSets = 0;
}

FPOperation::FPOperation(FPOperationType type)
{
    this->type = type;
    numInputs = 0;
    numOutputs = 0;
    numOpSets = 0;
}

FPOperation::~FPOperation()
{
    unsigned i;
    for (i=0; i<numInputs; i++) {
        delete inputs[i];
    }
    for (i=0; i<numOutputs; i++) {
        delete outputs[i];
    }
}

void FPOperation::setType(FPOperationType type)
{
    this->type = type;
}

FPOperationType FPOperation::getType()
{
    return type;
}

void FPOperation::addInputOperand(FPOperand *op)
{
    // start new opset if there are already output operands
    if (numOpSets == 0 || opSets[numOpSets-1].nOut > 0) {
        numOpSets++;
        opSets[numOpSets-1].nIn = 0;
        opSets[numOpSets-1].nOut = 0;
    }
    FPOperandSet *set = &opSets[numOpSets-1];
    set->in[set->nIn++] = op;
    inputs[numInputs++] = op;
}

void FPOperation::addOutputOperand(FPOperand *op)
{
    if (numOpSets == 0) {
        numOpSets++;
        opSets[numOpSets-1].nIn = 0;
        opSets[numOpSets-1].nOut = 0;
    }
    FPOperandSet *set = &opSets[numOpSets-1];
    set->out[set->nOut++] = op;
    outputs[numOutputs++] = op;
}

void FPOperation::reverseInputOperands()
{
    FPOperand *temp;
    unsigned i;
    if (numInputs <= 0) return;
    // swap input operands (two-by-two)
    for (i=0; i<numInputs-1; i+=2) {
        temp = inputs[i];
        inputs[i] = inputs[i+1];
        inputs[i+1] = temp;
    }
}

void FPOperation::invertFirstInputOperands()
{
    unsigned i;
    if (numInputs <= 0) return;
    // invert first input operands
    for (i=0; i<numInputs/2; i++) {
        inputs[i]->setInverted(true);
    }
}

void FPOperation::refreshAllOperands(FPContext *context)
{
    unsigned i;

    // refresh addresses and values for input operands
    for (i=0; i<numInputs; i++) {
        inputs[i]->refresh(context);
    }

    // we need to refresh the addresses for output operands, but not the values
    for (i=0; i<numOutputs; i++) {
        outputs[i]->refreshAddress(context);
    }
}

void FPOperation::refreshInputOperands(FPContext *context)
{
    unsigned i;

    // refresh addresses and values for input operands
    for (i=0; i<numInputs; i++) {
        inputs[i]->refresh(context);
    }
}

size_t FPOperation::getNumInputOperands()
{
    return numInputs;
}

size_t FPOperation::getNumOutputOperands()
{
    return numOutputs;
}

void FPOperation::getOperandSets(FPOperandSet* &ops, size_t &numOps)
{
    ops = opSets;
    numOps = numOpSets;
}

bool FPOperation::hasOperandOfType(FPOperandType type)
{
    bool hasOp = false;
    size_t i;
    for (i=0; !hasOp && i<numInputs; i++) {
        if (inputs[i]->getType() == type) {
            hasOp = true;
        }
    }
    for (i=0; !hasOp && i<numOutputs; i++) {
        if (outputs[i]->getType() == type) {
            hasOp = true;
        }
    }
    return hasOp;
}

void FPOperation::getNeededRegisters(set<FPRegister> &regs)
{
    size_t i;
    for (i=0; i<numInputs; i++) {
        inputs[i]->getNeededRegisters(regs);
    }
    for (i=0; i<numOutputs; i++) {
        outputs[i]->getNeededRegisters(regs);
    }
}

void FPOperation::getModifiedRegisters(set<FPRegister> &regs)
{
    size_t i;
    for (i=0; i<numOutputs; i++) {
        outputs[i]->getModifiedRegisters(regs);
    }
}

string FPOperation::toString()
{
    string ts;
    unsigned i, j;
    ts = FPOpType2Str(type);
    for (i=0; i<numOpSets; i++) {
        FPOperandSet set = opSets[i];
        ts += "\n   opset:";
        for (j=0; j<set.nIn; j++) {
            ts += "\n     in: " + set.in[j]->toString();
        }
        for (j=0; j<set.nOut; j++) {
            ts += "\n     out: " + set.out[j]->toString();
        }
    }
    /*
     *for (i=0; i<numInputs; i++) {
     *   ts += "\n   in: " + inputs[i]->toString();
     *}
     *for (i=0; i<numOutputs; i++) {
     *   ts += "\n   out: " + outputs[i]->toString();
     *}
     */
    return ts;
}

string FPOperation::toStringV()
{
    string ts;
    unsigned i, j;
    ts = FPOpType2Str(type);
    for (i=0; i<numOpSets; i++) {
        FPOperandSet set = opSets[i];
        for (j=0; j<set.nIn; j++) {
            ts += "\n   in: " + set.in[j]->toStringV();
        }
        for (j=0; j<set.nOut; j++) {
            ts += "\n   out: " + set.out[j]->toStringV();
        }
    }
    /*
     *for (i=0; i<numInputs; i++) {
     *    ts += "\n   in: " + inputs[i]->toStringV();
     *}
     *for (i=0; i<numOutputs; i++) {
     *    ts += "\n   out: " + outputs[i]->toString();
     *}
     */
    return ts;
}

}

