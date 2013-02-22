#include "FPSVMemPolicy.h"

namespace FPInst {

FPSVMemPolicy::FPSVMemPolicy(FPSVType defaultType)
    : FPSVPolicy(defaultType)
{ }

bool FPSVMemPolicy::shouldInstrument(FPSemantics *inst)
{
    bool instrument = false;

    // see if there's anything besides zero, stack, or noop operations
    // NOTE: this differs from the vanilla policy in that it doesn not ignore
    // movement instructions (e.g., movsd)
    FPOperation *op;
    for (unsigned k=0; k<inst->size(); k++) {
        op = (*inst)[k];
        FPOperationType t = op->getType();
        if (t != OP_NONE && t != OP_ZERO && t != OP_PUSH_X87 && t != OP_POP_X87) {
            instrument = true;
        }
    }

    if (instrument && sizeof(float) < sizeof(void*) &&
       !(inst->hasOperandOfType(IEEE_Double) || 
         inst->hasOperandOfType(C99_LongDouble) ||
         inst->hasOperandOfType(SSE_Quad))) {

       // just ignore operations that don't involve
       // doubles, long doubles, or XMM regs on 64-bit
       // (can't replace 32-bit values w/ pointers)
       instrument = false;
    }

    // ignore long doubles or non-floating-point operations
    if (!(inst->hasOperandOfType(IEEE_Double) || inst->hasOperandOfType(SSE_Quad)) 
            || inst->hasOperandOfType(C99_LongDouble)) {
        instrument = false;
    }

    // check for a single, double, or candidate replacement tag
    if (instrument) {
        FPReplaceEntryTag tag = getDefaultRETag(inst);
        if (!(tag == RETAG_SINGLE || tag == RETAG_DOUBLE || tag == RETAG_CANDIDATE)) {
            instrument = false;
        }
    }

    //printf("FPSVMemPolicy::shouldInstrument(%s) = %s\n",
            //inst->getDisassembly().c_str(), (instrument ? "yes" : "no"));

    return instrument;
}

/*
 * These are the functions you'd override in a child class to determine the
 * type of particular data structures or subroutines.
 */
FPSVType FPSVMemPolicy::getSVType()
{
    return defaultType;
}
FPSVType FPSVMemPolicy::getSVType(FPSemantics *inst)
{
    FPReplaceEntryTag tag = getDefaultRETag(inst);
    if (tag == RETAG_CANDIDATE) {
        return defaultType;
    } else {
        return RETag2SVType(tag);
    }
}
FPSVType FPSVMemPolicy::getSVType(FPOperand * /*op*/, FPSemantics *inst)
{
    FPReplaceEntryTag tag = getDefaultRETag(inst);
    if (tag == RETAG_CANDIDATE) {
        return defaultType;
    } else {
        return RETag2SVType(tag);
    }
}

FPReplaceEntryTag FPSVMemPolicy::getDefaultRETag(FPSemantics * inst)
{
    // check for memory/register operands appropriately

    FPReplaceEntryTag tag = RETAG_IGNORE;
    
    bool hasMemoryOperand = false;
    bool hasMemoryOutputOperand = false;
    
    FPOperation *op;
    FPOperandSet *ops;
    FPOperand *opnd;
    size_t nops;

    // check each operation...
    for (unsigned k=0; k<inst->size(); k++) {
        op = (*inst)[k];
        op->getOperandSets(ops, nops);

        // check each operand set
        for (unsigned j=0; j<nops; j++) {
            unsigned i;

            // check each input
            for (i=0; i<ops[j].nIn; i++) {
                opnd = ops[j].in[i];
                if (opnd->isMemory()) {
                    hasMemoryOperand = true;
                }
            }
            
            // check each output
            for (i=0; i<ops[j].nOut; i++) {
                opnd = ops[j].out[i];
                if (opnd->isMemory()) {
                    hasMemoryOperand = true;
                    hasMemoryOutputOperand = true;
                }
            }
        }
    }

    if (hasMemoryOutputOperand) {
        tag = RETAG_CANDIDATE;          // we can try replacing these
                                        // (should only be movement instructions)
    } else if (hasMemoryOperand) {
        tag = RETAG_DOUBLE;             // we need to handle memory->reg conversion
    } else {
        tag = RETAG_IGNORE;             // we can ignore these
    }

    return tag;
}

/*
 * This is the function you'd override in a child class to force certain
 * types of pointers to get written for particular data structures (via the
 * address of "op" or particular subroutines (via "inst").
 */
bool FPSVMemPolicy::shouldReplaceWithPtr(FPOperand *op, FPSemantics *inst)
{
    bool savePtr = false;
    switch (op->getType()) {

        // integer types: don't replace
        case SignedInt8:  case UnsignedInt8:
        case SignedInt16: case UnsignedInt16:
        case SignedInt32: case UnsignedInt32:
        case SignedInt64: case UnsignedInt64:
        case SSE_Quad:
            break;

        // never replace singles
        case IEEE_Single:
            break;

        // always replace doubles and long doubles
        case IEEE_Double:
            savePtr = (getSVType(inst) == SVT_IEEE_Single);
            break;

        // ignore long doubles
        case C99_LongDouble:
            break;
    }
    return savePtr;
}

}

