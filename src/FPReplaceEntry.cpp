#include "FPReplaceEntry.h"

namespace FPInst {

FPReplaceEntry::FPReplaceEntry()
{
    this->type = RETYPE_APP;
    this->idx = 0;
    this->parent = NULL;
    this->name = "";
    this->address = NULL;
    this->tag = RETAG_NONE;
}

FPReplaceEntry::FPReplaceEntry(FPReplaceEntryType type, size_t idx)
{
    this->type = type;
    this->idx = idx;
    this->parent = NULL;
    this->name = "";
    this->address = NULL;
    this->tag = RETAG_NONE;
}

FPReplaceEntryTag FPReplaceEntry::getEffectiveTag()
{
    FPReplaceEntryTag rtag = this->tag;
    if (this->parent != NULL) {
        FPReplaceEntryTag ptag = this->parent->getEffectiveTag();
        if (ptag != RETAG_NONE) {
            rtag = ptag;
        }
    }
    return rtag;
}

}

