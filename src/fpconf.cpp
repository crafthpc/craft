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

// main log (used for debug info, not actual logging)
FPLog *mainLog = NULL;

// main configuration
FPConfig *mainConfig = NULL;

// main instruction decoder
FPDecoder *mainDecoder = NULL;

// main analysis engine
FPAnalysis        *mainAnalysis        = NULL;
FPAnalysisInplace *mainAnalysisInplace = NULL;

// instrumentation modes
bool nullInst = false;
bool countInst = false;
bool detectCancel = false;
bool detectNaN = false;
bool trackRange = false;
bool inplaceSV = false;
bool reducePrec = false;
char* inplaceSVType = NULL;
unsigned long reducePrecDefaultPrec = 0;

// configuration file options
char *binary = NULL;
long binaryArg = 0;
bool addAll = false;
bool outputCandidates = false;
bool reportOriginal = false;

// function/instruction indices and counts
size_t midx = 0, fidx = 0, bbidx = 0, iidx = 0;
size_t total_modules = 0;
size_t total_functions = 0;
size_t total_basicblocks = 0;
size_t total_fp_instructions = 0;

// options
bool instShared;

// temporary entries
FPReplaceEntry *tempModRE  = NULL;
FPReplaceEntry *tempFuncRE = NULL;
FPReplaceEntry *tempBblkRE = NULL;
FPReplaceEntry *tempInsnRE = NULL;

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
    stringstream ss;
    ss.clear(); ss.str("");
    ss << inst->getDisassembly() << "  ["
       << mainLog->getSourceLineInfo(inst->getAddress()) << "]";
    entry->name = ss.str();
    entry->address = addr;

    // if we're running in "report" mode, report the highest-precision IEEE
    // operand present in the instruction
    if (reportOriginal) {
        if (inst->hasOperandOfType(IEEE_Double)) {
            entry->tag = RETAG_DOUBLE;
        } else if (inst->hasOperandOfType(IEEE_Single)) {
            entry->tag = RETAG_SINGLE;
        } else {
            entry->tag = RETAG_NONE;
        }
    }

    // handle pre-instrumentation analysis types
    if (mainAnalysis && mainAnalysis->shouldPreInstrument(inst)) {
        if (outputCandidates) {
            entry->tag = RETAG_CANDIDATE;
        } else if (nullInst) {
            entry->tag = RETAG_NULL;
        } else if (countInst) {
            entry->tag = RETAG_CINST;
        } else if (detectCancel) {
            entry->tag = RETAG_DCANCEL;
        } else if (detectNaN) {
            entry->tag = RETAG_DNAN;
        }
    }

    // configure range-tracking analysis
    if (trackRange && mainAnalysis->shouldReplace(inst)) {
        if (outputCandidates) {
            entry->tag = RETAG_CANDIDATE;
        } else {
            entry->tag = RETAG_TRANGE;
        }
    }

    // configure general replacement analysis
    if (inplaceSV && mainAnalysisInplace->shouldReplace(inst)) {
        if (outputCandidates) {
            entry->tag = mainAnalysisInplace->getDefaultRETag(inst);
        } else if (mainAnalysisInplace->getSVType(inst) == SVT_IEEE_Single) {
            entry->tag = RETAG_SINGLE;
        } else /* if (mainAnalysisInplace->getSVType(inst) == SVT_IEEE_Double) */ {
            entry->tag = RETAG_DOUBLE;
        }
    }

    // configure reduced precision analysis
    if (reducePrec && mainAnalysis->shouldReplace(inst)) {
        if (outputCandidates) {
            entry->tag = RETAG_CANDIDATE;
        } else {
            entry->tag = RETAG_RPREC;
        }
    }

    // don't report "none" or "ignore" unless explicitly desired
    if (!(entry->tag == RETAG_NONE || entry->tag == RETAG_IGNORE) || addAll) {

        // flush any buffered modules, functions, and basic blocks
        if (tempModRE) {
            mainConfig->addReplaceEntry(tempModRE);
            tempModRE = NULL;
        }
        if (tempFuncRE) {
            mainConfig->addReplaceEntry(tempFuncRE);
            tempFuncRE = NULL;
        }
        if (tempBblkRE) {
            mainConfig->addReplaceEntry(tempBblkRE);
            tempBblkRE = NULL;
        }
        mainConfig->addReplaceEntry(entry);
    }
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
    if (addAll) {
        mainConfig->addReplaceEntry(entry);
    } else {
        tempBblkRE = entry;
    }

    // get all instructions
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
    if (addAll) {
        mainConfig->addReplaceEntry(entry);
    } else {
        tempFuncRE = entry;
    }

    // config all basic blocks
    std::set<BPatch_basicBlock*> blocks;
    std::set<BPatch_basicBlock*>::iterator b;
    BPatch_flowGraph *cfg = function->getCFG();
    cfg->getAllBasicBlocks(blocks);
    for (b = blocks.begin(); b != blocks.end(); b++) {
        configBasicBlock(*b);
    }
}

void configModule(BPatch_module *mod, const char *name)
{
	char funcname[BUFFER_STRING_LEN];

    midx++;
    total_modules++;

    // build config entry
    FPReplaceEntry *entry = new FPReplaceEntry(RETYPE_MODULE, midx);
    entry->name = name;
    entry->address = mod->getBaseAddr();
    if (addAll) {
        mainConfig->addReplaceEntry(entry);
    } else {
        tempModRE = entry;
    }

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
		if ( (strcmp(funcname,"memset")!=0) && (strcmp(funcname,"call_gmon_start")!=0) &&
                (strcmp(funcname,"frame_dummy")!=0) && funcname[0] != '_'
                ) {

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

    // build config entry
    FPReplaceEntry *entry = new FPReplaceEntry(RETYPE_APP, 1);
    char *appName = binary, *tmp = binary;
    while (*tmp != '\0') {
        if (*tmp++ == '/') {
            appName = tmp;
        }
    }
    entry->name = appName;
    mainConfig->addReplaceEntry(entry);

    // get list of all modules
    std::vector<BPatch_module *>* modules;
    std::vector<BPatch_module *>::iterator m;
    modules = mainImg->getModules();

    // for each module ...
    for (m = modules->begin(); m != modules->end(); m++) {
        (*m)->getName(modname, BUFFER_STRING_LEN);

        // don't config our own library or libc/libm
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
	printf("\nUsage:  fpconf <analysis> [<options>] <binary>\n");
    printf(" Performs floating-point configuration on a binary.\n");
    printf("Analyses:\n");
    printf("\n");
    printf("  --report             report original precision: single/double (default)\n");
    printf("  --null               null instrumentation\n");
	printf("  --cinst              count all floating-point instructions\n");
	printf("  --dcancel            detect cancellation events\n");
	printf("  --dnan               detect NaN values\n");
	printf("  --trange             track operand value ranges\n");
    printf("  --svinp <policy>     in-place replacement shadow value analysis\n");
    printf("                         valid policies: \"single\", \"double\", \"mem_single\", \"mem_double\"\n");
    printf("  --rprec <bits>       reduced precision analysis\n");
    printf("\n");
    printf(" Options:\n");
    printf("\n");
    printf("  -a                   output all instructions (including those that would normally be ignored)\n");
    printf("  -c                   output original candidate configuration for automated search\n");
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
        } else if (strcmp(argv[i], "-a")==0) {
            addAll = true;
        } else if (strcmp(argv[i], "-c")==0) {
            outputCandidates = true;
		} else if (strcmp(argv[i], "-s")==0) {
			instShared = true;
		} else if (strcmp(argv[i], "--null")==0) {
			nullInst = true;
		} else if (strcmp(argv[i], "--report")==0) {
            reportOriginal = true;
		} else if (strcmp(argv[i], "--cinst")==0) {
            countInst = true;
		} else if (strcmp(argv[i], "--dcancel")==0) {
            detectCancel = true;
		} else if (strcmp(argv[i], "--dnan")==0) {
            detectNaN = true;
		} else if (strcmp(argv[i], "--trange")==0) {
            trackRange = true;
		} else if (strcmp(argv[i], "--svinp")==0) {
            inplaceSV = true;
            inplaceSVType = argv[++i];
		} else if (strcmp(argv[i], "--rprec")==0) {
            reducePrec = true;
            reducePrecDefaultPrec = strtoul(argv[++i], NULL, 10);
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

void initialize_analysis()
{
    if (nullInst) {
        mainAnalysis = new FPAnalysis();
    } else if (countInst) {
        mainConfig->setValue("c_inst", "yes");
        mainAnalysis = FPAnalysisCInst::getInstance();
    } else if (detectCancel) {
        mainConfig->setValue("d_cancel", "yes");
        mainAnalysis = FPAnalysisDCancel::getInstance();
    } else if (detectNaN) {
        mainConfig->setValue("d_nan", "yes");
        mainAnalysis = FPAnalysisDNan::getInstance();
    } else if (trackRange) {
        mainConfig->setValue("t_range", "yes");
        mainAnalysis = FPAnalysisTRange::getInstance();
    } else if (inplaceSV) {
        mainConfig->setValue("sv_inp", "yes");
        mainConfig->setValue("sv_inp_type", inplaceSVType);
        mainAnalysis = FPAnalysisInplace::getInstance();
        mainAnalysisInplace = FPAnalysisInplace::getInstance();
    } else if (reducePrec) {
        stringstream ss(""); ss << reducePrecDefaultPrec;
        mainConfig->setValue("r_prec", "yes");
        mainConfig->setValue("r_prec_default_precision", ss.str());
        mainAnalysis = FPAnalysisRPrec::getInstance();
    }
    if (mainAnalysis) {
        mainAnalysis->configure(mainConfig, mainDecoder, NULL, NULL);
    } else {
        reportOriginal = true;
    }
}
// }}}


int main(int argc, char *argv[])
{
    // analysis configuration
	if (!parseCommandLine((unsigned)argc, argv)) {
		usage();
		exit(EXIT_FAILURE);
	}
    mainLog = new FPLog("fpconf.log");
    mainLog->setDebugFile(binary);
    mainConfig = new FPConfig();

	// initalize DynInst library
	bpatch = new BPatch;
	bpatch->registerErrorCallback(errorFunc);

    // initialize instruction decoder
    mainDecoder = new FPDecoderXED();       // use Intel's decoder from Pin
    //mainDecoder = new FPDecoderIAPI();    // use Dyninst's InstructionAPI
    
    // set up analysis for decision-making
    initialize_analysis();

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

    // build configuration
    configApplication(app);

    // output configuration
    if (inplaceSV) {
        mainConfig->setValue("sv_inp_type", "config");
    }
    mainConfig->buildFile(cout);

	return(EXIT_SUCCESS);
}

