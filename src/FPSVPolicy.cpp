#include "FPSVPolicy.h"

namespace FPInst {

FPSVPolicy::FPSVPolicy(FPSVType defaultType)
{
    this->defaultType = defaultType;
}

bool FPSVPolicy::shouldInstrument(FPSemantics *inst)
{
    bool instrument = false;

    // see if there's anything besides move, zero, stack, or noop operations
    FPOperation *op;
    for (unsigned k=0; k<inst->size(); k++) {
        op = (*inst)[k];
        FPOperationType t = op->getType();
        if (t != OP_NONE && t != OP_MOV && t != OP_ZERO && t != OP_PUSH_X87 && t != OP_POP_X87) {
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

    // special case   TODO: remove this?
    //if (inst->getDisassembly().find("btc") != string::npos) {
       //instrument = true;
    //}

    return instrument;
}

/*
 * These are the functions you'd override in a child class to determine the
 * type of particular data structures or subroutines.
 */
FPSVType FPSVPolicy::getSVType()
{
    return defaultType;
}
FPSVType FPSVPolicy::getSVType(FPSemantics * /*inst*/)
{
    return defaultType;
}
FPSVType FPSVPolicy::getSVType(FPOperand * /*op*/, FPSemantics * /*inst*/)
{
    return defaultType;
}

/*
 * This is the function you'd override in a child class to determine the
 * desired hierarchy between shadow value types.
 */
FPSVType FPSVPolicy::getCommonType(FPSVType type1, FPSVType type2)
{
    FPSVType type = SVT_IEEE_Single;
    if (type1 == type2) {
        type = type1;
    } else {
        if (type1 == SVT_NONE || type2 == SVT_NONE) {
            // nothing can operate with a NONE type...
            type = SVT_NONE;
        } else if (type1 == SVT_MPFR_Quad || type2 == SVT_MPFR_Quad) {
            type = SVT_MPFR_Quad;
        } else if (type1 == SVT_IEEE_Double || type2 == SVT_IEEE_Double) {
            type = SVT_IEEE_Double;
        } else if (type1 == SVT_IEEE_Single || type2 == SVT_IEEE_Single) {
            type = SVT_IEEE_Single;
        } else {
            // TODO: special handling for composite types?
            type = SVT_NONE;
        }
    }
    assert(type != SVT_NONE || (type1 == SVT_NONE && type2 == SVT_NONE));
    return type;
}

/*
 * This is the function you'd override in a child class to force certain
 * types of pointers to get written for particular data structures (via the
 * address of "op" or particular subroutines (via "inst").
 */
bool FPSVPolicy::shouldReplaceWithPtr(FPOperand *op, FPSemantics * /*inst*/)
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

        // replace singles if we're on a 32-bit architecture
        case IEEE_Single:
            if (sizeof(void*) <= sizeof(float)) {
                savePtr = true;
            }
            break;

        // always replace doubles and long doubles
        case IEEE_Double:
        case C99_LongDouble:
            savePtr = true;
            break;
    }
    return savePtr;
}

}

