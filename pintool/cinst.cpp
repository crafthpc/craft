/*
 * Pin version of CRAFT cinst mode.
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
#define DEFAULT_OUT_FN  "{APP}-cinst_pin-{PID}.log"

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
    UINT64  count;              // number of executions observed

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
    LEVEL_BASE::OPCODE iclass = INS_Opcode(ins);
    INT32 icategory = INS_Category(ins);
    INT32 iextension = INS_Extension(ins);

    // check for floating-point instructions
    return (
        icategory == XED_CATEGORY_X87_ALU ||
        icategory == XED_CATEGORY_MMX ||
        icategory == XED_CATEGORY_SSE ||
        icategory == XED_CATEGORY_AVX ||
        icategory == XED_CATEGORY_AVX2 ||
        iextension == XED_EXTENSION_X87 ||
        iextension == XED_EXTENSION_MMX ||
        iextension == XED_EXTENSION_SSE ||
        iextension == XED_EXTENSION_SSE2 ||
        iextension == XED_EXTENSION_SSE3 ||
        iextension == XED_EXTENSION_SSE4 ||
        iextension == XED_EXTENSION_SSE4A ||
        iclass == XED_ICLASS_BTC            // used for floating-point negation
    );
}

/*
 * run time: increment a 64-bit unsigned counter
 */
VOID increment(UINT64 *counter)
{
    (*counter)++;
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
 * instrumentation time: insert calls to "increment"
 */
VOID handleInstruction(INS ins, VOID *)
{
    INT32 col;      // temporary source info variables
    INT32 line;
    string fn;

    if (shouldInstrument(ins)) {

        // extract address (unique identifier)
        ADDRINT addr = INS_Address(ins);

        // check to see if we've already added this instruction
        for (INS_INFO *ii = insList; ii != NULL; ii = ii->next) {
            if (ii->addr == addr) {

                // insert a call to increment and skip to next instruction
                INS_InsertCall(ins, IPOINT_BEFORE,
                        (AFUNPTR)increment,
                        IARG_PTR, &(ii->count), IARG_END);
                return;
            }
        }

        // initialize information in instruction table
        INS_INFO *info = new INS_INFO;
        info->addr = addr;
        info->count = 0;
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

        // insert a call to increment
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)increment,
                IARG_PTR, &(info->count), IARG_END);
    }
}

/*
 * utility: print all information
 */
void dumpTable(ostream &out)
{
    for (INS_INFO *ii = insList; ii != NULL; ii = ii->next) {
        out << std::dec << ii->count << ",0x"
            << std::hex << (unsigned long)ii->addr << std::dec
            << "," << ii->routine
            << "," << ii->source
            << "," << ii->lineno
            << "," << ii->image
            << ",\"" << ii->disas << "\""
            << endl;
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
        fn = appName + "-cinst_pin-" + hostname + "-" + decstr(appPid) + ".log";
    }
    ofstream outFile(fn.c_str());
    dumpTable(outFile);
    outFile.close();

    // add metadata to main log file
    LOG("CRAFT: Analyzed " + appName + " [pid=" + decstr(appPid) +
            "] running on " + hostname + "\n");
    LOG("CRAFT: Handled " + decstr(totalInstructions) +
            " unique instruction(s)\n");
    LOG("CRAFT: Saved execution count data to " + fn + "\n");
}

int main(int argc, char* argv[])
{
    // initialize Pin
    PIN_InitSymbols();  // we want symbol info if present
    if (PIN_Init(argc, argv)) {
        PIN_ERROR("This Pintool counts executions of floating-point"
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

