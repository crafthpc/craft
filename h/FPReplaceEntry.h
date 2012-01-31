#ifndef __FPREPLACEENTRY_H
#define __FPREPLACEENTRY_H

#include <stddef.h>
#include <string>

using namespace std;

namespace FPInst {

enum FPReplaceEntryType {
    RETYPE_APP, RETYPE_FUNCTION, RETYPE_BASICBLOCK, RETYPE_INSTRUCTION
};

enum FPReplaceEntryTag {
    RETAG_NONE, RETAG_IGNORE, RETAG_SINGLE, RETAG_DOUBLE
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

        FPReplaceEntryType type;
        size_t idx;

        FPReplaceEntry *parent;
        string name;
        void *address;
        FPReplaceEntryTag tag;

};

}

#endif
