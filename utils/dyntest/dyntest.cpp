// standard C libs
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

// STL classes
#include <string>
#include <vector>

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

// main Dyninst driver structure
BPatch *bpatch = NULL;

// top-level Dyninst mutatee structures
BPatch_addressSpace *mainApp = NULL;
BPatch_image        *mainImg = NULL;
PatchMgr::Ptr        mainMgr;

// analysis shared library
BPatch_object *libObj  = NULL;

// global parameters
char *binary = NULL;
bool instShared = false;
unsigned long iidx = 0;

// instructions to instrument
const bool FILTER_ENABLED = false;
const bool FILTER_INVERSE = false;  // false = keep ONLY instructions in filter
                                    // true  = keep all EXCEPT named instructions
const unsigned long FILTER_INSNS[] = { };
const unsigned long FILTER_INSNS_SIZE = 0;

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

BPatch_function* getMutateeFunction(const char *name) {

    // assumes there is a single function in the mutatee address space
    // with the given name; finds and returns it
    BPatch_Vector<BPatch_function *> funcs;
    mainImg->findFunction(name, funcs, true, true, true);
    assert(funcs.size() == 1);
    return funcs.at(0);
}

Snippet::Ptr buildInstrumentation(void *addr) {

    // null instrumentation (for testing)
    //BPatch_snippet *nullExpr = new BPatch_nullExpr();
    //return PatchAPI::convert(nullExpr);

    // instruction count instrumentation
    BPatch_variableExpr *instVar = mainImg->findVariable("_INST_main_inst_count");
    BPatch_snippet *idxExpr = new BPatch_arithExpr(BPatch_plus,
            *instVar, BPatch_constExpr(iidx*sizeof(unsigned long)));
    BPatch_snippet *countExpr = new BPatch_arithExpr(BPatch_deref,
            *idxExpr);
    BPatch_snippet *valExpr = new BPatch_arithExpr(BPatch_plus,
            *countExpr, BPatch_constExpr(1));
    BPatch_snippet *incExpr = new BPatch_arithExpr(BPatch_assign,
            *idxExpr, *valExpr);
    iidx++;
    return PatchAPI::convert(incExpr);
}

void handleInstruction(void *addr, Instruction::Ptr iptr, PatchBlock *block, PatchFunction *func) {

    bool handle = true;

    // filter unwanted instructions if the filter is enabled
    if (FILTER_ENABLED) {
        handle = (FILTER_INVERSE ? true : false);
        for (unsigned i=0; i<FILTER_INSNS_SIZE; i++) {
            if ((unsigned long)addr == FILTER_INSNS[i]) {
                handle = (FILTER_INVERSE ? false : true);
            }
        }
    }
    if (!handle) {
        return;
    }

    // print instruction info
    printf("  instruction at %lx: %s\n",
            (unsigned long)addr, iptr->format((Address)addr).c_str());

    // grab instrumentation point
    Point *prePoint  = mainMgr->findPoint(
                        Location::InstructionInstance(func, block, (Address)addr),
                        Point::PreInsn, true);

    // build and insert instrumentation
    prePoint->pushBack(buildInstrumentation(addr));
}

void handleBasicBlock(BPatch_basicBlock *block, PatchFunction *func)
{
    static size_t MAX_RAW_INSN_SIZE = 16;

    Instruction::Ptr iptr;
    void *addr;
    unsigned char bytes[MAX_RAW_INSN_SIZE];
    size_t nbytes, i;

    // get all floating-point instructions
    PatchBlock::Insns insns;
    PatchAPI::convert(block)->getInsns(insns);

    // handle each point separately
    PatchBlock::Insns::iterator j;
    for (j = insns.begin(); j != insns.end(); j++) {

        // get instruction bytes
        addr = (void*)((*j).first);
        iptr = (*j).second;
        nbytes = iptr->size();
        assert(nbytes <= MAX_RAW_INSN_SIZE);
        for (i=0; i<nbytes; i++) {
            bytes[i] = iptr->rawByte(i);
        }
        bytes[nbytes] = '\0';

        handleInstruction(addr, iptr, PatchAPI::convert(block), func);
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
        handleBasicBlock(*b, PatchAPI::convert(function));
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

    mainApp->beginInsertionSet();

    // for each module ...
    for (m = modules->begin(); m != modules->end(); m++) {
        (*m)->getName(modname, BUFFER_STRING_LEN);

        // for the purposes of this test,
        // don't handle our own library or libm
        if (strcmp(modname, "libdyntest.so") == 0 ||
            strcmp(modname, "libm.so.6") == 0 ||
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

    // add initialization/cleanup at init/fini
    BPatch_module *initFiniModule = getInitFiniModule();
    assert(initFiniModule != NULL);
    BPatch_function *initFunc = getMutateeFunction("_INST_init_analysis");
    BPatch_function *finiFunc = getMutateeFunction("_INST_cleanup_analysis");
    BPatch_Vector<BPatch_snippet*> *blankArgs = new BPatch_Vector<BPatch_snippet*>();
    BPatch_funcCallExpr *initCall = new BPatch_funcCallExpr(*initFunc, *blankArgs);
    BPatch_funcCallExpr *finiCall = new BPatch_funcCallExpr(*finiFunc, *blankArgs);
    initFiniModule->insertInitCallback(*initCall);
    initFiniModule->insertFiniCallback(*finiCall);

    mainApp->finalizeInsertionSet(false); 
}

void writeConfig(const char *fname) {
    FILE *fc = fopen(fname, "w");
    fprintf(fc, "num_instructions=%d\n", iidx);
    fclose(fc);
}

int main(int argc, char *argv[])
{
    // ABBREVIATED: parse command-line parameters
    binary = argv[1];
    instShared = false;

	// initalize DynInst library
	bpatch = new BPatch;

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

    // add the instrumentation library
    libObj = ((BPatch_binaryEdit*)app)->loadLibrary("libdyntest.so");
	if (libObj == NULL) {
		printf("ERROR: Unable to open libdyntest.so.\n");
        exit(EXIT_FAILURE);
    }

    // perform test
    handleApplication(app);

    ((BPatch_binaryEdit*)app)->writeFile("mutant");
    writeConfig("mutant.cfg");
    printf("Done.\n");

	return(EXIT_SUCCESS);
}

