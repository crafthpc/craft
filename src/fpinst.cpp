/*
 * fpinst.cpp
 *
 * Floating-point analysis instrumenter
 *
 * Originally written by Mike Lam, Fall 2008
 */

#include "fpinst.h"
//#include "fpinst_quad.h"

#define BUFFER_STRING_LEN 1024

// {{{ top-level data members (global parameters, counts, etc.)

// main Dyninst driver structure
BPatch *bpatch = NULL;
BPatch_addressSpace *mainApp = NULL;
BPatch_image *mainImg = NULL;
BPatch_object *libFPAnalysis = NULL;
BPatch_object *libFPC = NULL;
BPatch_object *libFPM = NULL;
PatchMgr::Ptr mainMgr;
bool patchAPI_debug = false;

// main instruction decoder
FPDecoder *mainDecoder = NULL;

// instrumentation log file
FPLog *logfile = NULL;

// instrumentation modes
bool nullInst = false;
char* pointerSVType = NULL;
char* inplaceSVType = NULL;
unsigned long reducePrecDefaultPrec = 0;

// analysis information
bool  fpinstAnalysisEnabled[TOTAL_ANALYSIS_COUNT];
char* fpinstAnalysisParam[TOTAL_ANALYSIS_COUNT];
vector<FPAnalysis*> activeAnalyses;

// instrumentation file options
char *binary = NULL;
long binaryArg = 0;
char outFileName[] = "mutant";
char *outFile = outFileName;
char configFileName[] = "base.cfg";
char *configFile = configFileName;
char *logFile = NULL;
char *logTag = NULL;
char *child_argv[45];
char *child_envp[10];

// instrumentation function options
char mainFuncName[] = "main";
char *exitFuncName = mainFuncName;
char restrictFuncs = ' ';
vector<string> funcs;
FPConfig *configuration = NULL;
vector<string> extraConfigs;

// instrumentation flags
bool process = false;           // run as process
bool listFuncs = false;         // list functions instrumented
bool instShared = false;        // instrument shared libraries
bool saveFPR = false;           // save FPR explicitly (DEPRECATED?)
bool instOnly = false;          // don't resume/execute
bool fastInst = false;          // enable optimized snippets
bool mismatches = false;        // enable mismatch calculation
bool cancellations = false;     // enable cancellation detection in shadow analysis (DEPRECATED)
bool debugPrint = false;        // print debug messages
bool stwalk = false;            // enable stack walks
bool reportAllGlobals = false;  // print all global variables from shadow analysis
bool profiling = false;         // enable profiling output
bool disableSampling = false;   // disable logarithmic cancellation sampling
bool instrFrames = false;       // add instrumentation stack frames
bool fortranMode = false;       // switch up instrumentation for FORTRAN programs
bool multicoreMode = false;     // use the LOCK prefix for INC instructions

// function/instruction indices and counts
size_t midx = 0, fidx = 0, bbidx = 0, iidx = 0;
size_t total_replacements = 0;
size_t total_modules = 0;
size_t total_functions = 0;
size_t total_basicblocks = 0;
size_t total_fp_instructions = 0;
size_t total_instrumented_instructions = 0;

// }}}

// {{{ Dyninst pieces (reg snippets, function pointers, etc.)

// architecture-dependent constants
#ifdef __X86_64__
string eaxRegName("rax");
string ebxRegName("rbx");
string ecxRegName("rcx");
string edxRegName("rdx");
string espRegName("rsp");
string ebpRegName("rbp");
string esiRegName("rsi");
string ediRegName("rdi");
string r8RegName("r8");
string r9RegName("r9");
string r10RegName("r10");
string r11RegName("r11");
string r12RegName("r12");
string r13RegName("r13");
string r14RegName("r14");
string r15RegName("r15");
string eflagsRegName("eflags");
#else
string eaxRegName("eax");
string ebxRegName("ebx");
string ecxRegName("ecx");
string edxRegName("edx");
string espRegName("esp");
string ebpRegName("ebp");
string esiRegName("esi");
string ediRegName("edi");
string r8RegName("e8");     // "e8"-"e15" won't exist -- doesn't matter
string r9RegName("e9");
string r10RegName("e10");
string r11RegName("e11");
string r12RegName("e12");
string r13RegName("e13");
string r14RegName("e14");
string r15RegName("e15");
string eflagsRegName("eflags");
#endif

// register snippets
BPatch_registerExpr *eaxExpr = NULL;
BPatch_registerExpr *ebxExpr = NULL;
BPatch_registerExpr *ecxExpr = NULL;
BPatch_registerExpr *edxExpr = NULL;
BPatch_registerExpr *espExpr = NULL;
BPatch_registerExpr *ebpExpr = NULL;
BPatch_registerExpr *esiExpr = NULL;
BPatch_registerExpr *ediExpr = NULL;
BPatch_registerExpr *r8Expr  = NULL;
BPatch_registerExpr *r9Expr  = NULL;
BPatch_registerExpr *r10Expr = NULL;
BPatch_registerExpr *r11Expr = NULL;
BPatch_registerExpr *r12Expr = NULL;
BPatch_registerExpr *r13Expr = NULL;
BPatch_registerExpr *r14Expr = NULL;
BPatch_registerExpr *r15Expr = NULL;
BPatch_registerExpr *eflagsExpr = NULL;

// register pointer snippets
BPatch_variableExpr *eipPtrExpr = NULL;
BPatch_variableExpr *eflagsPtrExpr = NULL;
BPatch_variableExpr *eaxPtrExpr = NULL;
BPatch_variableExpr *ebxPtrExpr = NULL;
BPatch_variableExpr *ecxPtrExpr = NULL;
BPatch_variableExpr *edxPtrExpr = NULL;
BPatch_variableExpr *ebpPtrExpr = NULL;
BPatch_variableExpr *espPtrExpr = NULL;
BPatch_variableExpr *esiPtrExpr = NULL;
BPatch_variableExpr *ediPtrExpr = NULL;
BPatch_variableExpr *r8PtrExpr  = NULL;
BPatch_variableExpr *r9PtrExpr  = NULL;
BPatch_variableExpr *r10PtrExpr = NULL;
BPatch_variableExpr *r11PtrExpr = NULL;
BPatch_variableExpr *r12PtrExpr = NULL;
BPatch_variableExpr *r13PtrExpr = NULL;
BPatch_variableExpr *r14PtrExpr = NULL;
BPatch_variableExpr *r15PtrExpr = NULL;

// instrumentation library functions
BPatch_function* initFunc;
BPatch_function* enableFunc;
BPatch_function* setFunc;
BPatch_function* setReplaceFunc;
BPatch_function* regFunc;
BPatch_function* handlePreFunc;
BPatch_function* handlePostFunc;
BPatch_function* handleReplFunc;
BPatch_function* handleUnspFunc;
BPatch_function* disableFunc;
BPatch_function* cleanupFunc;
BPatch_function* mainFunc;
BPatch_function* exitFunc;
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

void setup_config_file(FPConfig *configuration)
{
    vector<string>::iterator i;
    for (i = extraConfigs.begin(); i != extraConfigs.end(); i++) {
        configuration->addSetting(*i);
    }
    configuration->setValue("app_name", strip_to_base_filename(binary));

    // analysis configuration options
    for (size_t aidx=0; aidx < (size_t)TOTAL_ANALYSIS_COUNT; aidx++) {
        string tag = allAnalysisInfo[aidx].instance->getTag();

        if (configuration->hasValue(tag) && configuration->getValue(tag) == "yes") {

            // enabled in the default configuration
            fpinstAnalysisEnabled[aidx] = true;
            configuration->setValue("tag", tag);

            // special case analysis parameters
            if (tag == "sv_ptr") {
                assert(configuration->hasValue("sv_ptr_type"));
                configuration->setValue("tag", string("sv_ptr_") + configuration->getValue("sv_ptr_type"));
            } else if (tag == "sv_inp") {
                assert(configuration->hasValue("sv_inp_type"));
                configuration->setValue("tag", string("sv_inp_") + configuration->getValue("sv_inp_type"));
            } else if (tag == "r_prec") {
                assert(configuration->hasValue("r_prec_default_precision"));
                configuration->setValue("tag", string("r_prec_") + configuration->getValue("r_prec_default_precision"));
            }

        } else if (fpinstAnalysisEnabled[aidx]) {

            // enabled on the command line
            configuration->setValue(tag, "yes");
            configuration->setValue("tag", tag);

            // optional analysis parameter
            if (fpinstAnalysisParam[aidx] && strcmp("",fpinstAnalysisParam[aidx])!=0) {
                configuration->setValue(
                        string(allAnalysisInfo[aidx].fpinstParam),
                        string(fpinstAnalysisParam[aidx]));
                configuration->setValue("tag", allAnalysisInfo[aidx].instance->getTag()
                        + string("_") + string(fpinstAnalysisParam[aidx]));
            }
        }
    }

    if (logFile) {
        configuration->setValue("log_file", string(logFile));
    }
    if (logTag) {
        // override other auto-tags
        configuration->setValue("tag", string(logTag));
    }
    configuration->setValue("debug_file", process ? binary : outFile);
    if (mismatches) {
        configuration->setValue("enable_mismatch_detection", "yes");
    }
    if (cancellations) {
        configuration->setValue("enable_cancellation_detection", "yes");
    }
#if INCLUDE_DEBUG
    if (debugPrint) {
        configuration->setValue("enable_debug_print", "yes");
    }
#endif
    if (stwalk) {
        configuration->setValue("enable_stwalk", "yes");
    }
    if (reportAllGlobals) {
        configuration->setValue("report_all_globals", "yes");
    }
    if (profiling) {
        configuration->setValue("enable_profiling", "yes");
    }
    if (disableSampling) {
        configuration->setValue("enable_sampling", "no");
    }
    if (multicoreMode) {
        configuration->setValue("use_lock_prefix", "yes");
    }
}

void initializeActiveAnalyses() {
    for (size_t aidx=0; aidx < (size_t)TOTAL_ANALYSIS_COUNT; aidx++) {
        if (fpinstAnalysisEnabled[aidx]) {
            activeAnalyses.push_back(allAnalysisInfo[aidx].instance);
            allAnalysisInfo[aidx].instance->configure(configuration, mainDecoder, logfile, NULL);
        }
    }
}

bool isAnalysisEnabled(string tag) {
    for (size_t aidx=0; aidx < (size_t)TOTAL_ANALYSIS_COUNT; aidx++) {
        if (fpinstAnalysisEnabled[aidx] && allAnalysisInfo[aidx].instance->getTag() == tag) {
            return true;
        }
    }
    return false;
}

BPatch_function* getMutateeFunction(const char *name) {
    BPatch_Vector<BPatch_function *> funcs;
    mainImg->findFunction(name, funcs, true, true, true);
    assert(funcs.size() == 1);
    return funcs.at(0);
}

BPatch_function* getAnalysisFunction(const char *name) {
    // should this be searching libFPAnalysis instead of mainImg?
    return getMutateeFunction(name);
}

BPatch_module* findInitFiniModule()
{
    char nameBuffer[1024];
    BPatch_Vector<BPatch_module *> *modules = mainImg->getModules();
    BPatch_Vector<BPatch_module *>::iterator m;
    BPatch_Vector<BPatch_function *> initFiniFuncs;
    bool hasInit, hasFini, isShared, isDefault;
    for (m = modules->begin(); m != modules->end(); m++) {
        hasInit = hasFini = isShared = isDefault = false;
        (*m)->getName(nameBuffer, 1024);
        //printf("module: %s\n", nameBuffer);
        if (strcmp(nameBuffer,"DEFAULT_MODULE") == 0) {
            //printf("  DEFAULT MODULE\n");
            isDefault = true;
        }
        //((BPatch_image*)(*m)->getObjParent())->getProgramName(nameBuffer, 1024);
        //printf("  parent: %s\n", nameBuffer);
        if ((*m)->isSharedLib()) {
            //printf("  shared lib\n");
            isShared = true;
        }
        initFiniFuncs.clear();
        (*m)->findFunction("_init", initFiniFuncs, false);
        if (initFiniFuncs.size() > 0) {
            //printf("  _init\n");
            hasInit = true;
        }
        initFiniFuncs.clear();
        (*m)->findFunction("_fini", initFiniFuncs, false);
        if (initFiniFuncs.size() > 0) {
            //printf("  _fini\n");
            hasFini = true;
        }
        if (!isShared && hasInit && hasFini) {
            return *m;
        }
    }
    return NULL;
}

BPatch_constExpr* saveStringToBinary(const char *str, size_t nbytes)
{
    BPatch_constExpr* ptr = NULL;
    if (nbytes == 0) {
        // auto-detect
        nbytes = strlen(str) + 1;
    }
    /*
     *printf("  saving %lu-byte string \"", nbytes);
     *size_t i;
     *for (i=0; i<nbytes; i++) {
     *    printf("%c", str[i]);
     *}
     *printf("\"");
     */
    assert(nbytes < 2048);
    BPatch_variableExpr *strExpr = mainApp->malloc(nbytes);
    strExpr->writeValue(str, nbytes, false);
    //printf(" @ %p", strExpr->getBaseAddr());
    ptr = new BPatch_constExpr(strExpr->getBaseAddr());
    //printf("\n");
    return ptr;
}

Symbol *findLibFPMSymbol(string name) {
    assert(libFPAnalysis != NULL);

    Symtab *st = SymtabAPI::convert(libFPM);
    assert(st != NULL);

    std::vector<Symbol *> syms;
    st->findSymbol(syms, name, Symbol::ST_UNKNOWN,
            anyName, false, false, true);
    assert(!syms.empty());

    Symbol *sym = NULL;
    if (syms.size() > 1) {
        // prefer dynamic symbols
        for (unsigned i = 0; !sym && i < syms.size(); i++) {
            if (syms[i]->isInDynSymtab()) {
                sym = syms[i];
            }
        }
    }

    if (!sym) {
        sym = syms[0];
    }
    return sym;
}

string disassembleBlock(PatchBlock *block) {
    PatchBlock::Insns insns;
    PatchBlock::Insns::iterator j;
    Instruction::Ptr iptr;
    void *addr;
    unsigned char bytes[64];
    size_t nbytes, i;
    char buffer[64];
    string str("");

    block->getInsns(insns);
    for (j = insns.begin(); j != insns.end(); j++) {

        // get instruction bytes
        addr = (void*)((*j).first);
        iptr = (*j).second;
        nbytes = iptr->size();
        assert(nbytes <= 64);
        for (i=0; i<nbytes; i++) {
            bytes[i] = iptr->rawByte(i);
        }
        bytes[nbytes] = '\0';

        sprintf(buffer, "%08lx\t", (unsigned long)addr);
        str.append(buffer);
        str.append(FPDecoderXED::disassemble(bytes, nbytes));
        str.append("\n");
    }
    return str;
}

bool overwriteBlock(PatchBlock *block, unsigned char val) {
    ParseAPI::Block *b = block->block();
    Offset off = b->start();
    ParseAPI::SymtabCodeRegion *r = 
        dynamic_cast<ParseAPI::SymtabCodeRegion*>(b->region());
    if (r == NULL) return false;
    Offset region_off = (Offset)r->getPtrToInstruction(off) - 
                        (Offset)r->symRegion()->getPtrToRawData();
    bool success = false;
    while (off++ < b->end()) {
        success = r->symRegion()->patchData(region_off++, (void*)&val, 1);
        if (!success) return false;
    }
    return true;
}

// }}}

// {{{ snippet builders

BPatch_snippet* buildIncrementSnippet(const char *varname)
{
    BPatch_variableExpr *varExpr = mainImg->findVariable(varname);
    BPatch_snippet *valExpr = new BPatch_arithExpr(BPatch_plus,
            *varExpr, BPatch_constExpr(1));
    BPatch_snippet *incExpr = new BPatch_arithExpr(BPatch_assign,
            *varExpr, *valExpr);
    return incExpr;
}

Snippet::Ptr buildDefaultPreInstrumentation(FPAnalysis *analysis, FPSemantics *inst)
{
    long iidx = inst->getIndex();
    long aidx = _INST_get_analysis_id(analysis);
    BPatch_Vector<BPatch_snippet*> *args = new BPatch_Vector<BPatch_snippet*>();
    args->push_back(new BPatch_constExpr(aidx));
    args->push_back(new BPatch_constExpr(iidx));
    return PatchAPI::convert(new BPatch_funcCallExpr(*handlePreFunc, *args));
}

Snippet::Ptr buildDefaultPostInstrumentation(FPAnalysis *analysis, FPSemantics *inst)
{
    long iidx = inst->getIndex();
    long aidx = _INST_get_analysis_id(analysis);
    BPatch_Vector<BPatch_snippet*> *args = new BPatch_Vector<BPatch_snippet*>();
    args->push_back(new BPatch_constExpr(aidx));
    args->push_back(new BPatch_constExpr(iidx));
    return PatchAPI::convert(new BPatch_funcCallExpr(*handlePostFunc, *args));
}

Snippet::Ptr buildDefaultReplacementCode(FPAnalysis *analysis, FPSemantics *inst)
{
    long iidx = inst->getIndex();
    long aidx = _INST_get_analysis_id(analysis);
    BPatch_Vector<BPatch_snippet*> *args = new BPatch_Vector<BPatch_snippet*>();
    args->push_back(new BPatch_constExpr(aidx));
    args->push_back(new BPatch_constExpr(iidx));
    return PatchAPI::convert(new BPatch_funcCallExpr(*handleReplFunc, *args));
}

void buildRegHandlers(FPSemantics *inst, vector<Snippet::Ptr> &handlers)
{
    set<FPRegister> regs;
    set<FPRegister>::iterator i;
    BPatch_snippet *assignExpr = NULL, *derefExpr = NULL;
    BPatch_variableExpr *regPtrExpr = NULL;
    BPatch_snippet *regExpr = NULL;

    // {{{ initialize arguments for the value of registers EAX-EDX, EBP and ESP
    if (eaxExpr == NULL) {
        vector<BPatch_register> tempRegs;
        mainApp->getRegisters(tempRegs);
        //printf("%d register(s):\n", (int)tempRegs.size());
        for (unsigned k = 0; k < tempRegs.size(); k++) {
            //printf("%s \n", tempRegs[k].name().c_str());
            /*if (tempRegs[k].name() == eflagsRegName)
                eflagsExpr = new BPatch_registerExpr(tempRegs[k]);
            else*/ if (tempRegs[k].name() == eaxRegName)
                eaxExpr = new BPatch_registerExpr(tempRegs[k]);
            else if (tempRegs[k].name() == ebxRegName)
                ebxExpr = new BPatch_registerExpr(tempRegs[k]);
            else if (tempRegs[k].name() == ecxRegName)
                ecxExpr = new BPatch_registerExpr(tempRegs[k]);
            else if (tempRegs[k].name() == edxRegName)
                edxExpr = new BPatch_registerExpr(tempRegs[k]);
            else if (tempRegs[k].name() == espRegName)
                espExpr = new BPatch_registerExpr(tempRegs[k]);
            else if (tempRegs[k].name() == ebpRegName)
                ebpExpr = new BPatch_registerExpr(tempRegs[k]);
            else if (tempRegs[k].name() == esiRegName)
                esiExpr = new BPatch_registerExpr(tempRegs[k]);
            else if (tempRegs[k].name() == ediRegName)
                ediExpr = new BPatch_registerExpr(tempRegs[k]);
#ifdef __X86_64__
            else if (tempRegs[k].name() == r8RegName)
                r8Expr = new BPatch_registerExpr(tempRegs[k]);
            else if (tempRegs[k].name() == r9RegName)
                r9Expr = new BPatch_registerExpr(tempRegs[k]);
            else if (tempRegs[k].name() == r10RegName)
                r10Expr = new BPatch_registerExpr(tempRegs[k]);
            else if (tempRegs[k].name() == r11RegName)
                r11Expr = new BPatch_registerExpr(tempRegs[k]);
            else if (tempRegs[k].name() == r12RegName)
                r12Expr = new BPatch_registerExpr(tempRegs[k]);
            else if (tempRegs[k].name() == r13RegName)
                r13Expr = new BPatch_registerExpr(tempRegs[k]);
            else if (tempRegs[k].name() == r14RegName)
                r14Expr = new BPatch_registerExpr(tempRegs[k]);
            else if (tempRegs[k].name() == r15RegName)
                r15Expr = new BPatch_registerExpr(tempRegs[k]);
#endif
        }
        /*if (eflagsExpr == NULL) {
            printf("ERROR: can't find register %s!\n", eflagsRegName.c_str());
            exit(EXIT_FAILURE);
        }*/
        if (eaxExpr == NULL) {
            printf("ERROR: can't find register %s!\n", eaxRegName.c_str());
            exit(EXIT_FAILURE);
        }
        if (ebxExpr == NULL) {
            printf("ERROR: can't find register %s!\n", ebxRegName.c_str());
            exit(EXIT_FAILURE);
        }
        if (ecxExpr == NULL) {
            printf("ERROR: can't find register %s!\n", ecxRegName.c_str());
            exit(EXIT_FAILURE);
        }
        if (edxExpr == NULL) {
            printf("ERROR: can't find register %s!\n", edxRegName.c_str());
            exit(EXIT_FAILURE);
        }
        if (espExpr == NULL) {
            printf("ERROR: can't find register %s!\n", espRegName.c_str());
            exit(EXIT_FAILURE);
        }
        if (ebpExpr == NULL) {
            printf("ERROR: can't find register %s!\n", ebpRegName.c_str());
            exit(EXIT_FAILURE);
        }
        if (esiExpr == NULL) {
            printf("ERROR: can't find register %s!\n", esiRegName.c_str());
            exit(EXIT_FAILURE);
        }
        if (ediExpr == NULL) {
            printf("ERROR: can't find register %s!\n", ediRegName.c_str());
            exit(EXIT_FAILURE);
        }
#ifdef __X86_64__
        if (r8Expr == NULL) {
            printf("ERROR: can't find register %s!\n", r8RegName.c_str());
            exit(EXIT_FAILURE);
        }
        if (r9Expr == NULL) {
            printf("ERROR: can't find register %s!\n", r9RegName.c_str());
            exit(EXIT_FAILURE);
        }
        if (r10Expr == NULL) {
            printf("ERROR: can't find register %s!\n", r10RegName.c_str());
            exit(EXIT_FAILURE);
        }
        if (r11Expr == NULL) {
            printf("ERROR: can't find register %s!\n", r11RegName.c_str());
            exit(EXIT_FAILURE);
        }
        if (r12Expr == NULL) {
            printf("ERROR: can't find register %s!\n", r12RegName.c_str());
            exit(EXIT_FAILURE);
        }
        if (r13Expr == NULL) {
            printf("ERROR: can't find register %s!\n", r13RegName.c_str());
            exit(EXIT_FAILURE);
        }
        if (r14Expr == NULL) {
            printf("ERROR: can't find register %s!\n", r14RegName.c_str());
            exit(EXIT_FAILURE);
        }
        if (r15Expr == NULL) {
            printf("ERROR: can't find register %s!\n", r15RegName.c_str());
            exit(EXIT_FAILURE);
        }
#endif
    } // }}}

    // {{{ initialize register context pointers
    if (eaxPtrExpr == NULL) {
        eipPtrExpr = mainImg->findVariable("_INST_reg_eip");
        //eflagsPtrExpr = mainImg->findVariable("_INST_reg_eflags");
        eaxPtrExpr = mainImg->findVariable("_INST_reg_eax");
        ebxPtrExpr = mainImg->findVariable("_INST_reg_ebx");
        ecxPtrExpr = mainImg->findVariable("_INST_reg_ecx");
        edxPtrExpr = mainImg->findVariable("_INST_reg_edx");
        ebpPtrExpr = mainImg->findVariable("_INST_reg_ebp");
        espPtrExpr = mainImg->findVariable("_INST_reg_esp");
        esiPtrExpr = mainImg->findVariable("_INST_reg_esi");
        ediPtrExpr = mainImg->findVariable("_INST_reg_edi");
        r8PtrExpr  = mainImg->findVariable("_INST_reg_r8");
        r9PtrExpr  = mainImg->findVariable("_INST_reg_r9");
        r10PtrExpr = mainImg->findVariable("_INST_reg_r10");
        r11PtrExpr = mainImg->findVariable("_INST_reg_r11");
        r12PtrExpr = mainImg->findVariable("_INST_reg_r12");
        r13PtrExpr = mainImg->findVariable("_INST_reg_r13");
        r14PtrExpr = mainImg->findVariable("_INST_reg_r14");
        r15PtrExpr = mainImg->findVariable("_INST_reg_r15");
        if (eipPtrExpr == NULL) {
            printf("ERROR: can't find context variable _INST_reg_eip!\n");
            exit(EXIT_FAILURE);
        }
        /*if (eflagsPtrExpr == NULL) {
            printf("ERROR: can't find context variable _INST_reg_eflags!\n");
            exit(EXIT_FAILURE);
        }*/
        if (eaxPtrExpr == NULL) {
            printf("ERROR: can't find context variable _INST_reg_eax!\n");
            exit(EXIT_FAILURE);
        }
        if (ebxPtrExpr == NULL) {
            printf("ERROR: can't find context variable _INST_reg_ebx!\n");
            exit(EXIT_FAILURE);
        }
        if (ecxPtrExpr == NULL) {
            printf("ERROR: can't find context variable _INST_reg_ecx!\n");
            exit(EXIT_FAILURE);
        }
        if (edxPtrExpr == NULL) {
            printf("ERROR: can't find context variable _INST_reg_edx!\n");
            exit(EXIT_FAILURE);
        }
        if (ebpPtrExpr == NULL) {
            printf("ERROR: can't find context variable _INST_reg_ebp!\n");
            exit(EXIT_FAILURE);
        }
        if (espPtrExpr == NULL) {
            printf("ERROR: can't find context variable _INST_reg_esp!\n");
            exit(EXIT_FAILURE);
        }
        if (esiPtrExpr == NULL) {
            printf("ERROR: can't find context variable _INST_reg_esi!\n");
            exit(EXIT_FAILURE);
        }
        if (ediPtrExpr == NULL) {
            printf("ERROR: can't find context variable _INST_reg_edi!\n");
            exit(EXIT_FAILURE);
        }
#ifdef __X86_64__
        if (r8PtrExpr == NULL) {
            printf("ERROR: can't find context variable _INST_reg_r8!\n");
            exit(EXIT_FAILURE);
        }
        if (r9PtrExpr == NULL) {
            printf("ERROR: can't find context variable _INST_reg_r9!\n");
            exit(EXIT_FAILURE);
        }
        if (r10PtrExpr == NULL) {
            printf("ERROR: can't find context variable _INST_reg_r10!\n");
            exit(EXIT_FAILURE);
        }
        if (r11PtrExpr == NULL) {
            printf("ERROR: can't find context variable _INST_reg_r11!\n");
            exit(EXIT_FAILURE);
        }
        if (r12PtrExpr == NULL) {
            printf("ERROR: can't find context variable _INST_reg_r12!\n");
            exit(EXIT_FAILURE);
        }
        if (r13PtrExpr == NULL) {
            printf("ERROR: can't find context variable _INST_reg_r13!\n");
            exit(EXIT_FAILURE);
        }
        if (r14PtrExpr == NULL) {
            printf("ERROR: can't find context variable _INST_reg_r14!\n");
            exit(EXIT_FAILURE);
        }
        if (r15PtrExpr == NULL) {
            printf("ERROR: can't find context variable _INST_reg_r15!\n");
            exit(EXIT_FAILURE);
        }
#endif
    } // }}}

    // find all needed registers
    inst->getNeededRegisters(regs);

    // {{{ build read assignments
    for (i=regs.begin(); i!=regs.end(); i++) {
        regPtrExpr = NULL;
        regExpr = NULL;
        switch (*i) {
            // value of IP should always be the address of original instruction
            // since variable offsets will still be based on that
            case REG_EIP: regPtrExpr = eipPtrExpr;
                          regExpr = new BPatch_constExpr((unsigned long)inst->getAddress() + 
                                  (unsigned long)inst->getNumBytes()); 
                          break;
            case REG_EFLAGS: //regPtrExpr = eflagsPtrExpr; regExpr = eflagsExpr;
                             //assert(!"Reading from EFLAGS");    // TODO: remove
                             break;
            case REG_EAX: regPtrExpr = eaxPtrExpr; regExpr = eaxExpr; break;
            case REG_EBX: regPtrExpr = ebxPtrExpr; regExpr = ebxExpr; break;
            case REG_ECX: regPtrExpr = ecxPtrExpr; regExpr = ecxExpr; break;
            case REG_EDX: regPtrExpr = edxPtrExpr; regExpr = edxExpr; break;
            case REG_EBP: regPtrExpr = ebpPtrExpr; regExpr = ebpExpr; break;
            case REG_ESP: regPtrExpr = espPtrExpr; regExpr = espExpr; break;
            case REG_ESI: regPtrExpr = esiPtrExpr; regExpr = esiExpr; break;
            case REG_EDI: regPtrExpr = ediPtrExpr; regExpr = ediExpr; break;
            case REG_E8:  regPtrExpr = r8PtrExpr;  regExpr = r8Expr;  break;
            case REG_E9:  regPtrExpr = r9PtrExpr;  regExpr = r9Expr;  break;
            case REG_E10: regPtrExpr = r10PtrExpr; regExpr = r10Expr; break;
            case REG_E11: regPtrExpr = r11PtrExpr; regExpr = r11Expr; break;
            case REG_E12: regPtrExpr = r12PtrExpr; regExpr = r12Expr; break;
            case REG_E13: regPtrExpr = r13PtrExpr; regExpr = r13Expr; break;
            case REG_E14: regPtrExpr = r14PtrExpr; regExpr = r14Expr; break;
            case REG_E15: regPtrExpr = r15PtrExpr; regExpr = r15Expr; break;
            default: break;
        }
        if (regPtrExpr != NULL && regExpr != NULL) {
            derefExpr = new BPatch_arithExpr(BPatch_deref,
                    *regPtrExpr);
            assignExpr = new BPatch_arithExpr(BPatch_assign,
                    *derefExpr, *regExpr);

            /*
             *BPatch_type *t1 = regExpr->getType();
             *BPatch_type *t2 = derefExpr->getType();
             *cout << " read: assign ";
             *if (t1) { cout << t1->getName(); }
             *else { cout << " (unknown) "; }
             *cout << " to ";
             *if (t2) { cout << t2->getName(); }
             *else { cout << " (unknown) "; }
             *cout << endl;
             */

            handlers.insert(handlers.begin(), PatchAPI::convert(assignExpr));
            //printf(" adding read assignment for %s\n", FPContext::FPReg2Str(*i).c_str());
        } else {
            //printf("Couldn't build register read assignment expression for %s!\n", FPContext::FPReg2Str(*i).c_str());
            //exit(EXIT_FAILURE);
        }
    } // }}}

    // find all modified registers
    regs.clear();
    inst->getModifiedRegisters(regs);

    // {{{ build write assignments
    for (i=regs.begin(); i!=regs.end(); i++) {
        regPtrExpr = NULL;
        regExpr = NULL;
        switch (*i) {
            // DON'T ALLOW WRITES TO EIP
            case REG_EFLAGS: //regPtrExpr = eflagsPtrExpr; regExpr = eflagsExpr; 
                             //assert(!"Writing to EFLAGS");  // TODO: remove
                             break;
            case REG_EAX: regPtrExpr = eaxPtrExpr; regExpr = eaxExpr; break;
            case REG_EBX: regPtrExpr = ebxPtrExpr; regExpr = ebxExpr; break;
            case REG_ECX: regPtrExpr = ecxPtrExpr; regExpr = ecxExpr; break;
            case REG_EDX: regPtrExpr = edxPtrExpr; regExpr = edxExpr; break;
            case REG_EBP: regPtrExpr = ebpPtrExpr; regExpr = ebpExpr; break;
            case REG_ESP: regPtrExpr = espPtrExpr; regExpr = espExpr; break;
            case REG_ESI: regPtrExpr = esiPtrExpr; regExpr = esiExpr; break;
            case REG_EDI: regPtrExpr = ediPtrExpr; regExpr = ediExpr; break;
            case REG_E8:  regPtrExpr = r8PtrExpr;  regExpr = r8Expr;  break;
            case REG_E9:  regPtrExpr = r9PtrExpr;  regExpr = r9Expr;  break;
            case REG_E10: regPtrExpr = r10PtrExpr; regExpr = r10Expr; break;
            case REG_E11: regPtrExpr = r11PtrExpr; regExpr = r11Expr; break;
            case REG_E12: regPtrExpr = r12PtrExpr; regExpr = r12Expr; break;
            case REG_E13: regPtrExpr = r13PtrExpr; regExpr = r13Expr; break;
            case REG_E14: regPtrExpr = r14PtrExpr; regExpr = r14Expr; break;
            case REG_E15: regPtrExpr = r15PtrExpr; regExpr = r15Expr; break;
            default: break;
        }
        if (regPtrExpr != NULL && regExpr != NULL) {
            derefExpr = new BPatch_arithExpr(BPatch_deref,
                    *regPtrExpr);
            assignExpr = new BPatch_arithExpr(BPatch_assign,
                    *regExpr, *derefExpr);

            /*
             *BPatch_type *t1 = derefExpr->getType();
             *BPatch_type *t2 = regExpr->getType();
             *cout << " write: assign ";
             *if (t1) { cout << t1->getName(); }
             *else { cout << " (unknown) "; }
             *cout << " to ";
             *if (t2) { cout << t2->getName(); }
             *else { cout << " (unknown) "; }
             *cout << endl;
             */

            handlers.push_back(PatchAPI::convert(assignExpr));
            //printf(" adding write assignment for %s\n", FPContext::FPReg2Str(*i).c_str());
        } else {
            //printf("Couldn't build register write assignment expression for %s!\n", FPContext::FPReg2Str(*i).c_str());
            //exit(EXIT_FAILURE);
        }
    } // }}}
}

Snippet::Ptr buildUnsupportedInstHandler()
{
    BPatch_Vector<BPatch_snippet*> *uiArgs = new BPatch_Vector<BPatch_snippet*>();
    uiArgs->push_back(new BPatch_constExpr(iidx));
    return PatchAPI::convert(new BPatch_funcCallExpr(*handleUnspFunc, *uiArgs));

}

// }}}

// {{{ function replacements

// BPatch_addressSpace::replaceFunction version
void replaceFunctionCalls(const char* oldFuncName, const char* newFuncName)
{
    BPatch_Vector<BPatch_function*> funcs;
    BPatch_function *oldFunc;
    BPatch_function *newFunc;

    //cout << " replacing calls to \"" << oldFuncName << "\" with calls to \"" << newFuncName << "\"" << endl;

    // find original function
    mainImg->findFunction(oldFuncName, funcs, false);
    if (!funcs.size() || !funcs[0]) {
        // the function to be replaced does not exist; ignore it
        //cout << "WARNING: cannot find function \"" << oldFuncName << "\"" << endl;
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

    total_replacements++;

    stringstream ss;
    ss.clear();
    ss.str("");
    ss << "Replaced function \"" << oldFuncName << "\" with \"" << newFuncName << "\"";
    logfile->addMessage(STATUS, 0, ss.str(), "", "", NULL);
}

// OLD CODE: uses BPatch_funcJumpExpr
//
#if 0
void replaceFunctionCalls(const char* oldFuncName, const char* newFuncName)
{
    BPatch_Vector<BPatch_function*> funcs;
    BPatch_function *oldFunc;
    BPatch_function *newFunc;
    BPatch_function *nullFunc;

    //cout << " replacing calls to \"" << oldFuncName << "\" with calls to \"" << newFuncName << "\"" << endl;

    // find original function
    mainImg->findFunction(oldFuncName, funcs, false);
    if (!funcs.size() || !funcs[0]) {
        // the function to be replaced does not exist; ignore it
        cout << "WARNING: cannot find function \"" << oldFuncName << "\" for replacement" << endl;
        return;
    }
    /*
     *cout << "= searching for " << oldFuncName << ":  ";
     *for (int i=0; i<funcs.size(); i++) {
     *    funcs[i]->getTypedName(name, BUFFER_STRING_LEN);
     *    cout << name;
     *    funcs[i]->getModule()->getName(name, BUFFER_STRING_LEN);
     *    cout << " [" << name << "]  ";
     *    if (strstr(name,"libm") > 0) {
     *        oldFunc = funcs[i];
     *        cout << "*";
     *    }
     *}
     *cout << endl;
     */
    oldFunc = funcs[0];

    // find new function
    funcs.clear();
    mainImg->findFunction(newFuncName, funcs, false);
    if (!funcs.size()) {
        cout << "WARNING: cannot find function \"" << newFuncName << "\" for replacement" << endl;
        return;
    }
    newFunc = funcs[0];

    // find null function
    funcs.clear();
    mainImg->findFunction("_INST_null", funcs, false);
    if (!funcs.size()) {
        cout << "WARNING: cannot find function \"_INST_null\"" << endl;
        return;
    }
    nullFunc = funcs[0];

    // debug output
    /*
	 *char name[BUFFER_STRING_LEN];
     *oldFunc->getTypedName(name, BUFFER_STRING_LEN);
     *cout << "replacing function \"" << name << "\" [";
     *oldFunc->getModule()->getName(name, BUFFER_STRING_LEN);
     *cout << name << "] with \"";
     *newFunc->getTypedName(name, BUFFER_STRING_LEN);
     *cout << name << "\"" << endl;
     */

    // prevent infinite self-recursive loop
    if (strcmp(oldFuncName,newFuncName)==0 || oldFunc == newFunc) {
        // replacing function with a call to self; ignore it
        return;
    }

    // get entry point(s) of original function
    BPatch_Vector<BPatch_point*> *points = oldFunc->findPoint(BPatch_entry);
    if (!points || !points->size()) {
        cout << "WARNING: cannot instrument function \"" << oldFuncName << "\" for replacement" << endl;
        return;
    }

    // build "replace if _INST_status is inactive" snippet
    BPatch_variableExpr* flagVar = mainImg->findVariable("_INST_status");
    if (!flagVar) {
        cout << "ERROR: could not find instrumentation flag \"_INST_status\"" << endl;
        return;
    }
    BPatch_variableExpr* valVar = mainImg->findVariable("_INST_INACTIVE");
    if (!valVar) {
        cout << "ERROR: could not find instrumentation flag value \"_INST_INACTIVE\"" << endl;
        return;
    }
    BPatch_boolExpr flagVarCheck(BPatch_eq, *flagVar, *valVar);

    BPatch_funcJumpExpr newJump(*newFunc);
    BPatch_funcJumpExpr oldJump(*oldFunc);
    BPatch_funcJumpExpr nullJump(*nullFunc);
    
    BPatch_ifExpr instCheck(flagVarCheck, newJump);

    BPatch_Vector<BPatch_snippet*> snippets;
    snippets.push_back(&instCheck);
    //snippets.push_back(&oldJump);
    //snippets.push_back(&nullJump); // was enabled
    BPatch_sequence code(snippets);

    // insert code
    bool old_flag = bpatch->isTrampRecursive();
    bpatch->setTrampRecursive(true);
    mainApp->insertSnippet(instCheck, *points, BPatch_callBefore);
    bpatch->setTrampRecursive(old_flag);

    //bool old_flag = bpatch->isTrampRecursive();
    //bpatch->setTrampRecursive(false);
    //mainApp->replaceFunction(*oldFunc, *newFunc);
    //bpatch->setTrampRecursive(old_flag);
    
    total_replacements++;

    stringstream ss;
    ss.clear();
    ss.str("");
    ss << "Replaced function \"" << oldFuncName << "\" with \"" << newFuncName << "\"";
    logfile->addMessage(STATUS, 0, ss.str(), "", "", NULL);
}
#endif

void wrapFunction(const char* oldFuncName, const char* newFuncName)
{
    BPatch_Vector<BPatch_function*> funcs;
    BPatch_function *oldFunc;
    BPatch_function *newFunc;

    //cout << " wrapping calls to \"" << oldFuncName << "\" with calls to \"" << newFuncName << "\"";

    // find original function
    mainImg->findFunction(oldFuncName, funcs, false);
    if (!funcs.size() || !funcs[0]) {
        // the function to be replaced does not exist; ignore it
        //cout << "WARNING: cannot find function \"" << oldFuncName << "\"" << endl;
        //cout << " - not found" << endl;
        return;
    }
    oldFunc = funcs[0];

    //char buffer[50];
    //oldFunc->getModule()->getName(buffer, 50);
    //cout << " in library " << buffer << endl;

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

    Symbol *cloneSym = findLibFPMSymbol("_ORIG_" + string(oldFuncName));
    assert(cloneSym != NULL);
    mainApp->wrapFunction(oldFunc, newFunc, cloneSym);
    //cout << "Wrapped function \"" << oldFuncName << "\""
         //<< " with \"" << newFuncName << "\""
         //<< " (old renamed to \"" << cloneSym->getPrettyName() << "\")"
         //<< endl;

    total_replacements++;

    stringstream ss;
    ss.clear();
    ss.str("");
    ss << "Wrapped function \"" << oldFuncName << "\" with \"" << newFuncName << "\"";
    logfile->addMessage(STATUS, 0, ss.str(), "", "", NULL);
}

void replaceLibmFunctions()
{
    if (isAnalysisEnabled("sv_ptr") || isAnalysisEnabled("sv_inp")) {

        wrapFunction("fabs",  "_INST_fabs");
        wrapFunction("ceil",  "_INST_ceil");
        wrapFunction("erf",   "_INST_erf");
        wrapFunction("erfc",  "_INST_erfc");
        wrapFunction("exp",   "_INST_exp");
        wrapFunction("exp2",  "_INST_exp2");
        wrapFunction("floor", "_INST_floor");
        wrapFunction("log",   "_INST_log");
        wrapFunction("logb", "_INST_logb");
        wrapFunction("log10", "_INST_log10");
        wrapFunction("sqrt",  "_INST_sqrt");
        wrapFunction("cbrt",  "_INST_cbrt");
        wrapFunction("trunc", "_INST_trunc");

        wrapFunction("sin",   "_INST_sin");
        wrapFunction("cos",   "_INST_cos");
        wrapFunction("tan",   "_INST_tan");
        wrapFunction("asin",  "_INST_asin");
        wrapFunction("acos",  "_INST_acos");
        wrapFunction("atan",  "_INST_atan");
        wrapFunction("sinh",  "_INST_sinh");
        wrapFunction("cosh",  "_INST_cosh");
        wrapFunction("tanh",  "_INST_tanh");
        wrapFunction("asinh", "_INST_asinh");
        wrapFunction("acosh", "_INST_acosh");
        wrapFunction("atanh", "_INST_atanh");

        wrapFunction("atan2", "_INST_atan2");
        wrapFunction("fmod",  "_INST_fmod");
        wrapFunction("pow",   "_INST_pow");
        
        wrapFunction("fpclassify", "_INST_fpclassify");
        wrapFunction("isnormal",   "_INST_isnormal");

        wrapFunction("sincos", "_INST_sincos");

        // these are actually in libc (at least on Ubuntu),
        // so we need to handle them separately
        //
        //wrapFunction("copysign", "_INST_copysign");
        //wrapFunction("modf", "_INST_modf");
        //wrapFunction("ldexp", "_INST_ldexp");
        //wrapFunction("frexp", "_INST_frexp");
        //wrapFunction("isfinite",   "_INST_isfinite");
        //wrapFunction("finite",     "_INST_finite");
        //wrapFunction("isnan",      "_INST_isnan");
        //wrapFunction("isinf",      "_INST_isinf");
        
    }
}

// }}}

// {{{ application, function, basic block, and instruction instrumenters

void buildReplacement(void *addr, FPSemantics *inst, PatchBlock *block, FPAnalysis *analysis)
{
    // build snippet
    bool needsRegisters = false;
    bool success = true;
    Snippet::Ptr handler = analysis->buildReplacementCode(inst, mainApp, needsRegisters);
    if (!handler) {
        handler = buildDefaultReplacementCode(analysis, inst);
    }
    assert(!needsRegisters);    // not currently supported

    // CFG surgery (remove the old instruction and insert the new snippet)
    //
    //

    PatchBlock *insnBlock = block;   // block with old instruction
    void *preSplitAddr = addr;
    void *postSplitAddr = (void*)((unsigned long)preSplitAddr + inst->getNumBytes());

    // split before instruction
    if ((unsigned long)preSplitAddr > (unsigned long)insnBlock->start()) {
        if (patchAPI_debug) {
            printf("        splitting block [%p-%p] at %p\n",
                    (void*)insnBlock->start(), (void*)insnBlock->end(), preSplitAddr);
        }
        insnBlock = PatchModifier::split(insnBlock, (Address)preSplitAddr);
        if (!insnBlock) {
            printf("ERROR: could not split block [%p-%p] at %p\n",
                    (void*)block->start(), (void*)block->end(), preSplitAddr);
            assert(0);
        }
    } else {
        // instruction is at the beginning of a block; no need to split
        // pre-block
        if (patchAPI_debug) {
            printf("        insn is at beginning of block\n");
        }
        insnBlock = block;
    }

    // split after instruction
    PatchBlock *postBlock = NULL;
    if ((unsigned long)postSplitAddr < (unsigned long)insnBlock->end()) {
        if (patchAPI_debug) {
            printf("        splitting block [%p-%p] at %p\n",
                    (void*)insnBlock->start(), (void*)insnBlock->end(), postSplitAddr);
        }
        postBlock = PatchModifier::split(insnBlock, (Address)postSplitAddr);
        if (!postBlock) {
            printf("ERROR: could not split block [%p-%p] at %p\n",
                    (void*)insnBlock->start(), (void*)block->end(), postSplitAddr);
            assert(0);
        }
    } else {
        // instruction is at the end of a block; no need to split
        // post-block
        if (patchAPI_debug) {
            printf("        insn is at end of block\n");
        }
        assert(block->targets().size() == 1);
        PatchEdge *postEdge = *(insnBlock->targets().begin());
        assert(postEdge->type() == ParseAPI::FALLTHROUGH);
        postBlock = postEdge->trg();
    }

    // insert new code
    InsertedCode::Ptr icode = PatchModifier::insert(block->object(), handler, NULL);
    //InsertedCode::Ptr icode = PatchModifier::insert(block->object(), handler, prePoint);
    //InsertedCode::Ptr icode = PatchModifier::insert(block->object(), handler,
           //mainMgr->findPoint(Location::Block(instBlock), Point::BlockEntry, true));
    assert(icode->blocks().size() >= 1);
    PatchBlock *newBlock = icode->entry();
    assert(newBlock != NULL);

    vector<PatchEdge*> edges;
    vector<PatchEdge*>::const_iterator i;
    vector<PatchEdge*>::iterator j;
    bool keepInsnBlock = false;

    // grab all incoming edges to insnBlock
    edges.clear();
    for (i = insnBlock->sources().begin(); i != insnBlock->sources().end(); i++) {
        edges.push_back(*i);

        if (patchAPI_debug) {
            printf("        checking incoming edge of type %s\n", ParseAPI::format((*i)->type()).c_str());
        }

        // we can only redirect these types of edges
        if (!((*i)->type() == ParseAPI::COND_TAKEN || 
              (*i)->type() == ParseAPI::COND_NOT_TAKEN ||
              (*i)->type() == ParseAPI::DIRECT ||
              (*i)->type() == ParseAPI::FALLTHROUGH ||
              (*i)->type() == ParseAPI::CALL_FT)) {
            keepInsnBlock = true;
        }
    }

    if (keepInsnBlock) {

        // overwrite insnBlock with nops
        if (patchAPI_debug) {
            printf("        overwriting block [%p-%p]\n",
                    (void*)insnBlock->start(), (void*)insnBlock->end());
        }
        success = overwriteBlock(insnBlock, 0x90);    // nop
        assert(success);

        // redirect from insnBlock to newBlock
        edges.clear();
        for (i = insnBlock->targets().begin(); i != insnBlock->targets().end(); i++) {
            edges.push_back(*i);
        }
        for (j = edges.begin(); j != edges.end(); j++) {
            if (patchAPI_debug) {
                printf("        redirecting insn outgoing edge [%p-%p] -> [%p-%p] to [%p-%p]\n",
                       (void*)((*j)->src()->start()), (void*)((*j)->src()->end()),
                       (void*)((*j)->trg()->start()), (void*)((*j)->trg()->end()),
                       (void*)(newBlock->start()), (void*)(newBlock->end()));
            }
            success = PatchModifier::redirect(*j, newBlock);
            assert(success);
        }

    } else {

        // redirect from src/pre to newBlock (skip insnBlock)
        for (j = edges.begin(); j != edges.end(); j++) {
            if (patchAPI_debug) {
                printf("        redirecting incoming edge [%p-%p] -> [%p-%p] of type %s to [%p-%p]\n",
                       (void*)((*j)->src()->start()), (void*)((*j)->src()->end()),
                       (void*)((*j)->trg()->start()), (void*)((*j)->trg()->end()),
                       ParseAPI::format((*j)->type()).c_str(),
                       (void*)(newBlock->start()), (void*)(newBlock->end()));
            }
            success = PatchModifier::redirect(*j, newBlock);
            assert(success);
        }
    }

    // redirect icode's exit to postBlock
    // (should only be one of them)
    assert(icode->exits().size() == 1);
    if (patchAPI_debug) {
        printf("        redirecting outgoing edge [%p-%p] -> [%p-%p] of type %s to [%p-%p]\n",
               (void*)((*icode->exits().begin())->src()->start()),
               (void*)((*icode->exits().begin())->src()->end()),
               (void*)((*icode->exits().begin())->trg()->start()),
               (void*)((*icode->exits().begin())->trg()->end()),
               ParseAPI::format((*icode->exits().begin())->type()).c_str(),
               (void*)(postBlock->start()), (void*)(postBlock->end()));
    }
    success = PatchModifier::redirect(*icode->exits().begin(), postBlock);
    assert(success);

    // should be single entry and exit now
    PatchBlock *newEntry = icode->entry();
    PatchBlock *newExit = (*icode->exits().begin())->src();

    if (patchAPI_debug) {
        // debug output
        printf("    new sequence: => ");
        if ((unsigned long)preSplitAddr > (unsigned long)block->start()) {
           printf("[%p-%p] -> ", (void*)block->start(),
                                 (void*)block->end());
        } else if (keepInsnBlock) {
           printf("[%p-%p] -> ", (void*)insnBlock->start(),
                                 (void*)insnBlock->end());
        }
        printf("{ [%p-%p] ... [%p-%p] } ", 
              (void*)newEntry->start(), (void*)newEntry->end(),
              (void*)newExit->start(),  (void*)newExit->end());
        printf(" -> [%p-%p] ", (void*)postBlock->start(),
                              (void*)postBlock->end());
        printf("=> \n");
    }

    // disassemble for log
    set<PatchBlock*>::iterator k;
    string disassembly("");
    disassembly.append(disassembleBlock(newEntry));
    disassembly.append("\n");
    for (k = icode->blocks().begin(); k != icode->blocks().end(); k++) {
        if (((*k) != newEntry) && ((*k) != newExit)) {
            disassembly.append(disassembleBlock(*k));
            disassembly.append("\n");
        }
    }
    disassembly.append(disassembleBlock(newExit));
    disassembly.append("\n");

    // debug output
    logfile->addMessage(STATUS, 0, "Inserted " + analysis->getTag() + " replacement instrumentation.",
            disassembly, "", inst);
}

void buildPreInstrumentation(FPSemantics *inst, FPAnalysis *analysis,
        vector<Snippet::Ptr> &preHandlers, bool &preNeedsRegisters)
{
    // build snippet
    bool needsRegisters = false;
    Snippet::Ptr handler = analysis->buildPreInstrumentation(inst, mainApp, needsRegisters);
    if (!handler) {
        handler = buildDefaultPreInstrumentation(analysis, inst);
    }
    preHandlers.push_back(handler);
    preNeedsRegisters |= needsRegisters;

    // debug output
    logfile->addMessage(STATUS, 0, "Inserted " + analysis->getTag() + " pre-instrumentation.",
            "", "", inst);
}

void buildPostInstrumentation(FPSemantics *inst, FPAnalysis *analysis,
        vector<Snippet::Ptr> &postHandlers, bool &postNeedsRegisters)
{
    // build snippet
    bool needsRegisters = false;
    Snippet::Ptr handler = analysis->buildPostInstrumentation(inst, mainApp, needsRegisters);
    if (!handler) {
        handler = buildDefaultPostInstrumentation(analysis, inst);
    }
    postHandlers.push_back(handler);
    postNeedsRegisters |= needsRegisters;

    // debug output
    logfile->addMessage(STATUS, 0, "Inserted " + analysis->getTag() + " post-instrumentation.",
            "", "", inst);
}

bool buildInstrumentation(void* addr, FPSemantics *inst, PatchFunction *func, PatchBlock *block)
{
    vector<Snippet::Ptr> preHandlers;
    vector<Snippet::Ptr> postHandlers;
    bool preNeedsRegisters = false;
    bool postNeedsRegisters = false;
    bool replaced = false;

    if (listFuncs) {
        printf("Instrumenting instruction at %p: %s\n", addr, inst->getDisassembly().c_str());
        printf("  in block %p-%p\n", (void*)block->start(), (void*)block->end());
    }

    if (configuration->hasReplaceTagTree()) {

        // if there is a configuration tree, do what it
        // says ONLY and DO NOT iterate over every analysis
        //
        // NEWMODE: if you add your analysis to the config tree system, you'll
        // need to add a handler here (as well as all the associated changes in
        // FPConfig)
        //
        FPReplaceEntryTag tag = configuration->getReplaceTag(addr);
        assert(tag != RETAG_CANDIDATE);
        if (tag == RETAG_NULL) {
            preHandlers.push_back(PatchAPI::convert(new BPatch_nullExpr()));
            logfile->addMessage(STATUS, 0, "Inserted null pre-instrumentation.", "", "", inst);
        } else if (tag == RETAG_CINST) {
            buildPreInstrumentation(inst, FPAnalysisCInst::getInstance(), preHandlers, preNeedsRegisters);
        } else if (tag == RETAG_DCANCEL) {
            buildPreInstrumentation(inst, FPAnalysisDCancel::getInstance(), preHandlers, preNeedsRegisters);
        } else if (tag == RETAG_DNAN) {
            buildPreInstrumentation(inst, FPAnalysisDNan::getInstance(), preHandlers, preNeedsRegisters);
        } else if (tag == RETAG_TRANGE) {
            buildReplacement(addr, inst, block, FPAnalysisTRange::getInstance());
            replaced = true;
        } else if (tag == RETAG_SINGLE || tag == RETAG_DOUBLE) {
            buildReplacement(addr, inst, block, FPAnalysisInplace::getInstance());
            replaced = true;
        } else if (tag == RETAG_RPREC) {
            buildReplacement(addr, inst, block, FPAnalysisRPrec::getInstance());
            replaced = true;
        } else if (tag == RETAG_IGNORE || tag == RETAG_NONE) {
            // do nothing
        }

    } else {

        // for each analysis
        vector<FPAnalysis*>::iterator a;
        for (a = activeAnalyses.begin(); a != activeAnalyses.end(); a++) {

            if ((*a)->shouldReplace(inst)) {

                // can only replace once
                assert(!replaced);
                buildReplacement(addr, inst, block, *a);
                replaced = true;
            }

            if ((*a)->shouldPreInstrument(inst)) {
                buildPreInstrumentation(inst, *a, preHandlers, preNeedsRegisters);
            }
            if ((*a)->shouldPostInstrument(inst)) {
                buildPostInstrumentation(inst, *a, postHandlers, postNeedsRegisters);
            }
        }
    }

    // add register handlers
    if (preNeedsRegisters) {
        buildRegHandlers(inst, preHandlers);
    }
    if (postNeedsRegisters) {
        buildRegHandlers(inst, postHandlers);
    }

    // insert pre/post snippets
    Point *prePoint  = mainMgr->findPoint(
                        Location::InstructionInstance(func, block, (Address)addr),
                        Point::PreInsn, true);
    Point *postPoint = mainMgr->findPoint(
                        Location::InstructionInstance(func, block, (Address)addr),
                        Point::PostInsn, true);
    vector<Snippet::Ptr>::iterator k;
    for (k = preHandlers.begin(); k != preHandlers.end(); k++) {
        assert(prePoint != NULL);
        prePoint->pushBack(*k);
    }
    for (k = postHandlers.begin(); k != postHandlers.end(); k++) {
        assert(postPoint != NULL);
        postPoint->pushBack(*k);
    }

    if (replaced || (preHandlers.size() + postHandlers.size() > 0)) {
        // update instrumentation counts
        total_instrumented_instructions++;
        return true;
    } else {
        logfile->addMessage(WARNING, 0, "Ignored instruction",
                "", "", inst);
        return false;
    }
}

void instrumentInstruction(void* addr, unsigned char *bytes, size_t nbytes,
        PatchFunction *func, PatchBlock *block,
        BPatch_Vector<BPatch_snippet*> &initSnippets)
{
    // debug printing
    /*
     *printf("Instrumenting instruction at %p - %u bytes:", addr, (unsigned)nbytes);
     *for (unsigned k = 0; k < nbytes; k++)
     *printf(" %02x", bytes[k] & 0xFF);
     *printf("\n");
     */

    // decode instruction
    FPSemantics *inst = mainDecoder->lookupByAddr(addr);

    // insert instrumentation
    if (!inst || !inst->isValid()) {

        //printf("Unhandled or invalid instruction at %p (%u bytes): %s\n",
                //addr, (unsigned)nbytes,
                //(inst ? inst->getDisassembly().c_str() : "(invalid)"));

        if (debugPrint) {
            Point* point = mainMgr->findPoint(
                    Location::InstructionInstance(func, block, (Address)addr),
                    Point::PreInsn, true);
            assert(point != NULL);
            point->pushBack(buildUnsupportedInstHandler());
        }

    } else {

        // debug printing
        //cout << " instrumenting instruction w/ " << (preHandlers.size() + postHandlers.size()) << " snippets:" << endl;
        //cout << inst->toString() << endl;
        
        if (buildInstrumentation(addr, inst, func, block)) {

            // get instruction bytes and save to mutatee address
            // space
            //printf("saving instruction registration #%ld [%p]:  %s\n",
                    //inst->getIndex(), addr, inst->getDisassembly().c_str());
            BPatch_constExpr *bytesExpr = saveStringToBinary((char*)bytes, nbytes);
            assert(bytesExpr != NULL);
            //FPDecoderXED::printInstBytes(stdout, bytes, nbytes);

            // add registration call to init snippets
            BPatch_Vector<BPatch_snippet*> *regArgs = new BPatch_Vector<BPatch_snippet*>();
            regArgs->push_back(new BPatch_constExpr(inst->getIndex()));
            regArgs->push_back(new BPatch_constExpr(addr));
            regArgs->push_back(bytesExpr);
            regArgs->push_back(new BPatch_constExpr(nbytes));
            BPatch_funcCallExpr *regInst = new BPatch_funcCallExpr(*regFunc, *regArgs);
            initSnippets.push_back(regInst);

        }
    }
}

void instrumentBasicBlock(BPatch_function * function, BPatch_basicBlock *block, 
        BPatch_Vector<BPatch_snippet*> &initSnippets)
{
    static size_t MAX_RAW_INSN_SIZE = 16;

    Instruction::Ptr iptr;
    void *addr;
    unsigned char bytes[MAX_RAW_INSN_SIZE];
    size_t nbytes, i;

    // iterate backwards (PatchAPI restriction)
    PatchBlock::Insns insns;
    PatchAPI::convert(block)->getInsns(insns);
    //PatchBlock::Insns::iterator j;
    PatchBlock::Insns::reverse_iterator j;
    //for (j = insns.begin(); j != insns.end(); j++) {
    for (j = insns.rbegin(); j != insns.rend(); j++) {

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
            instrumentInstruction(addr, bytes, nbytes,
                    PatchAPI::convert(function), PatchAPI::convert(block),
                    initSnippets);
        }
    }
}

void instrumentFunction(BPatch_function *function, BPatch_Vector<BPatch_snippet*> &initSnippets)
{
    // iterate backwards (PatchAPI restriction)
    std::set<BPatch_basicBlock*> blocks;
    //std::set<BPatch_basicBlock*>::iterator b;
    std::set<BPatch_basicBlock*>::reverse_iterator b;
    BPatch_flowGraph *cfg = function->getCFG();
    cfg->getAllBasicBlocks(blocks);
    //for (b = blocks.begin(); b != blocks.end(); b++) {
    for (b = blocks.rbegin(); b != blocks.rend(); b++) {
        instrumentBasicBlock(function, *b, initSnippets);
    }
}

struct functionBasePredicate {
    bool operator()(BPatch_function *f1, BPatch_function *f2) const {
        return (f1->getBaseAddr() < f2->getBaseAddr());
    }
};

struct moduleNamePredicate {
    bool operator()(BPatch_module *m1, BPatch_module *m2) const {
        char m1name[BUFFER_STRING_LEN];
        char m2name[BUFFER_STRING_LEN];
        m1->getName(m1name, BUFFER_STRING_LEN);
        m2->getName(m2name, BUFFER_STRING_LEN);
        return (strcmp(m1name, m2name) < 0);
    }
};

void instrumentModule(BPatch_module *module, BPatch_Vector<BPatch_snippet*> &initSnippets)
{
	char funcname[BUFFER_STRING_LEN];
	std::vector<BPatch_function *>* functions;
	functions = module->getProcedures();

    // sort functions by base address
    std::sort(functions->begin(), functions->end(), functionBasePredicate());

	for (unsigned i = 0; i < functions->size(); i++) {
		BPatch_function *function = functions->at(i);
		function->getName(funcname, BUFFER_STRING_LEN);

        // CRITERIA FOR INSTRUMENTATION:
        // don't instrument
        //   - memset() or call_gmon_start() or frame_dummy()
        //   - functions that begin with an underscore
        // AND
        // if there's a preset list of functions to instrument,
        //   make sure the current function is on it
        // (and the inverse for excluded functions)
		if ( (strcmp(funcname,"memset")!=0) && (strcmp(funcname,"call_gmon_start")!=0)
                && (strcmp(funcname,"frame_dummy")!=0) && funcname[0] != '_' &&
                !(restrictFuncs == 'F' && find(funcs.begin(), funcs.end(), string(funcname)) != funcs.end()) &&
                (restrictFuncs != 'f' || find(funcs.begin(), funcs.end(), string(funcname)) != funcs.end())) {

            instrumentFunction(function, initSnippets);
		}
	}
}

void instrumentApplication()
{
	// lots of vectors...
	BPatch_Vector<BPatch_point*> *entryMainPoints;
	BPatch_Vector<BPatch_point*> *exitFuncPoints;
    BPatch_Vector<BPatch_snippet*> emptyArgs;

	// all of the initialization and output/cleanup calls
	BPatch_Vector<BPatch_snippet*> initSnippets;
	BPatch_Vector<BPatch_snippet*> finiSnippets;
    BPatch_snippet *initCode, *finiCode;
    BPatch_snippet *enableCall, *disableCall;

    // print pertinent warnings
    if (nullInst) {
        printf("NOTE: Null instrumentation enabled!\n");
    }
    if (fastInst) {
        printf("NOTE: Optimized instrumentation enabled!\n");
    }

    // enable conservative register saving if requested; otherwise, let libfpanalysis handle it
    if (saveFPR) {
        bpatch->setSaveFPR(true);
    } else {
        bpatch->setSaveFPR(false);
    }

    mainApp->beginInsertionSet();

	// grab references to analysis functions
    initFunc       = getAnalysisFunction("_INST_init_analysis");
    enableFunc     = getAnalysisFunction("_INST_enable_analysis");
    setFunc        = getAnalysisFunction("_INST_set_config");
    setReplaceFunc = getAnalysisFunction("_INST_set_config_replace_entry");
    regFunc        = getAnalysisFunction("_INST_register_inst");
    handlePreFunc  = getAnalysisFunction("_INST_handle_pre_analysis");
    handlePostFunc = getAnalysisFunction("_INST_handle_post_analysis");
    handleReplFunc = getAnalysisFunction("_INST_handle_replacement");
    handleUnspFunc = getAnalysisFunction("_INST_handle_unsupported_inst");
    disableFunc    = getAnalysisFunction("_INST_disable_analysis");
    cleanupFunc    = getAnalysisFunction("_INST_cleanup_analysis");

	// grab references to mutatee functions
    mainFunc = getMutateeFunction("main");
    exitFunc = getMutateeFunction(exitFuncName);
    entryMainPoints = mainFunc->findPoint(BPatch_entry);
    exitFuncPoints = exitFunc->findPoint(BPatch_exit);

    // replace some libm/libc functions
    replaceLibmFunctions();
    if (isAnalysisEnabled("sv_ptr") || isAnalysisEnabled("sv_inp")) {
        if (fortranMode) {
            replaceFunctionCalls("inst_fortran_report_", "_INST_fortran_report");
        } else {
            replaceFunctionCalls("printf", "_INST_printf");
            replaceFunctionCalls("fprintf", "_INST_fprintf");
            replaceFunctionCalls("sprintf", "_INST_sprintf");
        }
    }

    // build analysis initialization/enable/disable snippets
    initSnippets.push_back(new BPatch_funcCallExpr(*initFunc, emptyArgs));
    enableCall = new BPatch_funcCallExpr(*enableFunc, emptyArgs);
    //disableCall = new BPatch_funcCallExpr(*disableFunc, emptyArgs);
    // NEVER DISABLE -- TODO: fix this? dyninst doesn't seem to support
    // pointAfter very well for main()
    disableCall = new BPatch_nullExpr();

    // get list of all modules
	char modname[BUFFER_STRING_LEN];
    std::vector<BPatch_module *>* modules;
    std::vector<BPatch_module *>::iterator m;
    modules = mainImg->getModules();
    printf("Instrumenting modules.\n");

    // sort modules by name
    std::sort(modules->begin(), modules->end(), moduleNamePredicate());

    // for each module ...
    for (m = modules->begin(); m != modules->end(); m++) {
        (*m)->getName(modname, BUFFER_STRING_LEN);

        // don't instrument our own library or libc/libm
        if (strcmp(modname, "libfpanalysis.so") == 0 ||
            strcmp(modname, "libm.so.6") == 0 ||
            strcmp(modname, "libc.so.6") == 0) {
            continue;
        }

        // don't instrument shared libs unless requested
        if ((*m)->isSharedLib() && !instShared) {
            continue;
        }

        instrumentModule(*m, initSnippets);
    }

    // embed configuration entries and add an initialization call for each;
    // since we need to insert them at the very beginning of initSnippets,
    // add them in reverse order so that they'll be in the correct order in
    // the binary
    vector<string> config;
    configuration->getAllSettings(config);
    vector<string>::reverse_iterator cfgi;
    for (cfgi = config.rbegin(); cfgi != config.rend(); cfgi++) {
        BPatch_Vector<BPatch_snippet*> *initConfigArgs = new BPatch_Vector<BPatch_snippet*>();
        BPatch_constExpr *initNumInstsStr = saveStringToBinary((*cfgi).c_str(), 0);
        initConfigArgs->push_back(initNumInstsStr);
        BPatch_funcCallExpr *initConfigEntry = new BPatch_funcCallExpr(*setFunc, *initConfigArgs);
        initSnippets.insert(initSnippets.begin(), initConfigEntry);
    }
    
    // special handling for replacement configuration entries since we don't
    // want to have to allocate an entire string for them; again,
    // add them in reverse order so that they'll be in the correct order in
    // the binary
    vector<FPReplaceEntry*> rentries;
    configuration->getAllReplaceEntries(rentries);
    vector<FPReplaceEntry*>::reverse_iterator renti;
    for (renti = rentries.rbegin(); renti != rentries.rend(); renti++) {
        BPatch_Vector<BPatch_snippet*> *initReplaceArgs = new BPatch_Vector<BPatch_snippet*>();
        FPReplaceEntry *entry = *renti;
        //printf("adding replace entry: %s\n", entry->toString().c_str());
        BPatch_constExpr idxExpr (entry->idx);
        BPatch_constExpr addrExpr(entry->address);
        BPatch_constExpr typeExpr(entry->type);
        BPatch_constExpr tagExpr (entry->getEffectiveTag());
        initReplaceArgs->push_back(&idxExpr);
        initReplaceArgs->push_back(&addrExpr);
        initReplaceArgs->push_back(&typeExpr);
        initReplaceArgs->push_back(&tagExpr);
        BPatch_funcCallExpr *initReplaceEntry = new BPatch_funcCallExpr(*setReplaceFunc, *initReplaceArgs);
        initSnippets.insert(initSnippets.begin(), initReplaceEntry);
    }

    // generate application report
    stringstream report;
    report.clear(); report.str("");
    report << "Finished instrumentation:" << endl;
    report << "  " << total_replacements << " replaced library function(s)" << endl;
    report << "  " << total_fp_instructions << " floating-point instruction(s)" << endl;
    report << "    Total: " << total_instrumented_instructions << " instrumented" << endl;
    for (unsigned i=0; i<activeAnalyses.size(); i++) {
        report << "    " << activeAnalyses[i]->finalInstReport() << endl;
    }
    report << "    " << total_fp_instructions - total_instrumented_instructions << " ignored" << endl;
    report << "  " << total_basicblocks << " basic block(s)" << endl;
    report << "  " << total_functions << " function(s)" << endl;
    cout << report.str();
    logfile->addMessage(SUMMARY, 0, "Finished instrumentation.", report.str(), "", NULL);

    // add config value for number of instructions
    BPatch_Vector<BPatch_snippet*> initNumInstsArgs;
    stringstream ss; ss.clear(); ss.str("");
    ss << "num_instructions=" << iidx;
    BPatch_constExpr *initNumInstsStr = saveStringToBinary(ss.str().c_str(), 0);
    initNumInstsArgs.push_back(initNumInstsStr);
    BPatch_funcCallExpr initNumInsts(*setFunc, initNumInstsArgs);
    initSnippets.insert(initSnippets.begin(), &initNumInsts);

    // add cleanup call
    finiSnippets.push_back(new BPatch_funcCallExpr(*cleanupFunc, emptyArgs));

    if (nullInst) {

        // null initialize/cleanup placeholders (to test Dyninst overhead)
        initCode = new BPatch_nullExpr();
        finiCode = new BPatch_nullExpr();

    } else {

        // finish building initialization/cleanup snippets
        initCode = new BPatch_sequence(initSnippets);
        finiCode = new BPatch_sequence(finiSnippets);
    }

    // look for module with init/fini
    BPatch_module *initFiniModule = findInitFiniModule();
    if (initFiniModule) {

        // add initialization/cleanup at init/fini
        char nameBuffer[1024];
        initFiniModule->getName(nameBuffer, 1024);
        printf("Instrumenting init in module %s.\n", nameBuffer);
        initFiniModule->insertInitCallback(*initCode);
        printf("Instrumenting fini in module %s.\n", nameBuffer);
        initFiniModule->insertFiniCallback(*finiCode);

        // add enable/disable around main
        printf("Instrumenting entry of main.\n");
        mainApp->insertSnippet(*enableCall, *entryMainPoints, BPatch_callBefore);
        printf("Instrumenting exit of %s.\n", exitFuncName);
        mainApp->insertSnippet(*disableCall, *exitFuncPoints, BPatch_callAfter);

    } else {

        if (!nullInst) {
            // add enable and disable, then rebuild code
            initSnippets.push_back(enableCall);
            finiSnippets.insert(finiSnippets.begin(), disableCall);
            initCode = new BPatch_sequence(initSnippets);
            finiCode = new BPatch_sequence(finiSnippets);
        }

        // add at main and specified exit function
        printf("Instrumenting entry of main.\n");
        mainApp->insertSnippet(*initCode, *entryMainPoints, BPatch_callBefore);
        printf("Instrumenting exit of %s.\n", exitFuncName);
        mainApp->insertSnippet(*finiCode, *exitFuncPoints, BPatch_callAfter);
    }

    mainApp->finalizeInsertionSet(false);
}

// }}}

// {{{ decoding helper functions

/**
 * NOTE: the decoding pass is separate from instrumentation pass because the
 * instrumentation must work backwards (PatchAPI limitations), but we still want
 * the instruction IDs to be sequential to match config files created by fpconf.
 */

void decodeInstruction(void* addr, unsigned char *bytes, size_t nbytes)
{
    iidx++;
    total_fp_instructions++;
    
    FPSemantics *inst = mainDecoder->decode(iidx, addr, bytes, nbytes);

    if (listFuncs) {
        printf("      POINT %lu: bbidx=%lu fidx=%lu \"%s\" [", 
                iidx, bbidx, fidx,
                (inst ? inst->getDisassembly().c_str() : ""));
        FPDecoderXED::printInstBytes(stdout, bytes, nbytes);
        printf("]\n");
    }
}

void decodeBasicBlock(BPatch_basicBlock *block)
{
    static size_t MAX_RAW_INSN_SIZE = 16;

    Instruction::Ptr iptr;
    void *addr;
    unsigned char bytes[MAX_RAW_INSN_SIZE];
    size_t nbytes, i;

    bbidx++;
    total_basicblocks++;

    if (listFuncs) {
        printf("    BASIC BLOCK %lu: fidx=%lu start=%lx end=%lx size=%u\n", bbidx, fidx,
                block->getStartAddress(), block->getEndAddress(), block->size());
    }

    PatchBlock::Insns insns;
    PatchAPI::convert(block)->getInsns(insns);
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

        // apply filter
        if (mainDecoder->filter(bytes, nbytes)) {
            decodeInstruction(addr, bytes, nbytes);
        }
    }
}

void decodeFunction(BPatch_function *function, const char *name)
{
    fidx++;
    total_functions++;

    if (listFuncs) {
        Address start, end;
        function->getAddressRange(start, end);
        printf("  FUNCTION %lu: name=%s base=%p size=%u %s\n", fidx,
                name, function->getBaseAddr(),
                (unsigned)end - (unsigned)start,
                (function->isSharedLib() ? "[shared] " : ""));
    }

    // get all basic blocks
    std::set<BPatch_basicBlock*> blocks;
    std::set<BPatch_basicBlock*>::iterator b;
    BPatch_flowGraph *cfg = function->getCFG();
    cfg->getAllBasicBlocks(blocks);
    for (b = blocks.begin(); b != blocks.end(); b++) {
        decodeBasicBlock(*b);
    }
}

void decodeModule(BPatch_module *module, const char *name)
{
    midx++;
    total_modules++;

    if (listFuncs) {
        printf("MODULE %lu: name=%s base=%p\n", midx,
                name, module->getBaseAddr());
    }

	std::vector<BPatch_function *>* functions;
	functions = module->getProcedures();
    std::sort(functions->begin(), functions->end(), functionBasePredicate());
	char funcname[BUFFER_STRING_LEN];
	for (unsigned i = 0; i < functions->size(); i++) {
		BPatch_function *function = functions->at(i);
		function->getName(funcname, BUFFER_STRING_LEN);

        // CRITERIA FOR INSTRUMENTATION:
        // don't instrument
        //   - memset() or call_gmon_start() or frame_dummy()
        //   - functions that begin with an underscore
        // AND
        // if there's a preset list of functions to instrument,
        //   make sure the current function is on it
        // (and the inverse for excluded functions)
		if ( (strcmp(funcname,"memset")!=0) && (strcmp(funcname,"call_gmon_start")!=0)
                && (strcmp(funcname,"frame_dummy")!=0) && funcname[0] != '_' &&
                !(restrictFuncs == 'F' && find(funcs.begin(), funcs.end(), string(funcname)) != funcs.end()) &&
                (restrictFuncs != 'f' || find(funcs.begin(), funcs.end(), string(funcname)) != funcs.end())) {

            decodeFunction(function, funcname);
		}
    }
}

void decodeApplication()
{
	char modname[BUFFER_STRING_LEN];

    std::vector<BPatch_module *>* modules;
    std::vector<BPatch_module *>::iterator m;
    modules = mainImg->getModules();
    std::sort(modules->begin(), modules->end(), moduleNamePredicate());
    for (m = modules->begin(); m != modules->end(); m++) {
        (*m)->getName(modname, BUFFER_STRING_LEN);

        // don't decode our own library or libc/libm
        if (strcmp(modname, "libfpanalysis.so") == 0 ||
            strcmp(modname, "libm.so.6") == 0 ||
            strcmp(modname, "libc.so.6") == 0) {
            continue;
        }

        // don't decode shared libs unless requested
        if ((*m)->isSharedLib() && !instShared) {
            continue;
        }

        decodeModule(*m, modname);
    }
}

// }}}

// {{{ command-line parsing and help text

void usage()
{
	printf("\nUsage:  fpinst <analysis> [<options>] <mutatee> [<process-parameters>]\n");
    printf(" Performs floating-point instrumentation on a binary.\n");
    printf("Analyses:\n");
    printf("\n");
    printf("  --null               null instrumentation (for overhead testing)\n");
	printf("  --decoding-only      instruction decoding only (default--also useful for overhead testing)\n");

    for (size_t aidx=0; aidx < (size_t)TOTAL_ANALYSIS_COUNT; aidx++) {
        printf("  %-20s %s\n", allAnalysisInfo[aidx].fpinstUsage,
                allAnalysisInfo[aidx].fpinstHelpText);
        if (strcmp("", allAnalysisInfo[aidx].fpinstHelpText2)!=0) {
            printf("  %20s   %s\n", "", allAnalysisInfo[aidx].fpinstHelpText2);
        }
    }

    printf("\n");
    printf(" Options:\n");
    printf("\n");
    printf("  -c <filename>        use the specified base configuration file (default is \"base.cfg\")\n");
    printf("  -C \"<key>=<value>\"   add the given additional setting to the configuration\n");
    //printf("  -d                   detect cancellations (only activated with shadow/pointer value analyses)\n");
    printf("  -e <function-name>   print the summary on exit from a specific function (default is \"main\")\n");
    printf("                         (NOTE: -e is ignored if _fini can be used instead)\n");
    printf("  -f <filename>        restrict instrumentation to a list of functions from a file\n");
    printf("  -F <filename>        restrict instrumentation to EXCLUDE a list of functions from a file\n");
#if INCLUDE_DEBUG
    printf("  -g                   enable debug output (only activated with shadow/pointer value analyses)\n");
#endif
    printf("  -i                   instrument only (don't run the instrumented program)\n");
    printf("  -k                   conservative mode (saves entire x87/SSE state outside libfpanalysis)\n");
    printf("  -l                   list all instrumented functions\n");
    printf("  -L <filename>        write to specified log file\n");
    printf("  -m                   detect mismatches (only activated with shadow/pointer value analyses)\n");
    printf("  -M                   multicore mode (use LOCK prefix for INC instructions)\n");
    printf("  -N                   enable FORTRAN mode (instrument inst_fortran_report instead of printf)\n");
    printf("  -o <filename>        output to specified filename (default is \"mutant\")\n");
    printf("  -p                   run as process instead of using binary rewriter\n");
    printf("  -P                   enable profiling output (uses SIGPROF at microsecond intervals to print IP)\n");
    printf("  -r                   report all global variables (only activated with shadow/pointer value analyses)\n");
    printf("  -s                   instrument functions in shared libraries\n");
    printf("  -S                   disable logarithmic event sampling (only activated with cancellation detection)\n");
    printf("  -t                   optimize analysis with faster routines for some instructions\n");
    printf("  -T                   specify a tag that is appended to log file names\n");
    printf("  -w                   enable stack walks/traces (only activated with cancellation detection)\n");
    printf("  -y                   insert symbols for instrumentation stack frames (high disk overhead!)\n");
    printf("\n");
}

void parseFuncFile(char *fn, vector<string> &funcs)
{
    char buffer[BUFFER_STRING_LEN], *nl;
    FILE *fin = fopen(fn, "r");
    if (fin) {
        while (fgets(buffer, BUFFER_STRING_LEN, fin)) {
            nl = strrchr(buffer, '\n');
            if (nl) *nl = '\0';
            nl = strrchr(buffer, '\r');
            if (nl) *nl = '\0';
            string s = string(buffer);
            if (s != "") {
                funcs.push_back(s);
            }
        }
        fclose(fin);
    }
}


bool parseCommandLine(unsigned argc, char *argv[])
{
    unsigned i;
    bool validArgs = false;

    // analysis arguments
	for (i = 1; i < argc; i++) {

        bool foundAnalysisTag = false;
        for (size_t aidx=0; aidx < (size_t)TOTAL_ANALYSIS_COUNT; aidx++) {
            string param = allAnalysisInfo[aidx].fpinstSwitch;
            if (strcmp(argv[i], param.c_str())==0) {
                foundAnalysisTag = true;
                fpinstAnalysisEnabled[aidx] = true;
                if (allAnalysisInfo[aidx].fpinstParam && strcmp("", allAnalysisInfo[aidx].fpinstParam)!=0) {
                    fpinstAnalysisParam[aidx] = argv[++i];
                }
            }
        }
        if (foundAnalysisTag) {
            continue;
        }

		if (strcmp(argv[i], "-h")==0) {
			usage();
			exit(EXIT_SUCCESS);
		} else if (strcmp(argv[i], "-p")==0) {
			process = true;
		} else if (strcmp(argv[i], "-l")==0) {
			listFuncs = true;
		} else if (strcmp(argv[i], "-s")==0) {
			instShared = true;
		} else if (strcmp(argv[i], "-k")==0) {
			saveFPR = true;
		} else if (strcmp(argv[i], "-i")==0) {
			instOnly = true;
		} else if (strcmp(argv[i], "-t")==0) {
			fastInst = true;
		} else if (strcmp(argv[i], "-m")==0) {
			mismatches = true;
		} else if (strcmp(argv[i], "-M")==0) {
			multicoreMode = true;
		} else if (strcmp(argv[i], "-N")==0) {
			fortranMode = true;
		} else if (strcmp(argv[i], "-d")==0) {
			cancellations = true;
#if INCLUDE_DEBUG
		} else if (strcmp(argv[i], "-g")==0) {
			debugPrint = true;
#endif
		} else if (strcmp(argv[i], "-w")==0) {
			stwalk = true;
		} else if (strcmp(argv[i], "-r")==0) {
			reportAllGlobals = true;
		} else if (strcmp(argv[i], "-P")==0) {
			profiling = true;
		} else if (strcmp(argv[i], "-S")==0) {
			disableSampling = true;
        } else if (strcmp(argv[i], "-y")==0) {
            instrFrames = true;
		} else if (strcmp(argv[i], "--null")==0) {
			nullInst = true;
		} else if (strcmp(argv[i], "--decoding-only")==0) {
            // default
		} else if (strcmp(argv[i], "-c")==0 && i < argc-1) {
            configFile = argv[++i];
		} else if (strcmp(argv[i], "-L")==0 && i < argc-1) {
            logFile = argv[++i];
		} else if (strcmp(argv[i], "-T")==0 && i < argc-1) {
            logTag = argv[++i];
		} else if (strcmp(argv[i], "-C")==0 && i < argc-1) {
            extraConfigs.push_back(string(argv[++i]));
		} else if (strcmp(argv[i], "-e")==0 && i < argc-1) {
            exitFuncName = argv[++i];
		} else if (strcmp(argv[i], "-f")==0 && i < argc-1) {
            parseFuncFile(argv[++i], funcs);
            restrictFuncs = 'f';
		} else if (strcmp(argv[i], "-F")==0 && i < argc-1) {
            parseFuncFile(argv[++i], funcs);
            restrictFuncs = 'F';
		} else if (strcmp(argv[i], "-o")==0 && i < argc-1) {
            outFile = argv[++i];
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

    // prepare child arguments (sorry, this is ugly)
    for (i = 0; i < 45; i++) {
        child_argv[i] = (char*)malloc(sizeof(char[50]));
        strncpy(child_argv[i],"",50);
    }
    strncpy(child_argv[0], argv[binaryArg], 50);
    for (i = binaryArg+1; i < argc && i-binaryArg-1 < 45; i++) {
        strncpy(child_argv[i-binaryArg], argv[i], 50);
    }
    child_argv[argc-binaryArg] = NULL;

    return validArgs;
}

// }}}


int main(int argc, char *argv[])
{
    // enable to get profiling data on the instrumenter
    //_INST_begin_profiling();

    // initialize top-level analysis activation flags
    for (size_t aidx=0; aidx<(size_t)TOTAL_ANALYSIS_COUNT; aidx++) {
        fpinstAnalysisEnabled[aidx] = false;
        fpinstAnalysisParam[aidx] = NULL;
    }

    logfile = new FPLog("fpinst.log");

    // analysis configuration
	if (!parseCommandLine((unsigned)argc, argv)) {
		usage();
		exit(EXIT_FAILURE);
	}
    logfile->setDebugFile(binary);
    printf("Reading default configuration from %s\n", configFile);
    configuration = new FPConfig(configFile);
    setup_config_file(configuration);

    // initialize analysis stubs
    initializeActiveAnalyses();

	// initalize DynInst library
	bpatch = new BPatch;
	bpatch->registerErrorCallback(errorFunc);
    // TEMPORARY: disable tramp guard
    //bpatch->setTrampRecursive(true);

    // this enables the creation of symbols for each trampoline,
    // which takes a TON of memory (20+GB); I guess it's needed for
    // stack walking, but the price is high
    if (instrFrames) {
        bpatch->setInstrStackFrames(true);
    }

    // initialize instruction decoder
    mainDecoder = new FPDecoderXED();       // use Intel's decoder from Pin
    //mainDecoder = new FPDecoderIAPI();    // use Dyninst's InstructionAPI

	// start/open application
	BPatch_addressSpace *app;
	printf("Opening file: %s\n", binary);
	if (process) {
		// start process
		app = bpatch->processCreate(binary, (const char **)child_argv);
	} else {
		// open binary and dependencies
        // (even if we're not instrumenting shared libraries, we may need to
        // replace libm/libc functions)
		app = bpatch->openBinary(binary, true);
	}
	if (app == NULL) {
		printf("ERROR: Unable to start/open application.\n");
        exit(EXIT_FAILURE);
    }

	// save references to the application image
    mainApp = app;
    mainImg = mainApp->getImage();
    mainMgr = PatchAPI::convert(mainApp);

    // add the instrumentation library
    libFPAnalysis = ((BPatch_binaryEdit*)app)->loadLibrary("libfpanalysis.so");
	if (libFPAnalysis == NULL) {
		printf("ERROR: Unable to open libfpanalysis.so.\n");
        exit(EXIT_FAILURE);
    }

    // DEBUG: list modules in library
    /*
     *vector<BPatch_module *> libMods;
     *libFPAnalysis->modules(libMods);
     *printf("Loaded libfpanalysis with %lu module(s):\n", libMods.size());
     *char mname[1024];
     *for (unsigned i=0; i<libMods.size(); i++) {
     *    BPatch_module *m = libMods.at(i);
     *    m->getName(mname, 1024);
     *    printf(" - %s\n", mname);
     *}
     */

    // DEBUG: list functions in image
    /*
     *vector<BPatch_function *> *libFuncs = app->getImage()->getProcedures(true);
     *printf("Loaded libfpanalysis with %lu function(s):\n", libFuncs->size());
     *for (unsigned i=0; i<libFuncs->size(); i++) {
     *   BPatch_function *f = libFuncs->at(i);
     *   printf(" - %s\n", f->getName().c_str());
     *}
     */

    // add the libc/libm instrumentation runtime libraries (if necessary)
    if (isAnalysisEnabled("sv_ptr") || isAnalysisEnabled("sv_inp")) {
        libFPC = ((BPatch_binaryEdit*)app)->loadLibrary("libfpc.so");
        if (libFPC == NULL) {
            printf("ERROR: Unable to open libfpc.so.\n");
            exit(EXIT_FAILURE);
        }
        libFPM = ((BPatch_binaryEdit*)app)->loadLibrary("libfpm.so");
        if (libFPM == NULL) {
            printf("ERROR: Unable to open libfpm.so.\n");
            exit(EXIT_FAILURE);
        }
    }

    // perform instrumentation (agnostic to process/rewrite status)
    printf("Configuration:\n%s", configuration->getSummary().c_str());
    printf("Instrumenting application ...\n");
    decodeApplication();
    instrumentApplication();
    printf("Instrumentation complete!\n");

    // finalize log file
    logfile->close();

    // resume or rewrite/execute application
    if (process) {

        if (!instOnly) {
            printf("Resuming process ...\n");
            ((BPatch_process*)app)->continueExecution();
            while (!((BPatch_process*)app)->isTerminated()) {
                bpatch->waitForStatusChange();
            }
        }
        printf("Done.\n");

    } else {

        printf("Writing new binary to \"%s\" ...\n", outFile);
        ((BPatch_binaryEdit*)app)->writeFile(outFile);
        printf("Done.\n");
        if (!instOnly) {
            execv(outFile, child_argv);
        }

    }

	return(EXIT_SUCCESS);
}

