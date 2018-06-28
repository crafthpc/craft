/*
 * Pintool that extracts a floating-point data trace
 *
 * Mike Lam, LLNL, Summer 2016 and 2018
 */

#include <fstream>
#include <iostream>
#include <string>
#include <map>
using namespace std;

#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>

#include "pin.H"

extern "C" {
#include "xed-interface.h"
}

#ifndef PIN_H
#include <stdint.h>
typedef void*    ADDRINT;
typedef uint32_t UINT32;
typedef uint64_t UINT64;
#endif

#define MAX_STR_LEN     64
#define DEFAULT_OUT_FN  "{APP}-trace_pin-{PID}.log"

#define DUMP_BINARY 0
//#define DUMP_BINARY 1

/*
 * command-line options
 */
KNOB<string> KnobOutFile(KNOB_MODE_WRITEONCE, "pintool",
        "o", DEFAULT_OUT_FN, "output file name");

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
 * main output trace file
 */
static ofstream outFile;
static string outFileName;

/*
 * event data type
 */
typedef struct {
    ADDRINT ins;
    UINT32  dcount;     // 0, 1, or 2
    UINT64  data[2];
} FP_EVENT;

/*
 * instruction address => disassembly
 */
// TODO: change to unordered_map
map<ADDRINT, string> disas;

/*
 * saved effective address
 */
static ADDRINT effectiveAddr;

/*
 * utility: pretty print an event
 */
std::ostream& operator<<(std::ostream& os, const FP_EVENT &evt)
{
    os << "0x" << std::hex << evt.ins;
    os << " " << disas[evt.ins];
    if (evt.dcount > 0) {
        for (UINT32 i = 0; i < evt.dcount; i++) {
            if (i > 0) {
                os << endl;
                os << "0x" << std::hex << evt.ins;
                os << " " << disas[evt.ins];
            }
            os << "  ";
            os << *(double*)(&evt.data[i]);
        }
    }
    return os;
}

/*
 * utility: dump an event to the output file
 */
inline void dumpEvent(const FP_EVENT *evt)
{
    if (DUMP_BINARY) {
        outFile.write((const char*)evt, sizeof(FP_EVENT));
    } else {
        outFile << *evt << endl;
    }
}

/*
 * utility: strip the path out of a filename
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
 * utility: is the instruction part of the SSE/AVX instruction set?
 */
bool isSSE(INS ins)
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
 * runtime: add an FP_EVENT
 */
VOID fillBuffer(ADDRINT addr)
{
    FP_EVENT evt;
    evt.ins  = addr;
    evt.dcount = 0;
    dumpEvent(&evt);
}

/*
 * runtime: add an FP_EVENT that reads from memory
 */
VOID fillBufferMemRead64(ADDRINT addr, ADDRINT mem, UINT32 dcount)
{
    FP_EVENT evt;
    evt.ins  = addr;
    evt.dcount = dcount;
    evt.data[0] = *(UINT64*)mem;
    if (dcount == 2) {
        evt.data[1] = *(((UINT64*)mem)+1);
    }
    dumpEvent(&evt);
}

/*
 * runtime: save the effective address of a write instruction
 */
VOID saveEffectiveAddr(ADDRINT addr)
{
    effectiveAddr = addr;
}

/*
 * runtime: add an FP_EVENT that writes to memory at saved effective address
 */
VOID fillBufferMemWrite64(ADDRINT addr, UINT32 dcount)
{
    FP_EVENT evt;
    evt.ins  = addr;
    evt.dcount = dcount;
    evt.data[0] = *(UINT64*)effectiveAddr;
    if (dcount == 2) {
        evt.data[1] = *(((UINT64*)effectiveAddr)+1);
    }
    dumpEvent(&evt);
}

/*
 * instrumentation: add event for a binary operation instruction
 */
void handleBinOp(INS ins, UINT32 dcount)
{
    if (INS_IsMemoryRead(ins)) {
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)fillBufferMemRead64,
                IARG_INST_PTR, IARG_MEMORYREAD_EA,
                IARG_UINT32, dcount, IARG_END);
    } else if (INS_IsMemoryWrite(ins)) {
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)fillBufferMemWrite64,
                IARG_INST_PTR, IARG_MEMORYREAD_EA,
                IARG_UINT32, dcount, IARG_END);
    } else {
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)fillBuffer,
                IARG_INST_PTR, IARG_END);
    }
    totalInstructions++;
}

/*
 * instrumentation time: save main executable image name for output file name
 */
VOID handleImage(IMG img, VOID *)
{
    if (IMG_IsMainExecutable(img)) {

        // extract app name and pid
        appName = stripPath(IMG_Name(img).c_str());
        appPid = PIN_GetPid();

        // extract current host name
        char hostname[MAX_STR_LEN];
        if (!gethostname(hostname, MAX_STR_LEN)) {
            strncpy(hostname, "localhost", MAX_STR_LEN);
        }

        // open output file
        outFileName = KnobOutFile.Value().c_str();
        if (outFileName == DEFAULT_OUT_FN) {
            outFileName = appName + "-trace_pin-" + hostname + "-" + decstr(appPid) + ".log";
        }
        if (DUMP_BINARY) {
            outFile.open(outFileName.c_str(), ios::out | ios::binary);
        } else {
            outFile.open(outFileName.c_str());
        }
    }
}

/*
 * instrumentation: insert calls to event-generating functions
 */
VOID handleInstruction(INS ins, VOID *)
{
    // skip invalid routines for now (TODO: provide option to re-enable)
    RTN routine = INS_Rtn(ins);
    if (!RTN_Valid(routine)) {
        return;
    }

    // skip invalid images and shared libraries for now
    // (TODO: provide option to re-enable shared libraries)
    IMG image = SEC_Img(RTN_Sec(routine));
    if (!IMG_Valid(image) || IMG_Type(image) == IMG_TYPE_SHAREDLIB) {
        return;
    }

    // check non-floating-point instructions for movement to/from a
    // floating-point memory location
    if (!isSSE(ins)) {
        return;
    }

    // skip main for now (TODO: re-enable or add selective instrumentation)
    if (RTN_Name(routine) == "main") {
        return;
    }

    //LOG("Handling 0x" + hexstr(INS_Address(ins)) + ": " + INS_Disassemble(ins));
    //outFile << "handling " << INS_Disassemble(ins) << " in " << RTN_Name(routine) << endl;

    // save disassembly
    disas[INS_Address(ins)] = INS_Disassemble(ins);

    LEVEL_BASE::OPCODE iclass = INS_Opcode(ins);
    switch (iclass) {

        case XED_ICLASS_ADDSD:
        case XED_ICLASS_SUBSD:
        case XED_ICLASS_MULSD:
        case XED_ICLASS_DIVSD:
            handleBinOp(ins, 1); break;

        case XED_ICLASS_ADDPD:
        case XED_ICLASS_SUBPD:
        case XED_ICLASS_MULPD:
        case XED_ICLASS_DIVPD:
            handleBinOp(ins, 2); break;

        // flag anything we didn't know how to handle
        default:
            LOG("WARNING - unhandled instruction: " + INS_Disassemble(ins)
                + " (0x" + hexstr(INS_Address(ins)) + ")\n");
            break;
    }
}

/*
 * termination: dump all the instruction and log info
 */
VOID handleCleanup(INT32 code, VOID *v)
{
    // extract current host name
    char hostname[MAX_STR_LEN];
    if (!gethostname(hostname, MAX_STR_LEN)) {
        strncpy(hostname, "localhost", MAX_STR_LEN);
    }

    // close trace file
    outFile.close();

    // add metadata to main log file
    LOG("CRAFT: Analyzed " + appName + " [pid=" + decstr(appPid) +
            "] running on " + hostname + "\n");
    LOG("CRAFT: Handled " + decstr(totalInstructions) +
            " unique instruction(s)\n");
    LOG("CRAFT: Saved" + string(DUMP_BINARY ? " binary" : "") +
            " trace to " + outFileName + "\n");
}

int main(int argc, char* argv[])
{
    // initialize Pin
    PIN_InitSymbols();  // we want symbol info if present
    if (PIN_Init(argc, argv)) {
        PIN_ERROR("This Pintool extracts a trace of floating-point operations and values\n"
                + KNOB_BASE::StringKnobSummary() + "\n");
        return -1;
    }

    // uncomment to use AT&T syntax (to match the GNU debugger)
    PIN_SetSyntaxATT();

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

