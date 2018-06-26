/*
 * Pin version of CRAFT trange mode.
 *
 * Mike Lam, LLNL, Summer 2016
 */

#include <iostream>
#include <fstream>
#include <sstream>

#include <float.h>
#include <string.h>
#include <unistd.h>

#include "pin.H"
//#include "pin_isa.H"

extern "C" {
#include "xed-interface.h"
}

#define MAX_STR_LEN     64
#define DEFAULT_OUT_FN  "{APP}-trange_pin-{PID}.log"

using namespace std;

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
 * information maintained for each floating-point instruction
 */
typedef struct InsInfo
{
    // main data (id and count)
    ADDRINT addr;               // address
    double  min;                // minimum value read by instruction
    double  max;                // maximum value read by instruction

    // debug and semantic info
    string  disas;              // disassembly
    string  routine;            // containing routine name
    string  image;              // containing image
    string  source;             // source filename
    INT32   lineno;             // source line number

    // structure link
    struct InsInfo *next;       // next struct in linked list

} INS_INFO;

/*
 * linked list of instruction info structs
 */
static INS_INFO *insList = NULL;

/*
 * total number of instructions instrumented (excluding duplicates)
 */
static unsigned long totalInstructions = 0;

/*
 * min/max floating-point constants
 */
static double DMAX = DBL_MAX;
static double DMIN = DBL_MIN;

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

VOID updateMem32(const float *loc, double *min, double *max)
{
    if ((double)*loc < *min) {
        *min = (float)*loc;
    }
    if ((double)*loc > *max) {
        *max = (float)*loc;
    }
}

VOID updateReg32(const PIN_REGISTER *reg, double *min, double *max)
{
    updateMem32((float*)&(reg->flt[0]), min, max);
}

VOID updateReg32Packed(const PIN_REGISTER *reg, double *min, double *max)
{
    updateMem32((float*)&(reg->flt[0]), min, max);
    updateMem32((float*)&(reg->flt[1]), min, max);
    updateMem32((float*)&(reg->flt[2]), min, max);
    updateMem32((float*)&(reg->flt[3]), min, max);
}

VOID updateMem64(const double *loc, double *min, double *max)
{
    if (*loc < *min) {
        *min = *loc;
    }
    if (*loc > *max) {
        *max = *loc;
    }
}

VOID updateReg64(const PIN_REGISTER *reg, double *min, double *max)
{
    updateMem64((double*)&(reg->dbl[0]), min, max);
}

VOID updateReg64Packed(const PIN_REGISTER *reg, double *min, double *max)
{
    updateMem64((double*)&(reg->dbl[0]), min, max);
    updateMem64((double*)&(reg->dbl[1]), min, max);
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
 * instrumentation time: insert calls to "update" functions
 */
VOID handleInstruction(INS ins, VOID *)
{
    INT32 col;      // temporary source info variables
    INT32 line;
    string fn;

    if (shouldInstrument(ins)) {

        // extract address (unique identifier)
        ADDRINT addr = INS_Address(ins);

        INS_INFO *info = NULL;

        // check to see if we've already added this instruction
        for (INS_INFO *ii = insList; ii != NULL; ii = ii->next) {
            if (ii->addr == addr) {
                info = ii;
                break;
            }
        }

        // initialize information in instruction table
        if (info == NULL) {
            info = new INS_INFO;
            info->addr = addr;
            info->min = DMAX;
            info->max = DMIN;
            info->disas = INS_Disassemble(ins);
            RTN rtn = INS_Rtn(ins);
            if (RTN_Valid(rtn)) {
                info->routine = RTN_Name(rtn);
                info->image = stripPath(IMG_Name(SEC_Img(RTN_Sec(rtn))).c_str());
            } else {
                info->routine = "";
                info->image = "";
            }
            PIN_GetSourceLocation(addr, &col, &line, &fn);
            info->source = stripPath(fn.c_str());
            info->lineno = line;
            info->next = insList;
            insList = info;
            totalInstructions++;
        }

        if (INS_IsMemoryRead(ins)) {
            if (INS_MemoryReadSize(ins) == 8) {

                // double-precision memory operand
                INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)updateMem64,
                        IARG_MEMORYREAD_EA,
                        IARG_PTR, &(info->min),
                        IARG_PTR, &(info->max),
                        IARG_END);

            } else if (INS_MemoryReadSize(ins) == 4) {

                // single-precision memory operand
                INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)updateMem32,
                        IARG_MEMORYREAD_EA,
                        IARG_PTR, &(info->min),
                        IARG_PTR, &(info->max),
                        IARG_END);
            }
        }

        for (UINT32 r = 0; r < INS_MaxNumRRegs(ins); r++) {
            REG reg = INS_RegR(ins, r);
            if (REG_is_xmm(reg) && isFP64(ins)) {
                if (isPackedSSE(ins)) {

                    // double-precision packed register operand
                    INS_InsertCall(ins, IPOINT_BEFORE,
                            (AFUNPTR)updateReg64Packed,
                            IARG_REG_CONST_REFERENCE, reg,
                            IARG_PTR, &(info->min),
                            IARG_PTR, &(info->max),
                            IARG_END);

                } else {

                    // double-precision scalar register operand
                    INS_InsertCall(ins, IPOINT_BEFORE,
                            (AFUNPTR)updateReg64,
                            IARG_REG_CONST_REFERENCE, reg,
                            IARG_PTR, &(info->min),
                            IARG_PTR, &(info->max),
                            IARG_END);
                }

            } else if (REG_is_xmm(reg) && isFP32(ins)) {
                if (isPackedSSE(ins)) {

                    // single-precision packed register operand
                    INS_InsertCall(ins, IPOINT_BEFORE,
                            (AFUNPTR)updateReg32Packed,
                            IARG_REG_CONST_REFERENCE, reg,
                            IARG_PTR, &(info->min),
                            IARG_PTR, &(info->max),
                            IARG_END);

                } else {

                    // single-precision scalar register operand
                    INS_InsertCall(ins, IPOINT_BEFORE,
                            (AFUNPTR)updateReg32,
                            IARG_REG_CONST_REFERENCE, reg,
                            IARG_PTR, &(info->min),
                            IARG_PTR, &(info->max),
                            IARG_END);
                }
            }
        }
    }
}

/*
 * utility: print all information
 */
void dumpTable(ostream &out)
{
    out << "RANGE,MIN,MAX,ADDR,ROUTINE,SOURCE,LINENO,IMAGE,DISASSEMBLY" << endl;
    for (INS_INFO *ii = insList; ii != NULL; ii = ii->next) {
        if (ii->min != DMAX || ii->max != DMIN) {
            out.precision(4);
            out << std::scientific
                << (ii->max - ii->min)      // calculate range
                << "," << ii->min << "," << ii->max
                << ",0x" << std::hex << (unsigned long)ii->addr << std::dec
                << "," << ii->routine
                << "," << ii->source
                << "," << ii->lineno
                << "," << ii->image
                << ",\"" << ii->disas << "\""
                << endl;
        }
    }
}

/*
 * debugger: add hook that lets us dump the table contents
 */
static BOOL handleDebug(THREADID, CONTEXT*,
        const string &cmd, string *result, VOID*)
{
    *result = "";
    if (cmd == "dump") {
        stringstream ss("");
        dumpTable(ss);
        *result = ss.str();
        return TRUE;
    }
    return FALSE;
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

    // print all information into an output file
    string fn = KnobOutFile.Value().c_str();
    if (fn == DEFAULT_OUT_FN) {
        fn = appName + "-trange_pin-" + hostname + "-" + decstr(appPid) + ".log";
    }
    ofstream outFile(fn.c_str());
    dumpTable(outFile);
    outFile.close();

    // add metadata to main log file
    LOG("CRAFT: Analyzed " + appName + " [pid=" + decstr(appPid) +
            "] running on " + hostname + "\n");
    LOG("CRAFT: Handled " + decstr(totalInstructions) +
            " unique instruction(s)\n");
    LOG("CRAFT: Saved range tracking data to " + fn + "\n");
}

int main(int argc, char* argv[])
{
    // initialize Pin
    PIN_InitSymbols();  // we want symbol info if present
    if (PIN_Init(argc, argv)) {
        PIN_ERROR("This Pintool tracks the dynamic range of floating-point"
                  " instructions\n"
                + KNOB_BASE::StringKnobSummary() + "\n");
        return -1;
    }

    // uncomment to use AT&T syntax (to match the GNU debugger)
    //PIN_SetSyntaxATT();

    // register image callback
    IMG_AddInstrumentFunction(handleImage, 0);

    // register instruction callback
    INS_AddInstrumentFunction(handleInstruction, 0);

    // register debugging callback
    PIN_AddDebugInterpreter(handleDebug, 0);

    // register cleanup callback
    PIN_AddFiniFunction(handleCleanup, 0);

    // begin execution
    PIN_StartProgram();

    return 0;
}

