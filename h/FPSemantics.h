#ifndef __FPSEMANTICS_H
#define __FPSEMANTICS_H

#include <iostream>
#include <iomanip>
#include <string>

#include "FPOperation.h"

using namespace std;

namespace FPInst {

/**
 * Represents a single machine-code instruction.
 * Stores the semantics of the instruction as a list of FPOperation objects
 * (which themselves contain FPOperand objects). Also stores some miscellaneous
 * useful information about the original machine code instruction if available,
 * such as the decoded assembly sting, the original program address, and the
 * instruction length. The size() and operator[] functions have been overloaded,
 * allowing client classes to use the FPSemantics as a makeshift array.
 */
class FPSemantics {

    public:

        FPSemantics();

        void setDisassembly(string code);
        string getDisassembly();
        void setDecoderDebugData(string data);
        string getDecoderDebugData();
        void setIndex(unsigned long index);
        unsigned long getIndex();
        void setAddress(void *addr);
        void* getAddress();
        size_t getNumBytes();
        void getBytes(unsigned char *dest);
        void setBytes(unsigned char *bytes, size_t nbytes);

        /**
         * Returns true if none of the operations are of type OP_INVALID.
         */
        bool isValid();

        /**
         * Refreshes all of the operands in all of the operations for this
         * instruction.
         */
        void refresh(FPContext *context);

        /**
         * Add a new operation to the end of the list.
         */
        void add(FPOperation *newOp);

        /**
         * Searches all the suboperations for an operand of the given type.
         * Returns true if one exists. This is useful for queries in FPSVPolicy
         * routines.
         */
        bool hasOperandOfType(FPOperandType type);
        
        /**
         * Fills the given set with all the registers that would be needed to
         * refresh the addresses and values of input registers, as well as to
         * refresh the addresses (but not values) of output registers.
         */
        void getNeededRegisters(set<FPRegister> &regs);

        /**
         * Fills the given set with all the registers that may need to be
         * changed by the operation.
         */
        void getModifiedRegisters(set<FPRegister> &regs);

        size_t numOps;  ///< number of operations; made public for speed
        size_t size();  ///< retrieve the number of operations
        
        /**
         * Returns the nth operation.
         */
        FPOperation*& operator[](const size_t index);

        string toString();

    private:

        string assembly;
        string decoderDebugData;
        unsigned long iidx;
        void *address;
        unsigned char *bytes;
        size_t nbytes;

        FPOperation* ops[4]; // may need to increase size at some point

};

}

#endif
