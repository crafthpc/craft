#include "FPFilterFunc.h"

namespace FPInst {

bool FPFilterFunc::operator()(PatchAPI::Point::Type /*type*/, PatchAPI::Location loc, FPDecoder* decoder)
{
    if (!decoder || loc.type != PatchAPI::Location::InstructionInstance_) {
        return false;
    }
    return filter(loc.insn, decoder);
}

bool FPFilterFunc::filter(InstructionAPI::Instruction::Ptr inst, FPDecoder* decoder)
{
    unsigned char buffer[64];
    int size = inst->size();
    memcpy(buffer, inst->ptr(), size);
    return decoder->filter(buffer, size); 
}

}

