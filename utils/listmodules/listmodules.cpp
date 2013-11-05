// standard C libs
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

// STL classes
#include <string>
#include <vector>

// DyninstAPI
#include "BPatch.h"
#include "BPatch_flowGraph.h"
#include "BPatch_function.h"
#include "BPatch_point.h"

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

// analysis shared library
BPatch_object *libObj  = NULL;

// global parameters
char *binary = NULL;
bool instShared = false;
unsigned long iidx = 0;

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

        printf("  FUNC: %s (%lx)\n", funcname, (unsigned long)function->getBaseAddr());

	}

}

void handleApplication(BPatch_addressSpace *app)
{
	char modname[BUFFER_STRING_LEN];

	// get a reference to the application image
    mainApp = app;
    mainImg = mainApp->getImage();

    // get list of all modules
    std::vector<BPatch_module *>* modules;
    std::vector<BPatch_module *>::iterator m;
    modules = mainImg->getModules();

    mainApp->beginInsertionSet();

    // for each module ...
    for (m = modules->begin(); m != modules->end(); m++) {
        (*m)->getName(modname, BUFFER_STRING_LEN);

        // don't handle shared libs unless requested
        if ((*m)->isSharedLib() && !instShared) {
            continue;
        }

        printf("MODULE: %s (%lx)\n", modname, (unsigned long)(*m)->getBaseAddr());

        handleModule(*m, modname);
    }

    mainApp->finalizeInsertionSet(false); 
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

    // perform test
    handleApplication(app);

    printf("Done.\n");

	return(EXIT_SUCCESS);
}

