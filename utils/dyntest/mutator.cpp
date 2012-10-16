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
BPatch_addressSpace *mainApp = NULL;
BPatch_image *mainImg = NULL;

char *binary = NULL;
bool instShared = false;

void configBasicBlock(BPatch_basicBlock *block)
{
    static size_t MAX_RAW_INSN_SIZE = 16;

    Instruction::Ptr iptr;
    void *addr;
    unsigned char bytes[MAX_RAW_INSN_SIZE];
    size_t nbytes, i;

    // get all floating-point instructions
    PatchBlock::Insns insns;
    PatchAPI::convert(block)->getInsns(insns);

    // config each point separately
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

        printf("  instruction at %lx: %s\n", (unsigned long)addr, iptr->format((Address)addr).c_str());
    }
}

void configFunction(BPatch_function *function, const char *name)
{
    printf("function %s:\n", name);

    // config all basic blocks
    std::set<BPatch_basicBlock*> blocks;
    std::set<BPatch_basicBlock*>::iterator b;
    BPatch_flowGraph *cfg = function->getCFG();
    cfg->getAllBasicBlocks(blocks);
    for (b = blocks.begin(); b != blocks.end(); b++) {
        configBasicBlock(*b);
    }

    printf("\n");
}

void configModule(BPatch_module *mod, const char *name)
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
        // don't config:
        //   - main() or memset() or call_gmon_start() or frame_dummy()
        //   - functions that begin with an underscore
		if ( /* (strcmp(funcname,"main")!=0) && */ (strcmp(funcname,"memset")!=0)
                && (strcmp(funcname,"call_gmon_start")!=0) && (strcmp(funcname,"frame_dummy")!=0)
                && funcname[0] != '_') {

            configFunction(function, funcname);
		}
	}

}

void configApplication(BPatch_addressSpace *app)
{
	char modname[BUFFER_STRING_LEN];

	// get a reference to the application image
    mainApp = app;
    mainImg = mainApp->getImage();

    // get list of all modules
    std::vector<BPatch_module *>* modules;
    std::vector<BPatch_module *>::iterator m;
    modules = mainImg->getModules();

    // for each module ...
    for (m = modules->begin(); m != modules->end(); m++) {
        (*m)->getName(modname, BUFFER_STRING_LEN);

        // don't config our own library or libm
        if (strcmp(modname, "libfpanalysis.so") == 0 ||
            strcmp(modname, "libm.so.6") == 0 ||
            strcmp(modname, "libc.so.6") == 0) {
            //printf(" skipping %s [%s]\n", name, modname);
            continue;
        }

        // don't config shared libs unless requested
        if ((*m)->isSharedLib() && !instShared) {
            continue;
        }

        configModule(*m, modname);
    }
    
}

int main(int argc, char *argv[])
{
    // ABBREVIATED: parse command-line parameters
    binary = argv[1];
    instShared = false;

	// initalize DynInst library
	bpatch = new BPatch;

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

    // perform configuration
    configApplication(app);

	return(EXIT_SUCCESS);
}

