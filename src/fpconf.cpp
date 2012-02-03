/*
 * libconf.cpp
 *
 * Floating-point configuration utility
 *
 * Originally written by Mike Lam, Fall 2011
 */

#include "fpconf.h"

#define BUFFER_STRING_LEN 1024

// {{{ top-level data members (global parameters, counts, etc.)

// main Dyninst driver structure
BPatch *bpatch = NULL;
BPatch_addressSpace *mainApp = NULL;
BPatch_image *mainImg = NULL;

// main configuration
FPConfig *mainConfig = NULL;

// main instruction decoder
FPDecoder *mainDecoder = NULL;

// main analysis engine
FPAnalysisInplace *mainAnalysisInplace = NULL;

// configuration file options
char *binary = NULL;
long binaryArg = 0;

// function/instruction indices and counts
size_t fidx = 0, bbidx = 0, iidx = 0;
size_t total_functions = 0;
size_t total_basicblocks = 0;
size_t total_fp_instructions = 0;

// options
bool instShared;

// }}}

// {{{ error handling (boilerplate--no need to touch this!)
#define DYNINST_NO_ERROR -1
int errorPrint = 1; // external "dyninst" tracing (via errorFunc)
int expectError = DYNINST_NO_ERROR;
void errorFunc(BPatchErrorLevel level, int num, const char * const *params)
{
    if (num == 0) {
        // conditional reporting of warnings and informational messages
        if (errorPrint) {
            if ((level == BPatchInfo) || (level == BPatchWarning))
              { if (errorPrint > 1) printf("%s\n", params[0]); }
            else {
                printf("%s", params[0]);
            }
        }
    } else {
        // reporting of actual errors
        char line[256];
        const char *msg = bpatch->getEnglishErrorString(num);
        bpatch->formatErrorString(line, sizeof(line), msg, params);
        
        if (num != expectError) {
	  if(num != 112)
	    printf("Error #%d (level %d): %s\n", num, level, line);
        
            // We consider some errors fatal.
            if (num == 101) {
               exit(-1);
            }
        }
    }
}

// }}}

// {{{ helper functions

string strip_to_base_filename(string fname)
{
    size_t pos = fname.rfind('/');
    if (pos != string::npos) {
        return fname.substr(pos+1);
    } else {
        return fname;
    }
}

bool fpFilterFunc(Instruction::Ptr inst)
{
    unsigned char buffer[64];
    int size = inst->size();
    memcpy(buffer, inst->ptr(), size);
    return mainDecoder->filter(buffer, size);
}

// }}}

// {{{ application, function, basic block, and instruction configure routines

void configInstruction(void *addr, unsigned char *bytes, size_t nbytes)
{
    iidx++;
    total_fp_instructions++;

    // decode instruction
    FPSemantics *inst = mainDecoder->decode(iidx, addr, bytes, nbytes);

    // build config entry
    FPReplaceEntry *entry = new FPReplaceEntry(RETYPE_INSTRUCTION, iidx);
    entry->name = inst->getDisassembly();
    entry->address = addr;
    if (mainAnalysisInplace->shouldReplace(inst)) {
        entry->tag = RETAG_SINGLE;
        mainConfig->addReplaceEntry(entry);
    } else {
        entry->tag = RETAG_NONE;
    }
    //mainConfig->addReplaceEntry(entry);
}

void configBasicBlock(BPatch_basicBlock *block)
{
    static size_t MAX_RAW_INSN_SIZE = 16;

    Instruction::Ptr iptr;
    void *addr;
    unsigned char bytes[MAX_RAW_INSN_SIZE];
    size_t nbytes, i;

    bbidx++;
    total_basicblocks++;

    // build config entry
    FPReplaceEntry *entry = new FPReplaceEntry(RETYPE_BASICBLOCK, bbidx);
    entry->address = (void*)block->getStartAddress();
    mainConfig->addReplaceEntry(entry);

    // get all floating-point instructions
    PatchBlock::Insns insns;
    PatchAPI::convert(block)->getInsns(insns);

    // config each point separately
    PatchBlock::Insns::iterator j;
    //PatchBlock::Insns::reverse_iterator j;
    for (j = insns.begin(); j != insns.end(); j++) {
    //for (j = insns.rbegin(); j != insns.rend(); j++) {

        // get instruction bytes
        addr = (void*)((*j).first);
        iptr = (*j).second;
        nbytes = iptr->size();
        assert(nbytes <= MAX_RAW_INSN_SIZE);
        for (i=0; i<nbytes; i++) {
            bytes[i] = iptr->rawByte(i);
        }
        bytes[nbytes] = '\0';

        // apply filter
        if (mainDecoder->filter(bytes, nbytes)) {
            configInstruction(addr, bytes, nbytes);
        }
    }
}

void configFunction(BPatch_function *function, const char *name)
{
    fidx++;
    total_functions++;

    // build config entry
    FPReplaceEntry *entry = new FPReplaceEntry(RETYPE_FUNCTION, fidx);
    entry->name = name;
    entry->address = function->getBaseAddr();
    mainConfig->addReplaceEntry(entry);

    // config all basic blocks
    std::set<BPatch_basicBlock*> blocks;
    std::set<BPatch_basicBlock*>::iterator b;
    //std::set<BPatch_basicBlock*>::reverse_iterator b;
    BPatch_flowGraph *cfg = function->getCFG();
    cfg->getAllBasicBlocks(blocks);
    for (b = blocks.begin(); b != blocks.end(); b++) {
    //for (b = blocks.rbegin(); b != blocks.rend(); b++) {
        configBasicBlock(*b);
    }
}

void configApplication(BPatch_addressSpace *app)
{
	// get a reference to the application image
    mainApp = app;
    mainImg = mainApp->getImage();

	// string/char buffers
	char name[BUFFER_STRING_LEN];
	char modname[BUFFER_STRING_LEN];
    stringstream ss;

    // build config entry
    FPReplaceEntry *entry = new FPReplaceEntry(RETYPE_APP, 1);
    entry->name = binary;
    mainConfig->addReplaceEntry(entry);

	// get list of all functions
	std::vector<BPatch_function *>* functions;
	functions = mainImg->getProcedures();

	// for each function ...
	for (unsigned i = 0; i < functions->size(); i++) {
		BPatch_function *function = functions->at(i);
		function->getName(name, BUFFER_STRING_LEN);
		function->getModule()->getName(modname, BUFFER_STRING_LEN);

        // don't config functions in our own library or libm
        if (strcmp(modname, "libfpanalysis.so") == 0 ||
            strcmp(modname, "libm.so.6") == 0) {
            //printf(" skipping %s [%s]\n", name, modname);
            continue;
        }

        // CRITERIA FOR INSTRUMENTATION:
        // don't config functions from shared libraries (unless requested)
        //   or main() or memset() or call_gmon_start() or frame_dummy()
        //   or functions that begin with an underscore
        //   or functions that begin with "targ"
        // AND
        // if there's a preset list of functions to config,
        //   make sure the current function is on it
        // (and the inverse for excluded functions)
		if (/*!function->getModule()->isSharedLib() && */
                (instShared || !function->getModule()->isSharedLib())
                /* && (strcmp(name,"main")!=0) */ && (strcmp(name,"memset")!=0)
                && (strcmp(name,"call_gmon_start")!=0) && (strcmp(name,"frame_dummy")!=0)
                && name[0] != '_' &&
                !(strlen(name) > 4 && name[0]=='t' && name[1]=='a' && name[2]=='r' && name[3]=='g')) {

            configFunction(function, name);
		}
	}
    
    // generate application report
    /*
     *stringstream report;
     *report.clear(); report.str("");
     *report << "Finished configuration:" << endl;
     *report << "  " << total_fp_instructions << " floating-point instruction(s)" << endl;
     *report << "  " << total_basicblocks << " basic block(s)" << endl;
     *report << "  " << total_functions << " function(s)" << endl;
     *cout << report.str();
     */
}

// }}}

// {{{ command-line parsing and help text

void usage()
{
	printf("\nUsage:  fpconf [<options>] <binary>\n");
    printf(" Performs floating-point configuration on a binary.\n");
    printf("\n");
    printf(" Options:\n");
    printf("\n");
    printf("  -s                   configure functions in shared libraries\n");
    printf("\n");
}

bool parseCommandLine(unsigned argc, char *argv[])
{
    unsigned i;
    bool validArgs = false;

    // analysis arguments
	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-h")==0) {
			usage();
			exit(EXIT_SUCCESS);
		} else if (strcmp(argv[i], "-s")==0) {
			instShared = true;
        } else if (argv[i][0] == '-') {
            printf("Unrecognized option: %s\n", argv[i]);
            usage();
            exit(EXIT_FAILURE);
		} else if (i <= argc-1) {
            binary = argv[i];
            binaryArg = i;
			validArgs = true;
			break;
		}
	}

    return validArgs;
}

// }}}


int main(int argc, char *argv[])
{
    // analysis configuration
	if (!parseCommandLine((unsigned)argc, argv)) {
		usage();
		exit(EXIT_FAILURE);
	}
    mainConfig = new FPConfig();

	// initalize DynInst library
	bpatch = new BPatch;
	bpatch->registerErrorCallback(errorFunc);

    // initialize instruction decoder
    mainDecoder = new FPDecoderXED();       // use Intel's decoder from Pin
    //mainDecoder = new FPDecoderIAPI();    // use Dyninst's InstructionAPI
    
    // initialize analysis
    mainAnalysisInplace = FPAnalysisInplace::getInstance();
    mainAnalysisInplace->configure(new FPConfig(), mainDecoder, NULL, NULL);

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

    cout << "sv_inp=yes" << endl;
    cout << "sv_inp_type=config" << endl;
    cout << mainConfig->getSummary();

	return(EXIT_SUCCESS);
}

