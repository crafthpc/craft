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

string FPReplaceEntry::toString()
{
    stringstream ss;
    ss.clear(); ss.str("");
    ss << "  RE: " << name << " addr=" << address;
    switch (type) {
        case RETYPE_APP:         ss << " [app]";       break;
        case RETYPE_FUNCTION:    ss << " [func]";      break;
        case RETYPE_BASICBLOCK:  ss << " [bblk]";      break;
        case RETYPE_INSTRUCTION: ss << " [insn]";      break;
    }
    switch (tag) {
        case RETAG_SINGLE:  ss << " [single]";      break;
        case RETAG_DOUBLE:  ss << " [double]";      break;
        case RETAG_IGNORE:  ss << " [ignore]";      break;
        case RETAG_NONE:    break;
    }
    if (parent) {
       ss << " parent=" << parent->name;
    }
    return ss.str();
}

}

