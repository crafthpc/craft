#include "fpinfo.h"

FPAnalysisInfo allAnalysisInfo[] = {

    /* TEMPLATE:
    {   
        <singleton instance of FPAnalysis subclass>,
        <fpinst command-line parameter>, <fpinst usage text>,
        <fpinst help text (line 1)>,
        <fpinst help text (line 2)>
    }
    */

    // NOTE: null and decoding-only analyses are hard-coded;
    //       they do not have corresponding FPAnalysis subclasses
    //

    {   FPAnalysisCInst::getInstance(),
        "--cinst", "--cinst",
        "count all floating-point instructions",
        ""
    },

    {   FPAnalysisExample::getInstance(),
        "--dprint", "--dprint",
        "debug printing (example analysis",
        ""
    },
    
    {   FPAnalysisDCancel::getInstance(),
        "--dcancel", "--dcancel",
        "detect cancellation events",
        ""
    },

    {   FPAnalysisDNan::getInstance(),
        "--dnan", "--dnan",
        "detect NaN values",
        ""
    },

    {   FPAnalysisDNan::getInstance(),
        "--trange", "--trange",
        "track operand value ranges",
        ""
    },

    {   FPAnalysisPointer::getInstance(),
        "--svptr", "--svptr <policy>",
        "pointer-based replacement analysis",
        "valid policies: \"single\", \"double\""
    },

    {   FPAnalysisInplace::getInstance(),
        "--svinp", "--svinp <policy>",
        "in-place replacement analysis",
        "valid policies: \"single\", \"double\", \"mem_single\", \"mem_double\", \"config\""
    },

    {   FPAnalysisRPrec::getInstance(),
        "--rprec", "--rprec <bits>",
        "reduced-precision analysis",
        ""
    }

    // NEWMODE: add info for new analyses here
    //
    
};

