#include "FPSVConfigPolicy.h"

namespace FPInst {

FPSVConfigPolicy::FPSVConfigPolicy(FPConfig *config)
    : FPSVPolicy(SVT_NONE)
{
    this->config = config;
}

FPSVType FPSVConfigPolicy::RETag2SVType(FPReplaceEntryTag tag)
{
    FPSVType t = SVT_NONE;
    switch (tag) {
        case RETAG_DOUBLE:      t = SVT_IEEE_Double;    break;
        case RETAG_SINGLE:      t = SVT_IEEE_Single;    break;
        default:                t = SVT_NONE;           break;
    }
    return t;
}

bool FPSVConfigPolicy::shouldInstrument(FPSemantics *inst)
{
    FPReplaceEntryTag tag = config->getReplaceTag(inst->getAddress());
    //cout << " shouldInstrument(" << inst->getDisassembly() << ") = " 
        //<< (tag == RETAG_SINGLE ? "single" : "not single") << endl;
    return (tag == RETAG_SINGLE || tag == RETAG_DOUBLE);
}

FPSVType FPSVConfigPolicy::getSVType()
{
    return SVT_IEEE_Double;
}
FPSVType FPSVConfigPolicy::getSVType(FPSemantics *inst)
{
    /*
     *printf("getSVType(%p) = ", inst->getAddress());
     *if (config->getReplaceTag(inst->getAddress()) == RETAG_SINGLE) {
     *    printf("single\n");
     *} else if (config->getReplaceTag(inst->getAddress()) == RETAG_DOUBLE) {
     *    printf("double\n");
     *} else {
     *    printf("none\n");
     *}
     */
    return RETag2SVType(config->getReplaceTag(inst->getAddress()));
}
FPSVType FPSVConfigPolicy::getSVType(FPOperand * /*op*/, FPSemantics *inst)
{
    return RETag2SVType(config->getReplaceTag(inst->getAddress()));
}

/*
 * This is the function you'd override in a child class to force certain
 * types of pointers to get written for particular data structures (via the
 * address of "op" or particular subroutines (via "inst").
 */
bool FPSVConfigPolicy::shouldReplaceWithPtr(FPOperand *op, FPSemantics *inst)
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

