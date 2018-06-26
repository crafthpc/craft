/*
 * Pintool that extracts a floating-point data trace
 *
 * Mike Lam, LLNL, Summer 2016
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
//#include "pin_isa.H"

extern "C" {
#include "xed-interface.h"
}


// DATA TYPES //

#ifndef PIN_H
#include <stdint.h>
typedef void*    ADDRINT;
typedef uint32_t UINT32;
typedef uint64_t UINT64;
#endif

typedef struct {
    ADDRINT ins;
    UINT32  code;
    UINT64  in;
    UINT64  in2;
    UINT64  out;
    UINT64  data;
} FP_EVENT;

// TODO: change to unordered_map
map<ADDRINT, string> disas;


// CODES //

#define FP_NOP 0
#define FP_MOV 1
#define FP_ADD 2
#define FP_SUB 3
#define FP_MUL 4
#define FP_DIV 5


// OPERANDS //

#define OP_NONE     0x000
#define OP_MEM      0x001       // unresolved memory effective address

// TODO: general registers

#define OP_XMM0     0x100       // packed:  [ 0-31 ] -> offset +0x00
#define OP_XMM1     0x101       //          [32-63 ] -> offset +0x10
#define OP_XMM2     0x102       //          [64-95 ] -> offset +0x20
#define OP_XMM3     0x103       //          [96-127] -> offset +0x30
#define OP_XMM4     0x104
#define OP_XMM5     0x105
#define OP_XMM6     0x106
#define OP_XMM7     0x107
#define OP_XMM8     0x108
#define OP_XMM9     0x109
#define OP_XMM10    0x10A
#define OP_XMM11    0x10B
#define OP_XMM12    0x10C
#define OP_XMM13    0x10D
#define OP_XMM14    0x10E
#define OP_XMM15    0x10F

#define OP_LAST     0x500       // memory:  higher than 0x500


// OUTPUT HELPERS //

/*
 * pretty print an operand
 */
void prettyPrintOp(std::ostream& os, UINT64 op)
{
    if (op == OP_NONE) {
        os << "(none)";
    } else if (op == OP_MEM) {
        os << "(mem)";
    } else if (op >= OP_XMM0 && op <= OP_XMM15+0x30) {
        UINT64 num = op & 0xF;
        os << "xmm" << num;
        if (op & 0x10) {
            os << "[1]";
        } else if (op & 0x20) {
            os << "[2]";
        } else if (op & 0x30) {
            os << "[3]";
        }
    } else {
        os << "0x" << std::hex << op;
    }
}

/*
 * pretty print an event
 */
std::ostream& operator<<(std::ostream& os, const FP_EVENT &evt)
{
    os << "0x" << std::hex << evt.ins;
    os << " " << disas[evt.ins];
    /*
    switch (evt.code) {
        case FP_MOV:    os << " mov ";    break;
        case FP_ADD:    os << " add ";    break;
        case FP_SUB:    os << " sub ";    break;
        case FP_MUL:    os << " mul ";    break;
        case FP_DIV:    os << " div ";    break;
        default:        os << " nop ";    break;
    }
    prettyPrintOp(os, evt.in);
    if (evt.in2 != OP_NONE) {
        os << ", ";
        prettyPrintOp(os, evt.in2);
    }
    os << " -> ";
    prettyPrintOp(os, evt.out);
    */
    if (evt.in > OP_LAST || evt.in2 > OP_LAST) {
        os << " [data sgl=" << *(float*)(&evt.data)
           << " dbl=" << *(double*)(&evt.data) << "]";
    }
    return os;
}


// PINTOOL //

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
 * dump an event to the output file
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
 * convert XED opcodes to instruction operand type for analysis functions
 */
UINT32 encodeOpcode(INS ins)
{
    switch (INS_Opcode(ins)) {
        case XED_ICLASS_MOVSD:  return FP_MOV;
        case XED_ICLASS_ADDSD:  return FP_ADD;
        case XED_ICLASS_SUBSD:  return FP_SUB;
        case XED_ICLASS_MULSD:  return FP_MUL;
        case XED_ICLASS_DIVSD:  return FP_DIV;
        case XED_ICLASS_ADDPD:  return FP_ADD;
        case XED_ICLASS_SUBPD:  return FP_SUB;
        case XED_ICLASS_MULPD:  return FP_MUL;
        case XED_ICLASS_DIVPD:  return FP_DIV;
        default:                return FP_NOP;
    }
}

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
 * is the instruction part of the SSE/AVX instruction set?
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
 * extract and encode the idx-th XMM register read
 */
UINT32 encodeXMMRegR(INS ins, int idx)
{
    for (UINT32 i = 0; i < INS_MaxNumRRegs(ins); i++) {
        if (REG_is_xmm(INS_RegR(ins,i))) {
            if (idx == 0) {
                return INS_RegR(ins,i) - REG_XMM0 + OP_XMM0;
            } else {
                idx--;
            }
        }
    }
    cerr << INS_Disassemble(ins) << endl;
    assert(!"no xmm register");
}

/*
 * extract and encode the idx-th XMM register written
 */
UINT32 encodeXMMRegW(INS ins, int idx)
{
    for (UINT32 i = 0; i < INS_MaxNumWRegs(ins); i++) {
        if (REG_is_xmm(INS_RegW(ins,i))) {
            if (idx == 0) {
                return INS_RegW(ins,i) - REG_XMM0 + OP_XMM0;
            } else {
                idx--;
            }
        }
    }
    cerr << INS_Disassemble(ins) << endl;
    assert(!"no xmm register");
}

/*
 * runtime: add an FP_EVENT
 */
VOID fillBuffer(ADDRINT addr, UINT32 code, UINT64 in, UINT64 in2, UINT64 out)
{
    FP_EVENT evt;
    evt.ins  = addr;
    evt.code = code;
    evt.in   = in;
    evt.in2  = in2;
    evt.out  = out;
    evt.data = 0;
    dumpEvent(&evt);
}

/*
 * runtime: add an FP_EVENT that reads from memory
 */
VOID fillBufferMem64(ADDRINT addr, UINT32 code, UINT64 in, UINT64 in2, UINT64 out, UINT64 mem)
{
    FP_EVENT evt;
    evt.ins  = addr;
    evt.code = code;
    evt.in   = (in  == OP_MEM ? mem : in);
    evt.in2  = (in2 == OP_MEM ? mem : in2);
    evt.out  = (out == OP_MEM ? mem : out);
    evt.data = *(UINT64*)mem;
    dumpEvent(&evt);
}

/*
 * instrumentation: insert a call to an event dump function
 */
void insertBufferCall(INS ins, UINT32 code, UINT64 in, UINT64 in2, UINT64 out)
{
    //cout << "insertBufferCall " << hexstr(INS_Address(ins))
         //<< ": " << decstr(code) << " " << hexstr(in) << ", " << hexstr(in2)
         //<< " -> " << hexstr(out) << endl;
    if (in != OP_MEM && in2 != OP_MEM && out != OP_MEM) {
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)fillBuffer,
                IARG_INST_PTR,     IARG_UINT32, code,
                IARG_ADDRINT, in,  IARG_ADDRINT, in2,
                IARG_ADDRINT, out, IARG_ADDRINT, 0,       IARG_END);
        //INS_InsertFillBuffer(ins, IPOINT_BEFORE, mainBuf,
                //IARG_INST_PTR,     offsetof(FP_EVENT, ins),
                //IARG_UINT32, code, offsetof(FP_EVENT, code),
                //IARG_ADDRINT, in,   offsetof(FP_EVENT, in),
                //IARG_ADDRINT, in2,  offsetof(FP_EVENT, in2),
                //IARG_ADDRINT, out,  offsetof(FP_EVENT, out),
                //IARG_END);
    } else if ((in == OP_MEM && in2 != OP_MEM && out != OP_MEM) ||
               (in != OP_MEM && in2 == OP_MEM && out != OP_MEM)) {
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)fillBufferMem64,
                IARG_INST_PTR,     IARG_UINT32, code,
                IARG_ADDRINT, in,  IARG_ADDRINT, in2,
                IARG_ADDRINT, out, IARG_MEMORYREAD_EA,   IARG_END);
        //INS_InsertFillBuffer(ins, IPOINT_BEFORE, mainBuf,
                //IARG_INST_PTR,     offsetof(FP_EVENT, ins),
                //IARG_UINT32, code, offsetof(FP_EVENT, code),
                //IARG_MEMORYREAD_EA,offsetof(FP_EVENT, in),
                //IARG_ADDRINT, in2,  offsetof(FP_EVENT, in2),
                //IARG_ADDRINT, out,  offsetof(FP_EVENT, out),
                //IARG_END);
    //} else if (in != OP_MEM && in2 == OP_MEM && out != OP_MEM) {
        //INS_InsertFillBuffer(ins, IPOINT_BEFORE, mainBuf,
                //IARG_INST_PTR,     offsetof(FP_EVENT, ins),
                //IARG_UINT32, code, offsetof(FP_EVENT, code),
                //IARG_ADDRINT, in,   offsetof(FP_EVENT, in),
                //IARG_MEMORYREAD_EA,offsetof(FP_EVENT, in2),
                //IARG_ADDRINT, out,  offsetof(FP_EVENT, out),
                //IARG_END);
    } else if (in != OP_MEM && in2 != OP_MEM && out == OP_MEM) {
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)fillBuffer,
                IARG_INST_PTR,     IARG_UINT32, code,
                IARG_ADDRINT, in,  IARG_ADDRINT, in2,
                IARG_MEMORYWRITE_EA,   IARG_END);
        //INS_InsertFillBuffer(ins, IPOINT_BEFORE, mainBuf,
                //IARG_INST_PTR,     offsetof(FP_EVENT, ins),
                //IARG_UINT32, code, offsetof(FP_EVENT, code),
                //IARG_ADDRINT, in,   offsetof(FP_EVENT, in),
                //IARG_ADDRINT, in2,  offsetof(FP_EVENT, in2),
                //IARG_MEMORYWRITE_EA,offsetof(FP_EVENT, out),
                //IARG_END);
    } else {
        cerr << INS_Disassemble(ins) << endl;
        assert(!"more than two mem operands");
    }
}

/*
 * instrumentation: add event for a move instruction
 */
void handleMove(INS ins)
{
    if (INS_IsMemoryRead(ins)) {

        insertBufferCall(ins, FP_MOV, OP_MEM, OP_NONE, encodeXMMRegW(ins,0));
    } else if (INS_IsMemoryWrite(ins)) {
        insertBufferCall(ins, FP_MOV, encodeXMMRegR(ins,0), OP_NONE, OP_MEM);
#if 0
    } else if (INS_OperandCount(ins) == 2 &&
            REG_is_xmm(INS_RegW(ins,0)) && !REG_is_xmm(INS_RegR(ins,0))) {
        // special case for movq [gpr] -> [xmm]
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)shadowMovScalarGPR64ToReg64,
                IARG_REG_CONST_REFERENCE, INS_RegR(ins,0),
                IARG_UINT32, encodeXMMRegW(ins),
                IARG_END);
    } else if (INS_OperandCount(ins) == 2 &&
            REG_is_xmm(INS_RegW(ins,0)) && REG_is_xmm(INS_RegR(ins,0))) {
        //cout << "adding call to reg->reg mov:"
            //<< " src1=xmm" << INS_RegR(ins,0)-REG_XMM0
            //<< " src2=xmm" << INS_RegR(ins,1)-REG_XMM0
            //<< " dest=xmm" << INS_RegW(ins,0)-REG_XMM0
            //<< endl;
        UINT32 sreg = encodeXMMRegR(ins);
        UINT32 dreg = encodeXMMRegW(ins);
        INS_InsertCall(ins, IPOINT_BEFORE,
                (AFUNPTR)regFunc,
                IARG_UINT32, sreg,
                IARG_UINT32, dreg,
                IARG_END);
    } else {
        cout << "ignoring " << INS_Disassemble(ins) << endl;
        return;
#endif
    }
    totalInstructions++;
}

/*
 * instrumentation: add event for a binary operation instruction
 */
void handleBinOp(INS ins)
{
    if (INS_IsMemoryRead(ins)) {
        insertBufferCall(ins,
                encodeOpcode(ins),      // code
                encodeXMMRegR(ins,0),   // in
                IARG_MEMORYREAD_EA,     // in2
                encodeXMMRegW(ins,0));  // out
    } else if (INS_IsMemoryWrite(ins)) {
        insertBufferCall(ins,
                encodeOpcode(ins),      // code
                encodeXMMRegR(ins,0),   // in
                encodeXMMRegR(ins,1),   // in2
                IARG_MEMORYWRITE_EA);   // out
    } else {
        insertBufferCall(ins,
                encodeOpcode(ins),      // code
                encodeXMMRegR(ins,0),   // in
                encodeXMMRegR(ins,1),   // in2
                encodeXMMRegW(ins,0));  // out
    }
    totalInstructions++;
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

    disas[INS_Address(ins)] = INS_Disassemble(ins);

    //LOG("Handling 0x" + hexstr(INS_Address(ins)) + ": " + INS_Disassemble(ins));

    // check non-floating-point instructions for movement to/from a
    // floating-point memory location
    if (!isSSE(ins)) {
        //LOG("non-SSE instruction");
        if (INS_IsMemoryWrite(ins) && INS_MemoryWriteSize(ins) == 8) {
            //INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)shadowMovToMem64,
                    //IARG_MEMORYWRITE_EA, IARG_END);
            //LOG("handled b/c it might move FP data\n");
        //} else {
            //LOG("skipped\n");
        }
        return;
    }

    // otherwise, what we do depends on the opcode
    //LOG("attempting to instrument\n");
    LEVEL_BASE::OPCODE iclass = INS_Opcode(ins);
    switch (iclass) {

    // pure data movement
    case XED_ICLASS_MOVQ:
    case XED_ICLASS_MOVSD_XMM:
    case XED_ICLASS_MOVDQA:
    case XED_ICLASS_MOVAPS:
    case XED_ICLASS_MOVAPD:
        handleMove(ins); break;

    // binary arithmetic
    case XED_ICLASS_ADDSD: handleBinOp(ins); break;
    case XED_ICLASS_SUBSD: handleBinOp(ins); break;
    case XED_ICLASS_MULSD: handleBinOp(ins); break;
    case XED_ICLASS_DIVSD: handleBinOp(ins); break;
    case XED_ICLASS_ADDPD: handleBinOp(ins); break;
    case XED_ICLASS_SUBPD: handleBinOp(ins); break;
    case XED_ICLASS_MULPD: handleBinOp(ins); break;
    case XED_ICLASS_DIVPD: handleBinOp(ins); break;

    // Stupid Floating-Point Tricks (TM)
    /*
    case XED_ICLASS_PXOR:
    case XED_ICLASS_XORPD:
        if (!INS_IsMemoryRead(ins) && !INS_IsMemoryWrite(ins) &&
             REG_is_xmm(INS_RegW(ins,0)) && REG_is_xmm(INS_RegR(ins,1))) {
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)shadowXORPD,
                    IARG_UINT32, INS_RegW(ins,0) - REG_XMM0,
                    IARG_UINT32, INS_RegR(ins,1) - REG_XMM0,
                    IARG_REG_CONST_REFERENCE, INS_RegR(ins,0),
                    IARG_REG_CONST_REFERENCE, INS_RegR(ins,1),
                    IARG_END);
        } else {
            LOG("WARNING - unhandled instruction: " + INS_Disassemble(ins)
                + " (0x" + hexstr(INS_Address(ins)) + ")\n");
        }
        totalInstructions++;
        break;
    */

    // we can safely ignore these (they have no effect on shadow values)
    case XED_ICLASS_UCOMISD:        // unordered compare scalar double
    case XED_ICLASS_PCMPEQB:        // compare packed data for equal
    case XED_ICLASS_STMXCSR:        // save MXCSR register state
    case XED_ICLASS_LDMXCSR:        // load MXCSR register state
    case XED_ICLASS_FXSAVE:         // save x87/MMX/SSE state
    case XED_ICLASS_FXRSTOR:        // load x87/MMX/SSE state
    case XED_ICLASS_CVTSD2SI:       // convert to signed int
    case XED_ICLASS_CVTTSD2SI:      // convert to signed int (truncated)
        break;

    // we can ignore conversions to floating-point because the shadow values
    // will get initialized at the next read anyway
    case XED_ICLASS_CVTSI2SD:
        break;

    // flag anything we didn't know how to handle
    default:
        LOG("WARNING - unhandled instruction: " + INS_Disassemble(ins)
            + " (0x" + hexstr(INS_Address(ins)) + ")\n");
        break;
    }
}

/*
 *VOID* dumpBuffer(BUFFER_ID id, THREADID tid, const CONTEXT *ctx, VOID *buf, UINT64 numElements, VOID *v)
 *{
 *    FP_EVENT *elem = (FP_EVENT*)buf;
 *    for (UINT64 i = 0; i < numElements; i++) {
 *        cout << *elem;
 *        elem++;
 *    }
 *    return buf;
 *}
 */

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
        PIN_ERROR("This Pintool extracts a floating-point trace\n"
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
    //mainBuf = PIN_DefineTraceBuffer(sizeof(FP_EVENT), 1024, dumpBuffer, 0);

    // register cleanup callback
    PIN_AddFiniFunction(handleCleanup, 0);

    // begin execution
    PIN_StartProgram();

    return 0;
}

