#ifndef __FPFPSVCONFIGPOLICY_H
#define __FPFPSVCONFIGPOLICY_H

#include "FPSVPolicy.h"
#include "FPConfig.h"

namespace FPInst {

/**
 * Handles decision-making for config-based in-place replacement. Differs to the
 * settings in an FPConfig object for replacement decisions.
 */
class FPSVConfigPolicy : public FPSVPolicy {

    public:

        FPSVConfigPolicy(FPConfig *config);

        bool shouldInstrument(FPSemantics *inst);

        FPSVType getSVType();
        FPSVType getSVType(FPSemantics *inst);
        FPSVType getSVType(FPOperand *op, FPSemantics *inst);

        bool shouldReplaceWithPtr(FPOperand *op, FPSemantics *inst);

    private:

        FPSVType RETag2SVType(FPReplaceEntryTag tag);

        FPConfig *config;

};

}

#endif

