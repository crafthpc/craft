#ifndef __FPREPLACEENTRY_H
#define __FPREPLACEENTRY_H

#include <stddef.h>
#include <string>
#include <sstream>

using namespace std;

namespace FPInst {

enum FPReplaceEntryType {
    RETYPE_APP, RETYPE_MODULE, RETYPE_FUNCTION, RETYPE_BASICBLOCK, RETYPE_INSTRUCTION
};

enum FPReplaceEntryTag {
    RETAG_NONE,         // yield to parent in tree
    RETAG_IGNORE,       // do not replace or instrument
    RETAG_SINGLE,       // replace with svinp single
    RETAG_DOUBLE,       // replace with svinp double
    RETAG_CANDIDATE,    // placeholder for automatic search
    RETAG_NULL,         // null snippet
    RETAG_CINST,        // instruction counting
    RETAG_DCANCEL,      // cancellation detection
    RETAG_DNAN,         // NaN detection
    RETAG_TRANGE        // range tracking
};

/**
 * Stores a configuration entry for replacement.
 */
class FPReplaceEntry
{
    public:
        
        FPReplaceEntry();
        FPReplaceEntry(FPReplaceEntryType type, size_t idx);

        FPReplaceEntryTag getEffectiveTag();

        string toString();

        FPReplaceEntryType type;
        size_t idx;

        FPReplaceEntry *parent;
        string name;
        void *address;
        FPReplaceEntryTag tag;

};

}

#endif
