/**
 * rfunc.cpp
 *
 * Simple function-replacement mutator.
 * 
 * Mike Lam, Jan 2012
 */

#include <stdlib.h>
#include <iostream>

#include "BPatch.h"
#include "BPatch_binaryEdit.h"
#include "BPatch_image.h"

using namespace std;
using namespace Dyninst;

BPatch *bpatch = NULL;
BPatch_binaryEdit *mainApp = NULL;
BPatch_image *mainImg = NULL;

// this is nearly verbatim the function I use in my larger fpanalysis code
// for function replacement
// 
void replaceFunctionCalls(const char* oldFuncName, const char* newFuncName)
{
    BPatch_Vector<BPatch_function*> funcs;
    BPatch_function *oldFunc;
    BPatch_function *newFunc;

    cout << "Replacing calls to \"" << oldFuncName << "\" with calls to \"" << newFuncName << "\"" << endl;

    // find original function
    mainImg->findFunction(oldFuncName, funcs, false);
    if (!funcs.size() || !funcs[0]) {
        // the function to be replaced does not exist; ignore it
        cout << "WARNING: cannot find function \"" << oldFuncName << "\"" << endl;
        return;
    }
    oldFunc = funcs[0];

    // find new function
    funcs.clear();
    mainImg->findFunction(newFuncName, funcs, false);
    if (!funcs.size()) {
        cout << "WARNING: cannot find function \"" << newFuncName << "\"" << endl;
        return;
    }
    newFunc = funcs[0];

    // prevent infinite self-recursive loop
    if (strcmp(oldFuncName,newFuncName)==0 || oldFunc == newFunc) {
        // replacing function with a call to self; ignore it
        return;
    }

    mainApp->replaceFunction(*oldFunc, *newFunc);
}

int main(int argc, const char *argv[])
{
    if (argc != 4) {
        cout << "Usage:  rfunc <mutatee> <old-func-name> <new-func-name>" << endl;
        return EXIT_FAILURE;
    }

    // open mutatee with dependencies
	bpatch = new BPatch;
    mainApp = bpatch->openBinary(argv[1], true);
    mainImg = mainApp->getImage();
	if (mainApp == NULL) {
		cout << "ERROR: Unable to open mutatee." << endl;
        exit(EXIT_FAILURE);
    }

    // make the call to the function replacement routine
    replaceFunctionCalls(argv[2], argv[3]);

    // write the modified binary back to the current directory
    // (should be "mutant" + any modified dependencies)
    cout << "Writing new binary to mutant ..." << endl;
    mainApp->writeFile("mutant");

    return EXIT_SUCCESS;
}

