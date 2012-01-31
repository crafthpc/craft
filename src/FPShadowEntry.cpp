#include "FPShadowEntry.h"

namespace FPInst {

FPOperandAddress FPShadowEntry::getAddress(FPLog *log, FPContext *context)
{
    FPOperandAddress addr, maddr;
    size_t n, j;

    // check for hex address
    n = sscanf(name.c_str(), "%lx", &j);
    if (n > 0) {
        addr = (FPOperandAddress)j;
    } else {
        // look up variable name
        addr = log->getGlobalVarAddr(name);
    }
    if (addr != 0) { // valid address
        if (indirect) {
            // pointer -- apply indirection
            context->getMemoryValue((void*)&maddr, addr, sizeof(void*));
            if (maddr) {
                //cout << "  indirect (" << hex << addr << " -> " << maddr << dec << ")";
                addr = maddr;
            }
        }
    }

    return addr;
}

}

