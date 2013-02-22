#ifndef __FPFPSVMEMPOLICY_H
#define __FPFPSVMEMPOLICY_H

#include "FPSVPolicy.h"
#include "FPConfig.h"

namespace FPInst {

/**
 * Handles decision-making for memory-based replacement analysis.
 *
 * If an instruction is purely SSE register-based, it is ignored.
 * If an instruction reads a double from memory, it is replaced with doubles.
 * If an instruction write a double to memory, it is labeled as a candidate.
 */
class FPSVMemPolicy : public FPSVPolicy {

    public:

        FPSVMemPolicy(FPSVType defaultType);

        bool shouldInstrument(FPSemantics *inst);

        FPSVType getSVType();
        FPSVType getSVType(FPSemantics *inst);
        FPSVType getSVType(FPOperand *op, FPSemantics *inst);
        FPReplaceEntryTag getDefaultRETag(FPSemantics *inst);

        bool shouldReplaceWithPtr(FPOperand *op, FPSemantics *inst);

    private:

        FPConfig *config;

};

}

#endif

