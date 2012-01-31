#ifndef __FPCONF_H
#define __FPCONF_H

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

// helpers
#include "FPConfig.h"
#include "FPDecoderXED.h"
#include "FPDecoderIAPI.h"
#include "FPFilterFunc.h"
#include "FPSemantics.h"

// analyses
#include "FPAnalysisInplace.h"

using namespace std;
using namespace Dyninst;
using namespace Dyninst::InstructionAPI;
using namespace Dyninst::PatchAPI;
using namespace FPInst;

// error handling
void errorFunc(BPatchErrorLevel level, int num, const char * const *params);

// helper functions
bool fpFilterFunc(Instruction::Ptr inst);

// configuration functions
void configInstruction(void *addr, unsigned char *bytes, size_t nbytes);
void configBasicBlock(BPatch_basicBlock *block);
void configFunction(BPatch_function *function, const char *name);
void configApplication(BPatch_addressSpace *app);

// command-line parsing and help text
void usage();
void parseFuncFile(char *fn, vector<string> &funcs);
bool parseCommandLine(unsigned argc, char *argv[]);

#endif

