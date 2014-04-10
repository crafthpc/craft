#include "fpinfo.h"

FPAnalysisInfo allAnalysisInfo[] = {

    /* TEMPLATE:
    {   
        <singleton instance of FPAnalysis subclass>,
        <fpinst command-line switch>, <fpinst command-line parameter> (optional),
        <fpinst usage text>,
        <fpinst help text (line 1)>,
        <fpinst help text (line 2)>
    }
    */

    // NOTE: null and decoding-only analyses are hard-coded;
    //       they do not have corresponding FPAnalysis subclasses
    //

    {   FPAnalysisCInst::getInstance(),
        "--cinst", NULL,
        "--cinst",
        "count all floating-point instructions",
        ""
    },

    {   FPAnalysisExample::getInstance(),
        "--dprint", NULL,
        "--dprint",
        "debug printing (example analysis",
        ""
    },
    
    {   FPAnalysisDCancel::getInstance(),
        "--dcancel", NULL,
        "--dcancel",
        "detect cancellation events",
        ""
    },

    {   FPAnalysisDNan::getInstance(),
        "--dnan", NULL,
        "--dnan",
        "detect NaN values",
        ""
    },

    {   FPAnalysisTRange::getInstance(),
        "--trange", NULL,
        "--trange",
        "track operand value ranges",
        ""
    },

    {   FPAnalysisPointer::getInstance(),
        "--svptr", "sv_ptr_type",
        "--svptr <policy>",
        "pointer-based replacement analysis",
        "valid policies: \"single\", \"double\""
    },

    {   FPAnalysisInplace::getInstance(),
        "--svinp", "sv_inp_type",
        "--svinp <policy>",
        "in-place replacement analysis",
        "valid policies: \"single\", \"double\", \"mem_single\", \"mem_double\", \"config\""
    },

    {   FPAnalysisRPrec::getInstance(),
        "--rprec", "r_prec_default_precision",
        "--rprec <bits>",
        "reduced-precision analysis",
        ""
    }

    // NEWMODE: add info for new analyses here
    //
    
};

