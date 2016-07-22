/*
 * Pin version of CRAFT dcancel mode.
 *
 * Mike Lam, LLNL, Summer 2016
 */

#include <iostream>
#include <fstream>
#include <sstream>

#include <string.h>
#include <math.h>
#include <unistd.h>

#include "pin.H"
#include "pin_isa.H"

extern "C" {
#include "xed-interface.h"
}

#define MAX_STR_LEN     64
#define DEFAULT_OUT_FN  "{APP}-dcancel_pin-{PID}.log"

using namespace std;

/*
 * command-line options
 */
KNOB<string> KnobOutFile(KNOB_MODE_WRITEONCE, "pintool",
        "o", DEFAULT_OUT_FN, "output file name");
KNOB<UINT64> KnobThreshold(KNOB_MODE_WRITEONCE, "pintool",
        "t", "10", "cancellation threshold for reporting");

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
    UINT64 maxBits;             // max bits canceled in a single operation
    UINT64 totalBits;           // total bits canceled
    UINT64 count;               // execution count

    // debug and semantic info
    bool    isAdd;              // true=addition, false=subtraction
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
 * only report cancellations of more than "threshold" bits
 */
static UINT64 threshold = 0;

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
 * criteria that make an instruction relevant to cancellation analysis
 */
bool shouldInstrument(INS ins)
{
    // check for floating-point additions and subtractions
    LEVEL_BASE::OPCODE iclass = INS_Opcode(ins);
    switch (iclass) {
        case XED_ICLASS_ADDSS: case XED_ICLASS_ADDPS:
        case XED_ICLASS_ADDSD: case XED_ICLASS_ADDPD:
        case XED_ICLASS_SUBSS: case XED_ICLASS_SUBPS:
        case XED_ICLASS_SUBSD: case XED_ICLASS_SUBPD:
            return true;
    }
    return false;
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
            return true;
    }
    return false;
}

/*
 * criteria to check whether an instruction is an addition (as opposed to a
 * subtraction)
 */
bool isAddition(INS ins)
{
    LEVEL_BASE::OPCODE iclass = INS_Opcode(ins);
    switch (iclass) {
        case XED_ICLASS_ADDSS: case XED_ICLASS_ADDSD:
        case XED_ICLASS_ADDPS: case XED_ICLASS_ADDPD:
            return true;
    }
    return false;
}

/*
 * run time: update min/max trackers for various kinds of operands
 *
 * TODO: handle packed instructions
 */

VOID update(InsInfo *info, UINT64 bitsCanceled)
{
    info->count++;
    if (bitsCanceled > threshold) {
        if (bitsCanceled > info->maxBits) {
            info->maxBits = bitsCanceled;
        }
        info->totalBits += bitsCanceled;
    }

}

VOID update64(InsInfo *info, double *op1, double *op2)
{
    double result;
    int exp1, exp2, expr;
    if (info->isAdd) {
        result = *op1 + *op2;
    } else {
        result = *op1 - *op2;
    }
    frexp(*op1, &exp1);
    frexp(*op2, &exp2);
    frexp(result, &expr);
    int bitsCanceled;

    if (result == 0.0 && *op1 != 0.0 && *op2 != 0.0) {
        bitsCanceled = 53;      // complete cancellation
    } else {
        bitsCanceled = (UINT64)(exp1 > exp2 ? exp1 - expr : exp2 - expr);
        if (bitsCanceled < 0) {
            bitsCanceled = 0;
        }
    }

    /*
     *cout << "0x" << std::hex << info->addr << std::dec
     *     << " op1=" << *op1 << " (exp=" << exp1 << ")"
     *     << " op2=" << *op2 << " (exp=" << exp2 << ")"
     *     << " res=" << result << " (exp=" << expr << ")"
     *     << " bitsCanceled=" << bitsCanceled
     *     << " \"" << info->disas << "\"" << endl;
     */

    update(info, (UINT64)bitsCanceled);
}

VOID updateMemReg64(InsInfo *info, double *op1, PIN_REGISTER *op2)
{
    update64(info, op1, (double*)&(op2->dbl[0]));
}

VOID updateRegReg64(InsInfo *info, PIN_REGISTER *op1, PIN_REGISTER *op2)
{
    update64(info, (double*)&(op1->dbl[0]),
                   (double*)&(op2->dbl[0]));
}

VOID update32(InsInfo *info, float *op1, float *op2)
{
    float result;
    int exp1, exp2, expr;
    if (info->isAdd) {
        result = *op1 + *op2;
    } else {
        result = *op1 - *op2;
    }
    frexp(*op1, &exp1);
    frexp(*op2, &exp2);
    frexp(result, &expr);
    int bitsCanceled;

    if (result == 0.0 && *op1 != 0.0 && *op2 != 0.0) {
        bitsCanceled = 24;      // complete cancellation
    } else {
        bitsCanceled = (UINT64)(exp1 > exp2 ? exp1 - expr : exp2 - expr);
        if (bitsCanceled < 0) {
            bitsCanceled = 0;
        }
    }

    /*
     *cout << "0x" << std::hex << info->addr << std::dec
     *     << " op1=" << *op1 << " (exp=" << exp1 << ")"
     *     << " op2=" << *op2 << " (exp=" << exp2 << ")"
     *     << " res=" << result << " (exp=" << expr << ")"
     *     << " bitsCanceled=" << bitsCanceled
     *     << " \"" << info->disas << "\"" << endl;
     */

    update(info, (UINT64)bitsCanceled);
}

VOID updateMemReg32(InsInfo *info, float *op1, PIN_REGISTER *op2)
{
    update32(info, op1, (float*)&(op2->flt[0]));
}

VOID updateRegReg32(InsInfo *info, PIN_REGISTER *op1, PIN_REGISTER *op2)
{
    update32(info, (float*)&(op1->flt[0]),
                   (float*)&(op2->flt[0]));
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
            info->maxBits = 0;
            info->totalBits = 0;
            info->count = 0;
            info->isAdd = isAddition(ins);
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
            if (isFP64(ins)) {

                // double-precision memory and register operands
                INS_InsertCall(ins, IPOINT_BEFORE,
                        (AFUNPTR)updateMemReg64,
                        IARG_PTR, info,
                        IARG_MEMORYREAD_EA,
                        IARG_REG_CONST_REFERENCE, INS_RegR(ins, 0),
                        IARG_END);

            } else {

                // single-precision memory and register operands
                INS_InsertCall(ins, IPOINT_BEFORE,
                        (AFUNPTR)updateMemReg32,
                        IARG_PTR, info,
                        IARG_MEMORYREAD_EA,
                        IARG_REG_CONST_REFERENCE, INS_RegR(ins, 0),
                        IARG_END);

            }
        } else {
            if (isFP64(ins)) {

                // double-precision register operands
                INS_InsertCall(ins, IPOINT_BEFORE,
                        (AFUNPTR)updateRegReg64,
                        IARG_PTR, info,
                        IARG_REG_CONST_REFERENCE, INS_RegR(ins, 0),
                        IARG_REG_CONST_REFERENCE, INS_RegR(ins, 1),
                        IARG_END);

            } else {
                // TODO: single-precision register operands
                INS_InsertCall(ins, IPOINT_BEFORE,
                        (AFUNPTR)updateRegReg32,
                        IARG_PTR, info,
                        IARG_REG_CONST_REFERENCE, INS_RegR(ins, 0),
                        IARG_REG_CONST_REFERENCE, INS_RegR(ins, 1),
                        IARG_END);

            }
        }
    }
}

/*
 * utility: print all information
 */
void dumpTable(ostream &out)
{
    out << "MAXCANCEL,AVGCANCEL,COUNT,ADDR,ROUTINE,SOURCE,LINENO,IMAGE,DISASSEMBLY" << endl;
    for (INS_INFO *ii = insList; ii != NULL; ii = ii->next) {
        if (ii->count > 0) {
            out << ii->maxBits
                << "," << (double)ii->totalBits / (double)ii->count    // average
                << "," << ii->count
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
        fn = appName + "-dcancel_pin-" + hostname + "-" + decstr(appPid) + ".log";
    }
    ofstream outFile(fn.c_str());
    dumpTable(outFile);
    outFile.close();

    // add metadata to main log file
    LOG("CRAFT: Analyzed " + appName + " [pid=" + decstr(appPid) +
            "] running on " + hostname + "\n");
    LOG("CRAFT: Handled " + decstr(totalInstructions) +
            " unique instruction(s)\n");
    LOG("CRAFT: Saved cancellation detection data (threshold=" +
            decstr(threshold) + ") to " + fn + "\n");
}

int main(int argc, char* argv[])
{
    // initialize Pin
    PIN_InitSymbols();  // we want symbol info if present
    if (PIN_Init(argc, argv)) {
        PIN_ERROR("This Pintool detects cancellations in floating-point"
                  " instructions\n"
                + KNOB_BASE::StringKnobSummary() + "\n");
        return -1;
    }

    // initialize threshold value
    threshold = KnobThreshold.Value();
    if (threshold < 0 || threshold > 53) {
        PIN_ERROR("Threshold value must be >= 0 and <= 53\n"
                + KNOB_BASE::StringKnobSummary() + "\n");
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

