#include "FPSemantics.h"

namespace FPInst {

FPSemantics::FPSemantics()
{
    assembly = "";
    iidx = 0;
    address = NULL;
    bytes = NULL;
    nbytes = 0;
    numOps = 0;
}

void FPSemantics::setDisassembly(string code)
{
    assembly = code;
}

string FPSemantics::getDisassembly()
{
    return assembly;
}

void FPSemantics::setDecoderDebugData(string data)
{
    decoderDebugData = data;
}

string FPSemantics::getDecoderDebugData()
{
    return decoderDebugData;
}

void FPSemantics::setIndex(unsigned long index)
{
    iidx = index;
}

unsigned long FPSemantics::getIndex()
{
    return iidx;
}

void FPSemantics::setAddress(void *addr)
{
    address = addr;
}

void* FPSemantics::getAddress()
{
    return address;
}

void FPSemantics::setBytes(unsigned char *bytes, size_t nbytes)
{
    this->bytes = (unsigned char*)malloc(nbytes * sizeof(unsigned char));
    if (!(this->bytes)) {
        fprintf(stderr, "Out of memory!\n");
        exit(-1);
    }
    this->nbytes = nbytes;
    memcpy(this->bytes, bytes, nbytes);
}

size_t FPSemantics::getNumBytes() {
    return (size_t)nbytes;
}

void FPSemantics::getBytes(unsigned char* dest) {
    size_t i;
    for (i=0; i<nbytes; i++) {
        dest[i] = bytes[i];
    }
}

bool FPSemantics::isValid() {
    if (numOps == 0) return false;
    for (size_t k=0; k<numOps; k++) {
        if (ops[k]->getType() == OP_INVALID) {
            return false;
        }
    }
    return true;
}

void FPSemantics::refresh(FPContext *context)
{
    size_t i;
    for (i=0; i<numOps; i++) {
        ops[i]->refreshAllOperands(context);
    }
}

void FPSemantics::add(FPOperation *newOp)
{
    ops[numOps] = newOp;
    numOps++;
}

size_t FPSemantics::size()
{
    return numOps;
}

FPOperation*& FPSemantics::operator[](const size_t index)
{
    return ops[index];
}

bool FPSemantics::hasOperandOfType(FPOperandType type)
{
    bool hasOp = false;
    size_t i;
    for (i=0; !hasOp && i<numOps; i++) {
        if (ops[i]->hasOperandOfType(type)) {
            hasOp = true;
        }
    }
    return hasOp;
}

void FPSemantics::getNeededRegisters(set<FPRegister> &regs)
{
    size_t i;
    for (i=0; i<numOps; i++) {
        ops[i]->getNeededRegisters(regs);
    }
}

void FPSemantics::getModifiedRegisters(set<FPRegister> &regs)
{
    size_t i;
    for (i=0; i<numOps; i++) {
        ops[i]->getModifiedRegisters(regs);
    }
}

string FPSemantics::toString()
{
    stringstream ss;
    ss.clear();
    ss.str("");
    size_t i;
    unsigned char numc;
    unsigned long numi;

    // address, bytes, and assembly
    ss << hex << address;
    for (i=0; i<nbytes; i++) {
        if (i==0) ss << "  [";
        else ss << " ";
        numc = (unsigned char)bytes[i];
        numi = (unsigned)numc;
        ss << setw(2) << setfill('0') << numi;
        if (i==nbytes-1) ss << "]";
    }
    ss << dec;
    if (assembly != "")
        ss << "  \"" << assembly << "\"";
    
    // required registers
    ss << endl << "  needed regs: ";
    set<FPRegister> regs;
    getNeededRegisters(regs);
    set<FPRegister>::iterator j;
    for (j=regs.begin(); j!=regs.end(); j++) {
        ss << FPContext::FPReg2Str(*j) << " ";
    }
    ss << endl << "  modified regs: ";
    regs.clear();
    getModifiedRegisters(regs);
    for (j=regs.begin(); j!=regs.end(); j++) {
        ss << FPContext::FPReg2Str(*j) << " ";
    }

    // individual operations
    for (i=0; i<numOps; i++) {
        ss << endl << ops[i]->toString();
    }

    return ss.str();
}

}

