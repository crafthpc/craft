// standard C libs
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

// STL classes
#include <string>
#include <vector>

// XED
extern "C" {
#include "xed-interface.h"
}

// DyninstAPI
#include "BPatch.h"
#include "BPatch_basicBlock.h"
#include "BPatch_flowGraph.h"
#include "BPatch_function.h"
#include "BPatch_point.h"
#include "BPatch_snippet.h"

// PatchAPI
#include "PatchMgr.h"
#include "Point.h"
#include "Snippet.h"

using namespace std;
using namespace Dyninst;
using namespace Dyninst::InstructionAPI;
using namespace Dyninst::PatchAPI;

#define BUFFER_STRING_LEN 1024

#define COUNTER_TYPE_STR "unsigned long"
typedef unsigned long counter_t;

// main Dyninst driver structure
BPatch *bpatch = NULL;

// main XED decoder state
xed_state_t dstate;

// top-level Dyninst mutatee structures
BPatch_addressSpace *mainApp = NULL;
BPatch_image        *mainImg = NULL;
PatchMgr::Ptr        mainMgr;

BPatch_function                *printfFunc;
BPatch_Vector<BPatch_snippet*>  initCode;
BPatch_Vector<BPatch_snippet*>  finiCode;
BPatch_type                    *counterType;

// analysis shared library
BPatch_object *libObj  = NULL;

// global parameters
char *binary = NULL;
bool instShared = false;
bool libmCounts = false;
bool libcCounts = false;

BPatch_module* getInitFiniModule() {

    // detects the module that should be instrumented with init/cleanup
    BPatch_Vector<BPatch_module *> *modules = mainImg->getModules();
    BPatch_Vector<BPatch_module *>::iterator m;
    BPatch_Vector<BPatch_function *> initFiniFuncs;
    bool hasInit, hasFini, isShared;
    for (m = modules->begin(); m != modules->end(); m++) {
        hasInit = hasFini = isShared = false;
        if ((*m)->isSharedLib()) {
            isShared = true;
        }
        initFiniFuncs.clear();
        (*m)->findFunction("_init", initFiniFuncs, false);
        if (initFiniFuncs.size() > 0) {
            hasInit = true;
        }
        initFiniFuncs.clear();
        (*m)->findFunction("_fini", initFiniFuncs, false);
        if (initFiniFuncs.size() > 0) {
            hasFini = true;
        }
        if (!isShared && hasInit && hasFini) {
            return *m;
        }
    }
    return NULL;
}

bool hasMutateeFunction(const char *name) {
    BPatch_Vector<BPatch_function *> funcs;
    mainImg->findFunction(name, funcs, true, true, true);
    return funcs.size() > 0;
}

BPatch_function* getMutateeFunction(const char *name) {

    // assumes there is a single function in the mutatee address space
    // with the given name; finds and returns it
    BPatch_Vector<BPatch_function *> funcs;
    mainImg->findFunction(name, funcs, true, true, true);
    assert(funcs.size() == 1);
    return funcs.at(0);
}

BPatch_snippet* buildInitSnippet(BPatch_variableExpr *var) {
    return new BPatch_arithExpr(BPatch_assign, *var, BPatch_constExpr((counter_t)0));
}


BPatch_snippet* buildIncrementer(BPatch_variableExpr *var) {

    // null instrumentation (for testing)
    //BPatch_snippet *nullExpr = new BPatch_nullExpr();
    //return PatchAPI::convert(nullExpr);

    // instruction count instrumentation
    BPatch_snippet *valExpr = new BPatch_arithExpr(BPatch_plus,
            *var, BPatch_constExpr((counter_t)1));
    BPatch_snippet *incExpr = new BPatch_arithExpr(BPatch_assign,
            *var, *valExpr);
    return incExpr;
}

bool fpinstFilterFunc(Instruction::Ptr iptr) {
    unsigned char bytes[64];
    size_t nbytes = iptr->size();
    memcpy(bytes, iptr->ptr(), nbytes);
    char buffer[1048];
    bool add = false;
    xed_decoded_inst_t xedd;
    xed_error_enum_t xed_error;
    xed_iclass_enum_t iclass;
    xed_category_enum_t icategory;
    xed_extension_enum_t iextension;
    //xed_isa_set_enum_t iisaset;
    xed_decoded_inst_zero_set_mode(&xedd, &dstate);
    xed_decoded_inst_set_input_chip(&xedd, XED_CHIP_INVALID);
    xed_error = xed_decode(&xedd, (const xed_uint8_t*) bytes, nbytes);
    if (xed_error == XED_ERROR_NONE) { 
        xed_format_intel(&xedd, buffer, 1048, (xed_uint64_t)(long)bytes);
        iclass = xed_decoded_inst_get_iclass(&xedd);
        icategory = xed_decoded_inst_get_category(&xedd);
        iextension = xed_decoded_inst_get_extension(&xedd);
        //iisaset = xed_decoded_inst_get_isa_set(&xedd);
        if (icategory == XED_CATEGORY_SSE ||
            icategory == XED_CATEGORY_X87_ALU ||
            iextension == XED_EXTENSION_MMX ||
            iextension == XED_EXTENSION_SSE ||
            iextension == XED_EXTENSION_SSE2 ||
            iextension == XED_EXTENSION_SSE3 ||
            iextension == XED_EXTENSION_SSE4 ||
            iextension == XED_EXTENSION_SSE4A ||
            iextension == XED_EXTENSION_X87 ||
            iclass == XED_ICLASS_BTC) {
            add = true;
        }
    }
    return add;
}

BPatch_snippet* buildFiniSnippet(void *addr, Instruction::Ptr iptr, string func, BPatch_variableExpr *var) {

    // create printf call
    stringstream disas("");
    disas << "\"" << iptr->format((Address)addr) << "\"";
    disas << " {";
    bool comma = false;
    if (fpinstFilterFunc(iptr)) {
        if (comma) disas << ","; else comma = true;
        disas << "fp";
    }
    if (iptr->readsMemory()) {
        if (comma) disas << ","; else comma = true;
        disas << "mr";
    }
    if (iptr->writesMemory()) {
        if (comma) disas << ","; else comma = true;
        disas << "mw";
    }
    /*
     *std::set<RegisterAST::Ptr> rregs;
     *iptr->getWriteSet(rregs);
     *if (rregs.size() > 0) {
     *    if (comma) disas << ","; else comma = true;
     *    disas << "rr";
     *}
     *std::set<RegisterAST::Ptr> wregs;
     *iptr->getWriteSet(wregs);
     *if (wregs.size() > 0) {
     *    if (comma) disas << ","; else comma = true;
     *    disas << "rw";
     *}
     */
    disas << "}";
    BPatch_Vector<BPatch_snippet*> printfArgs;
    printfArgs.push_back(new BPatch_constExpr("%16lx: %-15lu %-20s %s\n"));
    printfArgs.push_back(new BPatch_constExpr((unsigned long)addr));
    printfArgs.push_back(var);
    printfArgs.push_back(new BPatch_constExpr(func.c_str()));
    printfArgs.push_back(new BPatch_constExpr(disas.str().c_str()));

    return new BPatch_funcCallExpr(*printfFunc, printfArgs);
}

BPatch_snippet* buildFiniSnippet(string func, BPatch_variableExpr *var) {

    // create printf call
    BPatch_Vector<BPatch_snippet*> printfArgs;
    printfArgs.push_back(new BPatch_constExpr("%20s: %-15d\n"));
    printfArgs.push_back(new BPatch_constExpr(func.c_str()));
    printfArgs.push_back(var);

    return new BPatch_funcCallExpr(*printfFunc, printfArgs);
}

BPatch_snippet* buildDebugPrint(string text) {

    // create printf call
    BPatch_Vector<BPatch_snippet*> printfArgs;
    printfArgs.push_back(new BPatch_constExpr(text.c_str()));
    return new BPatch_funcCallExpr(*printfFunc, printfArgs);
}

void handleLibFunc(string name)
{
    if (!hasMutateeFunction(name.c_str())) return;

    BPatch_function *func = getMutateeFunction(name.c_str());

    // allocate counter
    BPatch_variableExpr *counter = mainApp->malloc(*counterType);

    // build and insert initialization
    initCode.push_back(buildInitSnippet(counter));

    // insert instrumentation
    BPatch_Vector<BPatch_point*> *entryPoints = func->findPoint(BPatch_entry);
    mainApp->insertSnippet(*buildIncrementer(counter), *entryPoints);

    // build and insert reporting
    finiCode.push_back(buildFiniSnippet(name, counter));
}


void handleInstruction(void *addr, Instruction::Ptr iptr, BPatch_basicBlock *block, BPatch_function *func)
{
    // DEBUGGING CODE
    //static bool halt = false;
    //if (halt) return;
    //else halt = true;
    //if ((unsigned long)addr < (unsigned long)0x400564 ||
        //(unsigned long)addr > (unsigned long)0x4005db) return;

    // print instruction info
    //printf("  instruction at %lx: %s\n",
            //(unsigned long)addr, iptr->format((Address)addr).c_str());

    // allocate counter
    BPatch_variableExpr *counter = mainApp->malloc(*counterType);
    //printf("    counter at %lx\n", (unsigned long)counter->getBaseAddr());

    // build and insert initialization
    initCode.push_back(buildInitSnippet(counter));

    // build and insert instrumentation (DyninstAPI version)
    BPatch_point *blockEntry = block->findExitPoint();
    mainApp->insertSnippet(*buildIncrementer(counter), *blockEntry, BPatch_lastSnippet);

    // build and insert instrumentation (PatchAPI version)
    //Point *prePoint  = mainMgr->findPoint(
                        //Location::InstructionInstance(func, block, (Address)addr),
                        //Point::PreInsn, true);
    //prePoint->pushBack(PatchAPI::convert(buildIncrementer(counter)));

    // build and insert reporting
    finiCode.push_back(buildFiniSnippet(addr, iptr, func->getName(), counter));
}

void handleBasicBlock(BPatch_basicBlock *block, BPatch_function *func)
{
    Instruction::Ptr iptr;
    void *addr;

    // get all floating-point instructions
    PatchBlock::Insns insns;
    PatchAPI::convert(block)->getInsns(insns);

    // handle each point separately
    PatchBlock::Insns::iterator j;
    for (j = insns.begin(); j != insns.end(); j++) {

        addr = (void*)((*j).first);
        iptr = (*j).second;
        handleInstruction(addr, iptr, block, func);
    }
}

void handleFunction(BPatch_function *function, const char *name)
{
    //printf("function %s:\n", name);

    // handle all basic blocks
    std::set<BPatch_basicBlock*> blocks;
    std::set<BPatch_basicBlock*>::iterator b;
    BPatch_flowGraph *cfg = function->getCFG();
    cfg->getAllBasicBlocks(blocks);
    for (b = blocks.begin(); b != blocks.end(); b++) {
        handleBasicBlock(*b, function);
    }

    //printf("\n");
}

void handleModule(BPatch_module *mod, const char *name)
{
	char funcname[BUFFER_STRING_LEN];

	// get list of all functions
	std::vector<BPatch_function *>* functions;
	functions = mod->getProcedures();

	// for each function ...
	for (unsigned i = 0; i < functions->size(); i++) {
		BPatch_function *function = functions->at(i);
		function->getName(funcname, BUFFER_STRING_LEN);

        // CRITERIA FOR INSTRUMENTATION:
        // don't handle:
        //   - memset() or call_gmon_start() or frame_dummy()
        //   - functions that begin with an underscore
		if ( (strcmp(funcname,"memset")!=0)
                && (strcmp(funcname,"call_gmon_start")!=0)
                && (strcmp(funcname,"frame_dummy")!=0)
                && funcname[0] != '_') {

            handleFunction(function, funcname);
		}
	}

}

void handleLibCounts()
{
    // libc functions
    //

    if (libcCounts) {
        handleLibFunc("modf");
        handleLibFunc("frexp");  handleLibFunc("ldexp");
        handleLibFunc("scalbn"); handleLibFunc("scalbln");
        handleLibFunc("abs");
        handleLibFunc("copysign");
        handleLibFunc("isinf");  handleLibFunc("isnan");
    }

    // libm functions
    //

    if (libmCounts) {

        handleLibFunc("sin");  handleLibFunc("asin");
        handleLibFunc("cos");  handleLibFunc("acos");
        handleLibFunc("tan");  handleLibFunc("atan");  handleLibFunc("atan2");
        handleLibFunc("sinh"); handleLibFunc("asinh");
        handleLibFunc("cosh"); handleLibFunc("acosh");
        handleLibFunc("tanh"); handleLibFunc("atanh");

        handleLibFunc("exp");    handleLibFunc("log");
        handleLibFunc("exp2");   handleLibFunc("log2");
        handleLibFunc("expm1");  handleLibFunc("log10");
        handleLibFunc("ilogb");  handleLibFunc("logb");
        handleLibFunc("log1p");

        handleLibFunc("sqrt"); handleLibFunc("cbrt");
        handleLibFunc("pow");  handleLibFunc("hypot");

        handleLibFunc("erf");  handleLibFunc("tgamma");
        handleLibFunc("erfc"); handleLibFunc("lgamma");

        handleLibFunc("ceil");   handleLibFunc("floor");
        handleLibFunc("fmod");   handleLibFunc("trunc");
        handleLibFunc("rint");   handleLibFunc("round");
        handleLibFunc("lrint");  handleLibFunc("lround");
        handleLibFunc("llrint"); handleLibFunc("llround");
        handleLibFunc("nearbyint");
        handleLibFunc("remainder");
        handleLibFunc("remquo");

        handleLibFunc("fmax"); handleLibFunc("fmin");
        handleLibFunc("fabs");
        handleLibFunc("fdim"); handleLibFunc("fma");
        handleLibFunc("nextafter");
        handleLibFunc("nexttoward");
        handleLibFunc("NAN");

        handleLibFunc("fpclassify"); handleLibFunc("isfinite");
        handleLibFunc("isnormal");   handleLibFunc("signbit");
        handleLibFunc("isgreater");  handleLibFunc("isgreaterequal");
        handleLibFunc("isless");     handleLibFunc("islessequal");
        handleLibFunc("islessgreater");
        handleLibFunc("isunordered");

    }
}

void handleApplication(BPatch_addressSpace *app)
{
	char modname[BUFFER_STRING_LEN];

	// get a reference to the application image
    mainApp = app;
    mainImg = mainApp->getImage();
    mainMgr = PatchAPI::convert(mainApp);

    // get list of all modules
    std::vector<BPatch_module *>* modules;
    std::vector<BPatch_module *>::iterator m;
    modules = mainImg->getModules();

    // cache some objects for speed
    printfFunc  = getMutateeFunction("printf");
    counterType = mainImg->findType(COUNTER_TYPE_STR);

    mainApp->beginInsertionSet();

    initCode.push_back(buildDebugPrint("== Initializing fpinfo analysis ==\n"));

    // for each module ...
    for (m = modules->begin(); m != modules->end(); m++) {
        (*m)->getName(modname, BUFFER_STRING_LEN);

        // for the purposes of this test,
        // don't handle our own library or libm
        if (strcmp(modname, "libm.so.6") == 0 ||
            strcmp(modname, "libc.so.6") == 0) {
            //printf(" skipping %s [%s]\n", name, modname);
            continue;
        }

        // don't handle shared libs unless requested
        if ((*m)->isSharedLib() && !instShared) {
            continue;
        }

        handleModule(*m, modname);
    }

    handleLibCounts();

    finiCode.push_back(buildDebugPrint("== Done with fpinfo analysis ==\n"));

    // add initialization/cleanup at init/fini
    BPatch_module *initFiniModule = getInitFiniModule();
    assert(initFiniModule != NULL);
    BPatch_sequence *initCall = new BPatch_sequence(initCode);
    BPatch_sequence *finiCall = new BPatch_sequence(finiCode);
    initFiniModule->insertInitCallback(*initCall);
    initFiniModule->insertFiniCallback(*finiCall);

    mainApp->finalizeInsertionSet(false); 
}

int main(int argc, char *argv[])
{
    // parse command-line parameters
    for (int i=0; i<argc; i++) {
        if (strcmp(argv[i], "-libm") == 0) {
            libmCounts = true;
        } else if (strcmp(argv[i], "-libc") == 0) {
            libcCounts = true;
        } else {
            binary = argv[i];
        }
    }
    instShared = false;

	// initalize DynInst library
	bpatch = new BPatch;

    // initialize XED library
    xed_tables_init();
    xed_state_zero(&dstate);
    dstate.mmode=XED_MACHINE_MODE_LONG_64;

    printf("Testing %s\n", binary);

    // open binary (and possibly dependencies)
	BPatch_addressSpace *app;
    if (instShared) {
        app = bpatch->openBinary(binary, true);
    } else {
        app = bpatch->openBinary(binary, false);
    }
	if (app == NULL) {
		printf("ERROR: Unable to open application.\n");
        exit(EXIT_FAILURE);
    }

    app->loadLibrary("libc.so.6");
    if (libmCounts) {
        app->loadLibrary("libm.so.6");
    }

    // perform instrumentation
    handleApplication(app);

    ((BPatch_binaryEdit*)app)->writeFile("mutant");
    printf("Done.\n");

	return(EXIT_SUCCESS);
}

