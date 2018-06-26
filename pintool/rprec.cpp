/*
 * Pin version of CRAFT rprec mode.
 *
 * Mike Lam, LLNL, Summer 2016
 */

#include <iostream>
#include <fstream>
#include <sstream>

#include <string.h>
#include <unistd.h>

#include "pin.H"
//#include "pin_isa.H"

extern "C" {
#include "xed-interface.h"
}

#define MAX_STR_LEN     64

using namespace std;

/*
 * command-line options
 */
KNOB<UINT32> KnobPrecLevel(KNOB_MODE_WRITEONCE, "pintool",
        "r", "52", "# of bits of precision to be preserved");

/*
 * application info (used to build output file name)
 */
static string appName = "default";
static INT    appPid  = 0;

/*
 * total number of instructions instrumented (excluding duplicates)
 */
static unsigned long totalInstructions = 0;

/*
 * truncation constants
 */
static UINT64 DFLAG = 0xffffffffffffffff;
static UINT64 FFLAG = 0xffffffff;

/*
 * utility method: strip the path out of a filename
 */
const char* stripPath(const char *fn)
{
    const char *name = strrchr(fn, '/');
    if (name) {
        return name+1;
    } else {
        return fn;
    }
}

/*
 * criteria that make an instruction relevant to floating-point analysis
 */
bool shouldInstrument(INS ins)
{
    // extract instruction info
    INT32 icategory = INS_Category(ins);
    INT32 iextension = INS_Extension(ins);

    // check for floating-point instructions
    return (
        icategory == XED_CATEGORY_SSE ||
        icategory == XED_CATEGORY_AVX ||
        icategory == XED_CATEGORY_AVX2 ||
        iextension == XED_EXTENSION_SSE ||
        iextension == XED_EXTENSION_SSE2 ||
        iextension == XED_EXTENSION_SSE3 ||
        iextension == XED_EXTENSION_SSE4 ||
        iextension == XED_EXTENSION_SSE4A
    );
}

/*
 * criteria to check single-precision operands in an instruction
 */
bool isFP32(INS ins)
{
    LEVEL_BASE::OPCODE iclass = INS_Opcode(ins);
    switch (iclass) {
        case XED_ICLASS_ADDSS: case XED_ICLASS_ADDPS:
        case XED_ICLASS_SUBSS: case XED_ICLASS_SUBPS:
        case XED_ICLASS_MULSS: case XED_ICLASS_MULPS:
        case XED_ICLASS_DIVSS: case XED_ICLASS_DIVPS:
        case XED_ICLASS_MINSS: case XED_ICLASS_MINPS:
        case XED_ICLASS_MAXSS: case XED_ICLASS_MAXPS:
        case XED_ICLASS_SQRTSS: case XED_ICLASS_SQRTPS:
        case XED_ICLASS_CMPSS: case XED_ICLASS_CMPPS:
        case XED_ICLASS_COMISS: case XED_ICLASS_UCOMISS:
            return true;
    }
    return false;
}

/*
 * criteria to check double-precision operands in an instruction
 */
bool isFP64(INS ins)
{
    LEVEL_BASE::OPCODE iclass = INS_Opcode(ins);
    switch (iclass) {
        case XED_ICLASS_ADDSD: case XED_ICLASS_ADDPD:
        case XED_ICLASS_SUBSD: case XED_ICLASS_SUBPD:
        case XED_ICLASS_MULSD: case XED_ICLASS_MULPD:
        case XED_ICLASS_DIVSD: case XED_ICLASS_DIVPD:
        case XED_ICLASS_MINSD: case XED_ICLASS_MINPD:
        case XED_ICLASS_MAXSD: case XED_ICLASS_MAXPD:
        case XED_ICLASS_SQRTSD: case XED_ICLASS_SQRTPD:
        case XED_ICLASS_CMPSD: case XED_ICLASS_CMPPD:
        case XED_ICLASS_COMISD: case XED_ICLASS_UCOMISD:
            return true;
    }
    return false;
}

/*
 * criteria to check packed floating-point operands in an instruction
 */
bool isPackedSSE(INS ins)
{
    LEVEL_BASE::OPCODE iclass = INS_Opcode(ins);
    switch (iclass) {
        case XED_ICLASS_ADDPS: case XED_ICLASS_ADDPD:
        case XED_ICLASS_SUBPS: case XED_ICLASS_SUBPD:
        case XED_ICLASS_MULPS: case XED_ICLASS_MULPD:
        case XED_ICLASS_DIVPS: case XED_ICLASS_DIVPD:
        case XED_ICLASS_MINPS: case XED_ICLASS_MINPD:
        case XED_ICLASS_MAXPS: case XED_ICLASS_MAXPD:
        case XED_ICLASS_SQRTPS: case XED_ICLASS_SQRTPD:
        case XED_ICLASS_CMPPS: case XED_ICLASS_CMPPD:
            return true;
    }
    return false;
}

/*
 * run time: update min/max trackers for various kinds of operands
 */

VOID truncateMem32(UINT32 *loc)
{
    //cout << "truncating " << *(float*)loc;
    *loc = *loc & FFLAG;
    //cout << " to " << *(float*)loc << endl;
}

VOID truncateReg32(PIN_REGISTER *reg)
{
    truncateMem32((UINT32*)&(reg->flt[0]));
}

VOID truncateReg32Packed(PIN_REGISTER *reg)
{
    truncateMem32((UINT32*)&(reg->flt[0]));
    truncateMem32((UINT32*)&(reg->flt[1]));
    truncateMem32((UINT32*)&(reg->flt[2]));
    truncateMem32((UINT32*)&(reg->flt[3]));
}

VOID truncateMem64(UINT64 *loc)
{
    //cout << "truncating " << *(double*)loc;
    *loc = *loc & DFLAG;
    //cout << " to " << *(double*)loc << endl;
}

VOID truncateReg64(PIN_REGISTER *reg)
{
    truncateMem64((UINT64*)&(reg->dbl[0]));
}

VOID truncateReg64Packed(PIN_REGISTER *reg)
{
    truncateMem64((UINT64*)&(reg->dbl[0]));
    truncateMem64((UINT64*)&(reg->dbl[1]));
}

/*
 * instrumentation time: save main executable image name for output file name
 */
VOID handleImage(IMG img, VOID *)
{
    if (IMG_IsMainExecutable(img)) {
        appName = stripPath(IMG_Name(img).c_str());
        appPid = PIN_GetPid();
    }
}

/*
 * instrumentation time: insert calls to "truncate" functions
 */
VOID handleInstruction(INS ins, VOID *)
{
    if (shouldInstrument(ins)) {
        if (INS_IsMemoryWrite(ins)) {
            if (INS_MemoryWriteSize(ins) == 8) {

                // double-precision memory operand
                INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)truncateMem64,
                        IARG_MEMORYWRITE_EA,
                        IARG_END);

            } else if (INS_MemoryWriteSize(ins) == 4) {

                // single-precision memory operand
                INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)truncateMem32,
                        IARG_MEMORYWRITE_EA,
                        IARG_END);
            }
        }

        for (UINT32 r = 0; r < INS_MaxNumWRegs(ins); r++) {
            REG reg = INS_RegW(ins, r);
            if (REG_is_xmm(reg) && isFP64(ins)) {
                if (isPackedSSE(ins)) {

                    // double-precision packed register operand
                    INS_InsertCall(ins, IPOINT_BEFORE,
                            (AFUNPTR)truncateReg64Packed,
                            IARG_REG_REFERENCE, reg,
                            IARG_END);

                } else {

                    // double-precision scalar register operand
                    INS_InsertCall(ins, IPOINT_BEFORE,
                            (AFUNPTR)truncateReg64,
                            IARG_REG_REFERENCE, reg,
                            IARG_END);
                }

            } else if (REG_is_xmm(reg) && isFP32(ins)) {
                if (isPackedSSE(ins)) {

                    // single-precision packed register operand
                    INS_InsertCall(ins, IPOINT_BEFORE,
                            (AFUNPTR)truncateReg32Packed,
                            IARG_REG_REFERENCE, reg,
                            IARG_END);

                } else {

                    // single-precision scalar register operand
                    INS_InsertCall(ins, IPOINT_BEFORE,
                            (AFUNPTR)truncateReg32,
                            IARG_REG_REFERENCE, reg,
                            IARG_END);
                }
            }
        }
    }
}

/*
 * termination: log info
 */
VOID handleCleanup(INT32 code, VOID *v)
{
    // extract current host name
    char hostname[MAX_STR_LEN];
    if (!gethostname(hostname, MAX_STR_LEN)) {
        strncpy(hostname, "localhost", MAX_STR_LEN);
    }

    // add metadata to main log file
    LOG("CRAFT: Analyzed " + appName + " [pid=" + decstr(appPid) +
            "] running on " + hostname + "\n");
    LOG("CRAFT: Handled " + decstr(totalInstructions) +
            " unique instruction(s)\n");
    LOG("CRAFT: Performed reduced-precision analysis with " +
            decstr(KnobPrecLevel.Value()) + " bits preserved\n");
}

int main(int argc, char* argv[])
{
    // initialize Pin
    PIN_InitSymbols();  // we want symbol info if present
    if (PIN_Init(argc, argv)) {
        PIN_ERROR("This Pintool simulates reduced precision via truncation"
                  " of floating-point output operands\n"
                + KNOB_BASE::StringKnobSummary() + "\n");
        return -1;
    }

    // uncomment to use AT&T syntax (to match the GNU debugger)
    //PIN_SetSyntaxATT();

    UINT32 keep  = KnobPrecLevel.Value();
    if (keep > 52) {
        PIN_ERROR("Invalid precision level (must be >= 0 and <= 52)\n"
                + KNOB_BASE::StringKnobSummary() + "\n");
        return -1;
    }
    UINT32 trunc64 = 52 - keep;
    UINT32 trunc32 = (keep < 23 ? 23 - keep : 0);
    for (UINT64 bit64 = 0x1, i = 0; i < trunc64; bit64 <<= 1, i++) {
        DFLAG -= bit64;
    }
    for (UINT32 bit32 = 0x1, j = 0; j < trunc32; bit32 <<= 1, j++) {
        FFLAG -= bit32;
    }
    /*
     *cout << "keep: " << std::dec << KnobPrecLevel.Value()
     *     << " trunc64: " << std::dec << trunc64
     *     << " flag: " << std::hex << DFLAG
     *     << " trunc32: " << std::dec << trunc32
     *     << " flag: " << std::hex << FFLAG
     *     << endl;
     */

    // register image callback
    IMG_AddInstrumentFunction(handleImage, 0);

    // register instruction callback
    INS_AddInstrumentFunction(handleInstruction, 0);

    // register cleanup callback
    PIN_AddFiniFunction(handleCleanup, 0);

    // begin execution
    PIN_StartProgram();

    return 0;
}

