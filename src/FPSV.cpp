#include "FPSV.h"

namespace FPInst {

FPSV::FPSV(FPSVType type, FPOperandAddress addr)
{
    this->type = type;
    this->addr = addr;
    this->seed_size = 0;
    /*
     * NOT NECESSARY, DISABLED FOR SPEED
     *
     *for (size_t i = 0; i<this->seed_size; i++) {
     *    this->seed_data[i] = (uint32_t)0;
     *}
     */
}

FPSV::~FPSV()
{
}

void FPSV::setSeedData(FPSV *base)
{
    //cout << "base size=" << base->seed_size << endl;
    this->seed_size = base->seed_size;
    for (size_t i = 0; i<base->seed_size; i++) {
        this->seed_data[i] = base->seed_data[i];
    }
}

string FPSV::toString()
{
    return "INVALID SHADOW VALUE";
}

string FPSV::toDetailedString()
{
    return "INVALID SHADOW VALUE";
}

string FPSV::getTypeString()
{
    return "INVALID SV TYPE";
}

}
