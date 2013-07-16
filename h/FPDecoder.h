#ifndef __FPDECODER_H
#define __FPDECODER_H

#include <stdio.h>

#include "FPContext.h"
#include "FPSemantics.h"

namespace FPInst {

/**
 * Base class for an instruction decoder.
 * This library doesn't attempt to decode machine code bytes directly, but
 * differs the work to a decoder library. Having a base class allows us
 * to plug in different decoders if we wish.
 *
 * Currently, we have decoders that use XED (from Pin) and InstructionAPI (from
 * Dyninst), although the XED one has proven more reliable and the IAPI one has
 * not been maintained for a few months.
 */
class FPDecoder
{
    public:
        virtual bool filter(unsigned char *bytes, size_t nbytes) = 0;
        virtual FPSemantics* decode(unsigned long iidx, void *addr, unsigned char *bytes, size_t nbytes) = 0;
        virtual FPSemantics* lookup(unsigned long iidx) = 0;
        virtual FPSemantics* lookupByAddr(void* addr) = 0;
        virtual ~FPDecoder() { }
};

}

#endif

