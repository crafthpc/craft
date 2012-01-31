#ifndef __FPSHADOWENTRY_H
#define __FPSHADOWENTRY_H

#include "FPLog.h"
#include "FPContext.h"

using namespace std;

namespace FPInst {

enum FPShadowEntryType {
    INIT, REPORT, NOREPORT
};

/**
 * Stores a user-requested shadow value output entry. The user can specify
 * these in a mutator configuration file, and the analysis will try to output
 * any given locations at the end of a shadow value analysis run. At one point,
 * you could also use "INIT" entries to initialize values, but this feature has
 * not been supported for a while.
 */
class FPShadowEntry
{
    public:

        FPOperandAddress getAddress(FPLog *log, FPContext *context);

        FPShadowEntryType type;     ///< INIT or REPORT/NOREPORT
        string name;                ///< source name or hex address
        bool indirect;              ///< is it a pointer?
        unsigned size[2];           ///< array or matrix dimensions
        unsigned isize;             ///< individual entry size
        vector<string> vals;        ///< initialization values
};

}

#endif
