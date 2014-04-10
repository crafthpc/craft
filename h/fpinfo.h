#ifndef __FPINFO_H
#define __FPINFO_H

// STL classes
#include <string>

// analyses
#include "FPAnalysis.h"
#include "FPAnalysisCInst.h"
#include "FPAnalysisExample.h"
#include "FPAnalysisDCancel.h"
#include "FPAnalysisDNan.h"
#include "FPAnalysisTRange.h"
#include "FPAnalysisPointer.h"
#include "FPAnalysisInplace.h"
#include "FPAnalysisRPrec.h"

using namespace FPInst;


/**
 * FPAnalysisInfo - Stores info about FPAnalysis subclasses for the
 *                  fpinst mutator
 */
struct FPAnalysisInfo {

        FPAnalysis *instance;       // pointer to analysis instance at instrumentation or run time

        const char* fpinstSwitch;        // command-line parameter to enable (i.e., "--cinst")
        const char* fpinstParam;         // if present, the parameter immediately following the switch
                                         // will be added to the configuration using this setting name;
                                         // specify NULL for no parameter

        const char* fpinstUsage;         // usage text for "-h"
        const char* fpinstHelpText;      // description for "-h"
        const char* fpinstHelpText2;     // second line help for "-h" (ignored if blank)
};


// total number of valid analyses
// NEWMODE: increment this number
//
const size_t TOTAL_ANALYSIS_COUNT = 8;


/**
 * Hard-coded structure that serves as the glue between the fpinst/fpanalysis
 * object files and the FPAnalysis subclasses. The fpinst mutator uses all of
 * the command-line and usage/help info, while the fpanalysis library uses the
 * instance/tag <-> index (id) mapping.
 *
 * NEWMODE: update the definition of this structure in fpinfo.cpp
 */
extern FPAnalysisInfo allAnalysisInfo[];


#endif

