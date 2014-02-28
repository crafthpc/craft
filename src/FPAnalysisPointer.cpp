#include "fpflag.h"
#include "FPAnalysisPointer.h"

namespace FPInst {

#ifdef USE_GARBAGE_COLLECTOR

// hooks for garbage collection
void fpinst_init_hook (FPLog* log);
void *fpinst_malloc (size_t, const void *);
void *fpinst_realloc (void*, size_t, const void *);
void fpinst_free (void*, const void *);
void *(*fpinst_old_malloc) (size_t, const void *);
void *(*fpinst_old_realloc) (void*, size_t, const void *);
void (*fpinst_old_free) (void*, const void *);

#define RESTORE_GC_HOOKS  __malloc_hook = fpinst_old_malloc; \
                          __realloc_hook = fpinst_old_realloc; \
                          __free_hook = fpinst_old_free

#define SAVE_GC_HOOKS     fpinst_old_malloc = __malloc_hook; \
                          fpinst_old_realloc = __realloc_hook; \
                          fpinst_old_free = __free_hook

#define INSTALL_GC_HOOKS  __malloc_hook = fpinst_malloc; \
                          __realloc_hook = fpinst_realloc; \
                          __free_hook = fpinst_free

bool hook_installed = false;
FPLog* hook_log = NULL;
//string hook_malloc_lbl= "malloc";
//string hook_realloc_lbl("realloc");
//string hook_free_lbl("free");

//size_t hook_allocated = 0;

/*
 *double* hook_temp_ptr;
 *int hook_temp_size = 10;
 */

void fpinst_init_hook (FPLog* log)
{
    if (!hook_installed) {
        SAVE_GC_HOOKS;
        INSTALL_GC_HOOKS;
        hook_installed = true;
        hook_log = log;
        printf("Installed malloc hooks\n");
    }
}

void * fpinst_malloc (size_t size, const void *)
{
    void *result;
    RESTORE_GC_HOOKS;
    if (_INST_status == _INST_ACTIVE) {
        result = malloc(size);
        //printf("internal malloc (%lu) = %p\n", size, result);
        /*
         *static size_t alloc_bytes = 0;
         *static size_t alloc_bytes_total = 0;
         *alloc_bytes += size;
         *if (alloc_bytes > 1024) {
         *    alloc_bytes_total += alloc_bytes/1024;
         *    alloc_bytes = alloc_bytes%1024;
         *}
         *if (alloc_bytes_total %1024 == 0) {
         *    printf("internal malloc: %luM allocated\n", alloc_bytes_total);
         *}
         */
    } else {
        result = GC_MALLOC(size);
        //hook_allocated += size;
        //printf("GC malloc (%lu) = %p\n", size, result);
        //printf("  MEM: [ %8lu  KB =  %8lu MB ]    malloc (%lu) = %p\n", 
                //hook_allocated, hook_allocated/1024, size, result);
        /*
         *if (size == (size_t)(sizeof(double)*hook_temp_size*hook_temp_size)) {
         *    hook_temp_ptr = (double*)result;
         *    printf("  matrix found!!\n");
         *}
         */
        //raise(SIGINT);
    }
    SAVE_GC_HOOKS;
    //if (hook_log && size > (size_t)5000) {
        //hook_log->addMessage(STATUS, (long)size, hook_malloc_lbl, 
                //hook_malloc_lbl, hook_log->getStackTrace(), NULL);
    //}
    //printf("%s\n", (new FPLog("dummy.txt"))->getStackTrace().c_str());
    INSTALL_GC_HOOKS;
    if (!result) {
        printf("ERROR: Out of memory!\n");
    }
    return result;
}

void * fpinst_realloc (void *ptr, size_t size, const void *)
{
    void *result;
    RESTORE_GC_HOOKS;
    if (_INST_status == _INST_ACTIVE) {
        result = realloc(ptr, size);
        //printf("internal realloc (%u) = %p\n", (unsigned int) size, result);
    } else {
        result = GC_REALLOC(ptr, size);
        //printf("realloc (%u) = %p\n", (unsigned int) size, result);
    }
    SAVE_GC_HOOKS;
    //if (hook_log && size > (size_t)5000) {
        //hook_log->addMessage(STATUS, (long)size, hook_realloc_lbl, 
                //hook_log->getStackTrace(), NULL);
    //}
    INSTALL_GC_HOOKS;
    if (!result) {
        printf("ERROR: Out of memory!\n");
    }
    return result;
}

void fpinst_free (void *ptr, const void *)
{
    RESTORE_GC_HOOKS;
    SAVE_GC_HOOKS;
    if (_INST_status == _INST_ACTIVE) {
        free(ptr);
        //printf ("internal free (%p)\n", ptr);
    } /*else {
        // essentially a NOOP now
        GC_gcollect();
        printf ("gcollect (%p)\n", ptr);
    }*/
    INSTALL_GC_HOOKS;
}

#endif

bool _INST_segfault = false;
void _INST_segfault_handler (int /*sig*/)
{
    _INST_segfault = true;
}

bool FPAnalysisPointer::debugPrint = false;

void FPAnalysisPointer::enableDebugPrint()
{
    debugPrint = true;
}

void FPAnalysisPointer::disableDebugPrint()
{
    debugPrint = false;
}

bool FPAnalysisPointer::isDebugPrintEnabled()
{
    return debugPrint;
}

extern "C" {
    FPAnalysisPointer *_INST_Main_PointerAnalysis = NULL;
}

bool FPAnalysisPointer::existsInstance()
{
    return (_INST_Main_PointerAnalysis != NULL);
}

FPAnalysisPointer* FPAnalysisPointer::getInstance()
{
    if (!_INST_Main_PointerAnalysis) {
        _INST_Main_PointerAnalysis = new FPAnalysisPointer();
    }
    return _INST_Main_PointerAnalysis;
}


FPAnalysisPointer::FPAnalysisPointer()
{
    detectCancellations = false;
    cancelAnalysis = NULL;
    reportAllGlobals = false;
    insnsInstrumented = 0;
#ifdef USE_LIVE_PTR_LIST
    unsigned long i;
    for (i = 0; i < LIVE_PTR_TBL_SIZE; i++) {
        livePointers[i] = NULL;
    }
#endif
}

string FPAnalysisPointer::getTag()
{
    return "sv_ptr";
}

string FPAnalysisPointer::getDescription()
{
    return "Pointer-replacement Shadow Value Analysis";
}

void FPAnalysisPointer::configure(FPConfig *config, FPDecoder *decoder,
        FPLog *log, FPContext *context)
{
    FPAnalysis::configure(config, decoder, log, context);
#ifdef USE_GARBAGE_COLLECTOR
    if (context != NULL) {
        fpinst_init_hook(log);
    }
#endif
    if (config->hasValue("sv_ptr_type")) {
        string type = config->getValue("sv_ptr_type");
        if (type == "single") {
            mainPolicy = new FPSVPolicy(SVT_IEEE_Single);
        } else if (type == "double") {
            mainPolicy = new FPSVPolicy(SVT_IEEE_Double);
        } else {
            printf("ERROR: unrecognized shadow value type \"%s\"",
                    type.c_str());
            mainPolicy = new FPSVPolicy(SVT_IEEE_Double);
        }
    } else {
        mainPolicy = new FPSVPolicy(SVT_IEEE_Double);
    }
    if (config->getValue("report_all_globals") == "yes") {
        enableReportAllGlobals();
    }
    if (config->getValue("enable_debug_print") == "yes") {
        enableDebugPrint();
    }
    vector<FPShadowEntry*> entries;
    config->getAllShadowEntries(entries);
    setShadowEntries(entries);
}

bool FPAnalysisPointer::shouldPreInstrument(FPSemantics * /*inst*/)
{
    return false;
}

bool FPAnalysisPointer::shouldPostInstrument(FPSemantics * /*inst*/)
{
    return false;
}

bool FPAnalysisPointer::shouldReplace(FPSemantics *inst)
{
    return mainPolicy->shouldInstrument(inst);
}

Snippet::Ptr FPAnalysisPointer::buildPreInstrumentation(FPSemantics * /*inst*/,
        BPatch_addressSpace * /*app*/, bool & /*needsRegisters*/)
{
    return Snippet::Ptr();
}

Snippet::Ptr FPAnalysisPointer::buildPostInstrumentation(FPSemantics * /*inst*/,
        BPatch_addressSpace * /*app*/, bool & /*needsRegisters*/)
{
    return Snippet::Ptr();
}

Snippet::Ptr FPAnalysisPointer::buildReplacementCode(FPSemantics * /*inst*/,
        BPatch_addressSpace * /*app*/, bool &needsRegisters)
{
    insnsInstrumented++;
    needsRegisters = true;
    return Snippet::Ptr();
}

void FPAnalysisPointer::enableCancellationDetection(int minPriority)
{
    detectCancellations = true;
    if (cancelAnalysis == NULL) {
        cancelAnalysis = FPAnalysisDCancel::getInstance();
        cancelAnalysis->configure(configuration, decoder, logFile, context);
        cancelAnalysis->setMinPriority(minPriority);
    }
}

void FPAnalysisPointer::disableCancellationDetection()
{
    detectCancellations = false;
}

void FPAnalysisPointer::enableReportAllGlobals()
{
    reportAllGlobals = true;
}

void FPAnalysisPointer::disableReportAllGlobals()
{
    reportAllGlobals = false;
}

void FPAnalysisPointer::setShadowEntries(vector<FPShadowEntry*> &entries)
{
    FPShadowEntry *entry;
    FPOperandAddress addr;
    unsigned long isize, rows, cols;
    vector<FPShadowEntry*>::iterator i;
    shadowEntries.clear();
    for (i=entries.begin(); i!=entries.end(); i++) {
        shadowEntries.push_back(*i);
        entry = *i;
        if (entry->type == INIT) {
            addr = entry->getAddress(logFile, context);
            isize = logFile->getGlobalVarSize(addr);
            rows = entry->size[0];
            cols = entry->size[1];
            if (isize > 0 && entry->vals.size() < rows * cols) {
                printf("ERROR: size of array/matrix does not match size of provided values: %s\n", entry->name.c_str());
            }
        }
    }
}

void FPAnalysisPointer::getShadowEntries(vector<FPShadowEntry*> &entries)
{
    // does not clear return vector first!
    vector<FPShadowEntry*>::iterator i;
    for (i=shadowEntries.begin(); i!=shadowEntries.end(); i++) {
        entries.push_back(*i);
    }
}

string FPAnalysisPointer::finalInstReport()
{
    stringstream ss;
    ss << "Pointer: " << insnsInstrumented << " instrumented";
    return ss.str();
}

void FPAnalysisPointer::registerInstruction(FPSemantics * /*inst*/)
{ }

void FPAnalysisPointer::handlePreInstruction(FPSemantics * /*inst*/)
{ }

void FPAnalysisPointer::handlePostInstruction(FPSemantics * /*inst*/)
{ }

void FPAnalysisPointer::handleReplacement(FPSemantics *inst)
{
    FPOperation *op;
    FPOperationType opt;
    FPOperandSet *ops;
    FPOperand **outputs;
    //FPOperand *output;
    FPOperand **inputs;
    unsigned i, j;
    size_t nOps;

#if INCLUDE_DEBUG
    set<FPRegister> regs;
    bool prevDebugPrint = debugPrint;
    // exclude certain instructions from debug output
    // TODO: remove?
    //if (inst->getDisassembly().find("cvtsi2sd") != string::npos) {
        //debugPrint = false;
    //}
    if (debugPrint) {
        inst->getNeededRegisters(regs);
        cout << "started pointer analysis handling at " << hex << inst->getAddress() << dec;
        cout << " [size=" << inst->getNumBytes() << "]: ";
        cout << inst->getDisassembly() << endl;
        //cout << hex << "esp: " << context->reg_esp << "  ebp: " << context->reg_ebp << dec << endl;
        for (set<FPRegister>::iterator r = regs.begin(); r != regs.end(); r++) {
            context->dumpFXReg(*r);
        }
        //cout << inst->toString() << endl;
    }
#endif

    currInst = inst;
    context->resetQueuedActions();

    for (i=0; i<inst->numOps; i++) {
        op = (*inst)[i];
        opt = op->type;

#if INCLUDE_DEBUG
        if (debugPrint) {
            cout << "operation #" << i << ":  type=" << FPOperation::FPOpType2Str(opt) << endl;
        }
#endif
        if (opt == OP_PUSH_X87) {
            context->pushX87Stack(true);
            continue;
        } else if (opt == OP_POP_X87) {
            context->popX87Stack(true);
            continue;
        }

        ops = op->opSets;
        nOps = op->numOpSets;
        for (j=0; j<nOps; j++) {
            outputs = ops[j].out;
            //output = outputs[0];
            inputs = ops[j].in;
            currFlag = NULL;
            switch (opt) {

                // conversions
                case OP_MOV: case OP_CVT:
                    handleConvert(outputs[0], inputs[0]); break;

                case OP_ZERO:
                    handleZero(outputs[0]); break;

                // unary operations
                case OP_SQRT: case OP_NEG: case OP_ABS: case OP_RCP:
                    // "normal" operations
                    handleUnaryOp(opt, outputs[0], inputs[0]); break;

                case OP_AM:
                    // no output
                    handleUnaryOp(opt, NULL, inputs[0]); break;

                // binary operations

                case OP_ADD: case OP_SUB:
                case OP_MUL: case OP_DIV: case OP_FMOD:
                case OP_MIN: case OP_MAX:
                case OP_AND: case OP_OR: case OP_XOR:
                    // "normal" operations
                    handleBinaryOp(opt, outputs[0], inputs[0], inputs[1]); break;

                case OP_COM: case OP_UCOM:
                case OP_COMI: case OP_UCOMI:
                    // no output
                    handleBinaryOp(opt, NULL, inputs[0], inputs[1]); break;

                case OP_CMP:
                    // third input (flag)
                    inputs[2]->refresh(context);
                    currFlag = &(inputs[2]->currentValue);
                    handleBinaryOp(opt, outputs[0], inputs[0], inputs[1]); break;


                // }}}
                default:
                    stringstream ss;
                    ss.clear();
                    ss.str("");
                    ss << "unhandled instruction: " << inst->getDisassembly();
                    ss << "  [" << FPOperation::FPOpType2Str(opt) << "]" << endl;
                    logFile->addMessage(WARNING, 0, ss.str(), "", 
                           "", inst);
                    break;
            }
        }
    }

    context->finalizeQueuedActions();

#if INCLUDE_DEBUG
    if (debugPrint) {
        for (set<FPRegister>::iterator r = regs.begin(); r != regs.end(); r++) {
            context->dumpFXReg(*r);
        }
        cout << "done handling at " << hex << inst->getAddress() << dec;
        cout << /*": " << inst->getDisassembly() <<*/ endl << endl;
    }
    debugPrint = prevDebugPrint;
#endif
}

void FPAnalysisPointer::handleConvert(FPOperand *output, FPOperand *input)
{
    FPSV *inputVal, *resultVal = NULL;
    FPSVType resultType;
    FPOperandAddress resultAddr;

    // initialize inputs and outputs
    input->refresh(context);
    inputVal = getOrCreateSV(input);
    output->refreshAddress(context);
    resultType = mainPolicy->getSVType(output, currInst);
    resultAddr = output->getCurrentAddress();
    
    // do conversion if necessary
    if (resultType == inputVal->type) {
        resultVal = inputVal;
        resultVal->addr = resultAddr;
    } else {
        resultVal = createSV(resultType, resultAddr);
        convertSV(resultVal, inputVal);
        resultVal->setSeedData(inputVal);
#if INCLUDE_DEBUG
        if (debugPrint) {
            cout << "FPAnalysisPointer convert: " << inputVal->toDetailedString() << " [" << inputVal << "] => "
                 << resultVal->toDetailedString() << " [" << resultVal << "]" << endl;
        }
#endif
    }

    if (mainPolicy->shouldReplaceWithPtr(output, currInst)) {
        saveSVPtrToContext(output, resultVal, context);
    } else {
        saveSVValueToContext(output, resultVal, context);
    }
}

void FPAnalysisPointer::handleZero(FPOperand *output)
{
    FPSVType resultType;
    FPOperandAddress resultAddr;
    FPSV *resultVal = NULL;

    // do operation
    output->refreshAddress(context);
    resultType = mainPolicy->getSVType(output, currInst);
    resultAddr = output->getCurrentAddress();
    resultVal = createSV(resultType, resultAddr);
    resultVal->setToZero();

    // save to context
    if (mainPolicy->shouldReplaceWithPtr(output, currInst)) {
        saveSVPtrToContext(output, resultVal, context);
    } else {
        saveSVValueToContext(output, resultVal, context);
    }
}

void FPAnalysisPointer::handleUnaryOp(FPOperationType type, FPOperand *output, FPOperand *input)
{
    FPSV *inputVal;
    FPSVType resultType = SVT_NONE;
    FPOperandAddress resultAddr = NULL;
    FPSV *resultVal = NULL, *temp = NULL;

    // initialize inputs and outputs
    input->refresh(context);
    inputVal = getOrCreateSV(input);
    if (output) {
        output->refreshAddress(context);
        resultType = mainPolicy->getSVType(output, currInst);
        resultAddr = output->getCurrentAddress();

        if (resultType == inputVal->type) {
            // output is also of the same type
            // for now, let's save an allocation and just write to the output
            // TODO: fix this semantic error!!!
            temp = getOrCreateSV(output);
        } else {
            temp = createSV(inputVal->type, resultAddr);
        }
    }

    // do operation
    switch (type) {
        case OP_AM:
            handleAm(inputVal); break;
        default:        // all other "normal" operations
            temp->doUnaryOp(type, inputVal); break;
    }

    if (output) {
        // convert the result if it is not already of the desired type
        if (temp->type == resultType) {
            resultVal = temp;
        } else {
            resultVal = createSV(resultType, resultAddr);
            convertSV(resultVal, temp);
        }

        // save to context
        if (mainPolicy->shouldReplaceWithPtr(output, currInst)) {
            saveSVPtrToContext(output, resultVal, context);
        } else {
            saveSVValueToContext(output, resultVal, context);
        }
    }
}


void FPAnalysisPointer::handleBinaryOp(FPOperationType type, FPOperand *output, FPOperand *input1, FPOperand *input2)
{
    FPSV *input1Val, *input2Val;
    FPSVType commonType = SVT_NONE;
    FPSV *temp1 = NULL, *temp2 = NULL;
    FPSVType resultType = SVT_NONE;
    FPOperandAddress resultAddr = NULL;;
    FPSV *resultVal = NULL, *temp = NULL;


    // initialize inputs and outputs
    input1->refresh(context);
    input2->refresh(context);
    input1Val = getOrCreateSV(input1);
    input2Val = getOrCreateSV(input2);
    if (output) {
        resultType = mainPolicy->getSVType(output, currInst);
        resultAddr = output->getCurrentAddress();
        output->refreshAddress(context);
    }

    if (input1Val->type == input2Val->type) {

        // easy case: inputs are of the same type
        temp1 = input1Val;
        temp2 = input2Val;

        if (output && resultType == input1Val->type) {
            // output is also of the same type
            // for now, let's save an allocation and just write to the output
            // TODO: fix this semantic error!!!
            temp = getOrCreateSV(output);
        } else if (output) {
            temp = createSV(input1Val->type, resultAddr);
        }

    } else {

        // hard case: inputs are of different types, so find the policy-desired
        // common type and convert them both to that type for the operation
        commonType = mainPolicy->getCommonType(input1Val->type, input2Val->type);
        assert(commonType != SVT_NONE);
        if (input1Val->type == commonType) {
            temp1 = input1Val;
        } else {
            temp1 = createSV(commonType, input1Val->addr);
            temp1->setSeedData(input1Val);
            convertSV(temp1, input1Val);
        }
        if (input2Val->type == commonType) {
            temp2 = input2Val;
        } else {
            temp2 = createSV(commonType, input2Val->addr);
            temp2->setSeedData(input2Val);
            convertSV(temp2, input2Val);
        }
        if (output) {
            temp = createSV(commonType, resultAddr);
        }
    }

    // do operation
    switch (type) {
        case OP_AND:
            handleAndOp(temp, temp1, temp2); break;
        case OP_OR:
            handleOrOp(temp, temp1, temp2); break;
        case OP_XOR:
            handleXorOp(temp, temp1, temp2); break;
        case OP_COM:
            handleCom(temp1, temp2, true); break;
        case OP_UCOM:
            handleCom(temp1, temp2, false); break;
        case OP_COMI:
            handleComI(temp1, temp2, true); break;
        case OP_UCOMI:
            handleComI(temp1, temp2, false); break;
        case OP_CMP:
            handleCmp(temp, temp1, temp2, true); break;
        default:        // all other "normal" operations
            temp->doBinaryOp(type, temp1, temp2); break;
    }

    if (output) {
        // convert the result if it is not already of the desired type
        if (temp->type == resultType) {
            resultVal = temp;
        } else {
            resultVal = createSV(resultType, resultAddr);
            convertSV(resultVal, temp);
        }

        // save to context
        if (mainPolicy->shouldReplaceWithPtr(output, currInst)) {
            saveSVPtrToContext(output, resultVal, context);
        } else {
            saveSVValueToContext(output, resultVal, context);
        }
    }
}

#define IS_800_CONST(OPD) (((OPD)->seed_size == 1 && (*(uint32_t*)(OPD)->seed_data) == 0x80000000) || \
                           ((OPD)->seed_size == 2 && (*(uint64_t*)(OPD)->seed_data) == 0x8000000000000000))
#define IS_7FF_CONST(OPD) (((OPD)->seed_size == 1 && (*(uint32_t*)(OPD)->seed_data) == 0x7fffffff) || \
                           ((OPD)->seed_size == 2 && (*(uint64_t*)(OPD)->seed_data) == 0x7fffffffffffffff))
#define IS_0_CONST(OPD)   (((OPD)->seed_size == 1 && (*(uint32_t*)(OPD)->seed_data) == 0x0) || \
                           ((OPD)->seed_size == 2 && (*(uint64_t*)(OPD)->seed_data) == 0x0))

#define IS_SEED_EQUAL(OPD1,OPD2) (((OPD1)->seed_size == 1 && (OPD2)->seed_size == 1 &&  \
                                   (*(uint32_t*)(OPD1)->seed_data) == (*(uint32_t*)(OPD2)->seed_data)) || \
                                  ((OPD1)->seed_size == 2 && (OPD2)->seed_size == 2 &&  \
                                   (*(uint64_t*)(OPD1)->seed_data) == (*(uint64_t*)(OPD2)->seed_data)))

void FPAnalysisPointer::handleAndOp(FPSV *output, FPSV *input1, FPSV *input2)
{
    // set to zero
    if (IS_0_CONST(input1) || IS_0_CONST(input2)) {
        output->setToZero();

    // absolute value
    } else if (IS_7FF_CONST(input1)) {
        output->doUnaryOp(OP_ABS, input2);
    } else if (IS_7FF_CONST(input2)) {
        output->doUnaryOp(OP_ABS, input1);

    // extract sign bit (preserve seed data)
    } else if (IS_800_CONST(input1)) {
        if (input2->isNegative()) {
            output->setToNegativeBitFlag();
        } else {
            output->setToPositiveBitFlag();
        }
        output->setSeedData();
    } else if (IS_800_CONST(input2)) {
        if (input1->isNegative()) {
            output->setToNegativeBitFlag();
        } else {
            output->setToPositiveBitFlag();
        }
        output->setSeedData();

    // move/copy
    } else if (IS_SEED_EQUAL(input1, input2)) {
        convertSV(output, input1);

    // set to zero (fp version)
    } else if (input1->isBitwiseZero() || input2->isBitwiseZero()) {
        output->setToZero();

    // move/copy (fp version)
    } else if (input1->compare(input2) == 0) {
        convertSV(output, input1);

    } else {
        cout << "ERROR: unhandled bitwise operation (" << input1->toDetailedString()
             << " AND " << input2->toDetailedString() << ")" << endl;
    }
}

void FPAnalysisPointer::handleOrOp(FPSV *output, FPSV *input1, FPSV *input2)
{
    // set sign bit
    if (IS_800_CONST(input1)) {
        output->doUnaryOp(OP_ABS, input2);
        output->doUnaryOp(OP_NEG, output);
    } else if (IS_800_CONST(input2)) {
        output->doUnaryOp(OP_ABS, input1);
        output->doUnaryOp(OP_NEG, output);

    // move/copy
    } else if (IS_0_CONST(input1)) {
        convertSV(output, input2);
    } else if (IS_0_CONST(input2)) {
        convertSV(output, input1);
    } else if (IS_SEED_EQUAL(input1,input2)) {
        convertSV(output, input1);

    // move/copy (fp version)
    } else if (input1->isBitwiseZero()) {
        convertSV(output, input2);
    } else if (input2->isBitwiseZero()) {
        convertSV(output, input1);
    } else if (input1->compare(input2) == 0) {
        convertSV(output, input1);

    } else {
        cout << "ERROR: unhandled bitwise operation (" << input1->toDetailedString()
             << " OR " << input2->toDetailedString() << ")" << endl;
    }
}

void FPAnalysisPointer::handleXorOp(FPSV *output, FPSV *input1, FPSV *input2)
{
    // set to zero
    if (IS_SEED_EQUAL(input1, input2)) {
        output->setToZero();

    // invert sign bit
    } else if (IS_800_CONST(input1)) {
        output->doUnaryOp(OP_NEG, input2);
    } else if (IS_800_CONST(input2)) {
        output->doUnaryOp(OP_NEG, input1);

    // move/copy
    } else if (IS_0_CONST(input1)) {
        convertSV(output, input2);
    } else if (IS_0_CONST(input2)) {
        convertSV(output, input1);
    
    // set to zero (fp version)
    } else if (input1->compare(input2) == 0) {
        output->setToZero();

    } else {
        cout << "ERROR: unhandled bitwise operation (" << input1->toDetailedString()
             << " XOR " << input2->toDetailedString() << ")" << endl;
    }
}

void FPAnalysisPointer::handleAm(FPSV *input)
{
    unsigned fsw = 0;
    bool norm = false, nan = false, inf = false, neg = false, zero = false;

    if (context->isX87StackEmpty()) {
        fsw |= FPContext::FP_X87_FLAG_C3;
        fsw |= FPContext::FP_X87_FLAG_C0;
    } else {
        zero = input->isZero();
        if (zero) {
            fsw |= FPContext::FP_X87_FLAG_C3;
        } else {
            norm = input->isNormal();
            if (norm) {
                fsw |= FPContext::FP_X87_FLAG_C2;
            } else {
                nan = input->isNaN();
                inf = input->isInf();
                if (nan) {
                    fsw |= FPContext::FP_X87_FLAG_C0;
                } else if (inf) {
                    fsw |= (FPContext::FP_X87_FLAG_C0 | FPContext::FP_X87_FLAG_C2);
                }
            }
            neg = input->isNegative();
            if (neg) {
                fsw |= FPContext::FP_X87_FLAG_C1;
            }
        }
    }

#if INCLUDE_DEBUG
    if (FPAnalysisPointer::debugPrint) {
        cout << "FPAnalysisPointer examine " << input->toDetailedString() << ": ";
        cout << (norm ? "[norm] " : "");
        cout << (nan ? "[nan] " : ""); cout << (inf ? "[inf] " : "");
        cout << (neg ? "[neg] " : ""); cout << (zero ? "[zero] " : "");
        cout << endl;
    }
#endif

    context->fxsave_state->fsw &= (unsigned short)(~(FPContext::FP_X87_CLEAR_FLAGS));    // clear C0-C3
    context->fxsave_state->fsw |= (unsigned short)fsw;                // set C0, C1, C2, and C3 flags
}

void FPAnalysisPointer::handleCom(FPSV *input1, FPSV *input2, bool ordered)
{
    int cmp = 0;
    unsigned long fsw = 0;

    // TODO: use "ordered"
    if (ordered) {}

    if (input1->isNaN() || input2->isNaN()) {
        fsw = FPContext::FP_X87_FLAGS_UNORDERED;
    } else {
        cmp = input1->compare(input2);
        cout << "cmp=" << cmp << "  ";
        if (cmp == 0) {
            fsw = FPContext::FP_X87_FLAGS_EQUAL;
        } else if (cmp > 0) {
            fsw = FPContext::FP_X87_FLAGS_GREATER;
        } else /* if (cmp < 0) */ {
            fsw = FPContext::FP_X87_FLAGS_LESS;
        }
    }

#if INCLUDE_DEBUG
    if (FPAnalysisPointer::debugPrint) {
        cout << "FPAnalysisPointer " << (ordered ? "ordered " : "unordered ");
        cout << "compare: " << input1->toDetailedString() << " <=>" << endl;
        cout << "     " << input2->toDetailedString() << "      ";
        if (fsw == FPContext::FP_X87_FLAGS_UNORDERED) {
            cout << "(unordered)  ";
        } else if (fsw == FPContext::FP_X87_FLAGS_EQUAL) {
            cout << "(equal)  ";
        } else if (fsw == FPContext::FP_X87_FLAGS_GREATER) {
            cout << "(greater)  ";
        } else if (fsw == FPContext::FP_X87_FLAGS_LESS) {
            cout << "(less)  ";
        } else {
            cout << "(unknown?)  ";
        }
        cout << endl;
    }
#endif

    context->fxsave_state->fsw &= (unsigned short)(~(FPContext::FP_X87_CLEAR_FLAGS));    // clear C0-C3
    context->fxsave_state->fsw |= (unsigned short)fsw;                // set C0, C1, C2, and C3 flags
}

void FPAnalysisPointer::handleComI(FPSV *input1, FPSV *input2, bool ordered)
{
    int cmp = 0;
    unsigned long flags = 0;

    // TODO: use "ordered"
    if (ordered) {}

    if (input1->isNaN() || input2->isNaN()) {
        flags = FPContext::FP_FLAGS_UNORDERED;
    } else {
        cmp = input1->compare(input2);
        if (cmp == 0) {
            flags = FPContext::FP_FLAGS_EQUAL;
        } else if (cmp > 0) {
            flags = FPContext::FP_FLAGS_GREATER;
        } else /* if (cmp < 0) */ {
            flags = FPContext::FP_FLAGS_LESS;
        }
    }

#if INCLUDE_DEBUG
    if (FPAnalysisPointer::debugPrint) {
        cout << "FPAnalysisPointer " << (ordered ? "ordered " : "unordered ");
        cout << "compare (rflags): " << input1->toDetailedString() << " <=>" << endl;
        cout << "     " << input2->toDetailedString() << "      ";
        if (flags == FPContext::FP_FLAGS_UNORDERED) {
            cout << "(unordered)  ";
        } else if (flags == FPContext::FP_FLAGS_EQUAL) {
            cout << "(equal)  ";
        } else if (flags == FPContext::FP_FLAGS_GREATER) {
            cout << "(greater)  ";
        } else if (flags == FPContext::FP_FLAGS_LESS) {
            cout << "(less)  ";
        } else {
            cout << "(unknown?)  ";
        }
        cout << endl;
    }
#endif

    context->reg_eflags &= ~(FPContext::FP_CLEAR_FLAGS);    // clear OF, SF, and AF flags
    context->reg_eflags |= flags;                // set ZF, PF, and CF flags
}

void FPAnalysisPointer::handleCmp(FPSV *output, FPSV *input1, FPSV *input2, bool ordered)
{
    uint8_t flag = currFlag->data.uint8;
    int cmp;
    bool result = false;

    // TODO: use "ordered"
    if (ordered) {}

    if (input1->isNaN() || input2->isNaN()) {
        if (flag >= 0x3 && flag <= 0x6) {
            result = true;
        } else {
            result = false;
        }
    } else {
        cmp = input1->compare(input2);
        switch (flag) {
            case 0x0: result = (cmp == 0); break;
            case 0x1: result = (cmp  < 0); break;
            case 0x2: result = (cmp <= 0); break;
            case 0x3: result = false;      break;
            case 0x4: result = (cmp != 0); break;
            case 0x5: result = (cmp >= 0); break;
            case 0x6: result = (cmp  > 0); break;
            case 0x7: result = true;       break;
        }
    }

#if INCLUDE_DEBUG
    if (FPAnalysisPointer::debugPrint) {
        cout << "FPAnalysisPointer " << (ordered ? "ordered " : "unordered ");
        cout << "cmp: " << input1->toDetailedString() << " <=>" << endl;
        cout << "     " << input2->toDetailedString() << "      ";
        cout << " [" << flag << "] ";
        if (result) {
            cout << "(true)  ";
        } else {
            cout << "(false)  ";
        }
        cout << endl;
    }
#endif

    if (result) {
        output->setToCMPTrue();
    } else {
        output->setToCMPFalse();
    }
}

float FPAnalysisPointer::handleUnaryReplacedFunc(FPOperationType type, float input)
{
    FPSV *inputVal = getOrCreateSV(input);

    FPSV *resultVal = createSV(inputVal->type, 0);
    resultVal->doUnaryOp(type, inputVal);

    if (sizeof(float) >= sizeof(void*)) {
        return *(float*)(resultVal);
    } else {
        return resultVal->getValue(IEEE_Single).data.flt;
    }
}

double FPAnalysisPointer::handleUnaryReplacedFunc(FPOperationType type, double input)
{
    FPSV *inputVal = getOrCreateSV(input);

    FPSV *resultVal = createSV(inputVal->type, 0);
    resultVal->doUnaryOp(type, inputVal);

    double rval = 0.0;
    *(FPSV**)(&rval) = resultVal;
    return rval;
}

long double FPAnalysisPointer::handleUnaryReplacedFunc(FPOperationType type, long double input)
{
    FPSV *inputVal = getOrCreateSV(input);

    FPSV *resultVal = createSV(inputVal->type, 0);
    resultVal->doUnaryOp(type, inputVal);

    long double rval = 0.0L;
    *(FPSV**)(&rval) = resultVal;
    return rval;
}

float FPAnalysisPointer::handleBinaryReplacedFunc(FPOperationType type, float input1, float input2)
{
    FPSV *input1Val = getOrCreateSV(input1);
    FPSV *input2Val = getOrCreateSV(input2);

    assert(input1Val->type == input2Val->type);
    FPSV *resultVal = createSV(input1Val->type, 0);
    resultVal->doBinaryOp(type, input1Val, input2Val);

    if (sizeof(float) >= sizeof(void*)) {
        return *(float*)(resultVal);
    } else {
        return resultVal->getValue(IEEE_Single).data.flt;
    }
}

double FPAnalysisPointer::handleBinaryReplacedFunc(FPOperationType type, double input1, double input2)
{
    FPSV *input1Val = getOrCreateSV(input1);
    FPSV *input2Val = getOrCreateSV(input2);

    assert(input1Val->type == input2Val->type);
    FPSV *resultVal = createSV(input1Val->type, 0);
    resultVal->doBinaryOp(type, input1Val, input2Val);

    double rval = 0.0;
    *(FPSV**)(&rval) = resultVal;
    return rval;
}

long double FPAnalysisPointer::handleBinaryReplacedFunc(FPOperationType type, long double input1, long double input2)
{
    FPSV *input1Val = getOrCreateSV(input1);
    FPSV *input2Val = getOrCreateSV(input2);

    assert(input1Val->type == input2Val->type);
    FPSV *resultVal = createSV(input1Val->type, 0);
    resultVal->doBinaryOp(type, input1Val, input2Val);

    long double rval = 0.0L;
    *(FPSV**)(&rval) = resultVal;
    return rval;
}

// example of custom handler

#define COMMON_SINCOS_CODE(TYPE, MEMBER)    \
    FPSV *inputVal = getOrCreateSV(input); \
    FPSV *resultSinVal = createSV(inputVal->type, 0); \
    FPSV *resultCosVal = createSV(inputVal->type, 0); \
    resultSinVal->doUnaryOp(OP_SIN, inputVal); \
    resultCosVal->doUnaryOp(OP_COS, inputVal); \
    if (sizeof(input) >= sizeof(void*)) { \
        *(void**)(sin_output) = resultSinVal; \
        *(void**)(cos_output) = resultCosVal; \
    } else { \
        *sin_output = resultSinVal->getValue(TYPE).data.MEMBER; \
        *cos_output = resultCosVal->getValue(TYPE).data.MEMBER; \
    }

void FPAnalysisPointer::handleReplacedFuncSinCos(float input, float *sin_output, float *cos_output)
{ COMMON_SINCOS_CODE(IEEE_Single, flt); }
void FPAnalysisPointer::handleReplacedFuncSinCos(double input, double *sin_output, double *cos_output)
{ COMMON_SINCOS_CODE(IEEE_Double, dbl); }
void FPAnalysisPointer::handleReplacedFuncSinCos(long double input, long double *sin_output, long double *cos_output)
{ COMMON_SINCOS_CODE(C99_LongDouble, ldbl); }

#define COMMON_MODF_CODE(RTYPE, TYPE, MEMBER)    \
    RTYPE rval = 0.0; \
    FPSV *inputVal = getOrCreateSV(input); \
    FPSV *resultIntVal = createSV(inputVal->type, 0); \
    FPSV *resultFracVal = createSV(inputVal->type, 0); \
    resultFracVal->doModF(inputVal, resultIntVal); \
    if (sizeof(input) >= sizeof(void*)) { \
        *(void**)(int_output) = resultIntVal; \
        *(void**)(&rval) = resultFracVal; \
    } else { \
        *int_output = resultIntVal->getValue(TYPE).data.MEMBER; \
        rval = resultFracVal->getValue(TYPE).data.MEMBER; \
    } \
    return rval

float FPAnalysisPointer::handleReplacedFuncModF(float input, float *int_output)
{ COMMON_MODF_CODE(float, IEEE_Single, flt); }
double FPAnalysisPointer::handleReplacedFuncModF(double input, double *int_output)
{ COMMON_MODF_CODE(double, IEEE_Double, dbl); }
long double FPAnalysisPointer::handleReplacedFuncModF(long double input, long double *int_output)
{ COMMON_MODF_CODE(long double, C99_LongDouble, ldbl); }

#define COMMON_FREXP_CODE(RTYPE, TYPE, MEMBER)    \
    RTYPE rval = 0.0; \
    FPSV *inputVal = getOrCreateSV(input); \
    FPSV *resultSigVal = createSV(inputVal->type, 0); \
    resultSigVal->doFrExp(inputVal, exp_output); \
    if (sizeof(input) >= sizeof(void*)) { \
        *(void**)(&rval) = resultSigVal; \
    } else { \
        rval = resultSigVal->getValue(TYPE).data.MEMBER; \
    } \
    return rval

float FPAnalysisPointer::handleReplacedFuncFrExp(float input, int *exp_output)
{ COMMON_FREXP_CODE(float, IEEE_Single, flt); }
double FPAnalysisPointer::handleReplacedFuncFrExp(double input, int *exp_output)
{ COMMON_FREXP_CODE(double, IEEE_Double, dbl); }
long double FPAnalysisPointer::handleReplacedFuncFrExp(long double input, int *exp_output)
{ COMMON_FREXP_CODE(long double, C99_LongDouble, ldbl); }

#define COMMON_LDEXP_CODE(RTYPE, TYPE, MEMBER)    \
    RTYPE rval = 0.0; \
    FPSV *inputSigVal = getOrCreateSV(sig_input); \
    FPSV *resultVal = createSV(inputSigVal->type, 0); \
    resultVal->doLdExp(inputSigVal, exp_input); \
    if (sizeof(sig_input) >= sizeof(void*)) { \
        *(void**)(&rval) = resultVal; \
    } else { \
        rval = resultVal->getValue(TYPE).data.MEMBER; \
    } \
    return rval

float FPAnalysisPointer::handleReplacedFuncLdExp(float sig_input, int exp_input)
{ COMMON_LDEXP_CODE(float, IEEE_Single, flt); }
double FPAnalysisPointer::handleReplacedFuncLdExp(double sig_input, int exp_input)
{ COMMON_LDEXP_CODE(double, IEEE_Double, dbl); }
long double FPAnalysisPointer::handleReplacedFuncLdExp(long double sig_input, int exp_input)
{ COMMON_LDEXP_CODE(long double, C99_LongDouble, ldbl); }

bool FPAnalysisPointer::isSVPtr(void *ptr) {
#ifdef USE_GARBAGE_COLLECTOR
    return ((ptr) != NULL) && (GC_base(ptr) != NULL);
#else
#ifdef USE_LIVE_PTR_LIST
    unsigned long idx;
    FPLivePtrList list;
    bool isLive = false;
    idx = (unsigned long)ptr % LIVE_PTR_TBL_SIZE;
    list = livePointers[idx];
    while (list != NULL && list->ptr != ptr)
        list = list->next;
    if (list != NULL) {
        isLive = true;
    } 
    //printf("isSVPtr(%p)=%s\n", ptr, isLive ? "true" : "false");
    return isLive;
#else
    return (false && ptr==NULL); // latter check is to appease -Werror
#endif
#endif
}

FPSV* FPAnalysisPointer::getOrCreateSV(float value)
{
    void *addr = *(void**)(&value);
    FPSV *ptr = NULL;
    FPOperandValue val;
    val.type = IEEE_Single;
    val.data.flt = value;
    if (sizeof(float) >= sizeof(void*) && isSVPtr(addr)) {
        ptr = (FPSV*)addr;
    } else {
        ptr = createSV(mainPolicy->getSVType(), addr);
        ptr->setValue(val);
    }
    return ptr;
}

FPSV* FPAnalysisPointer::getOrCreateSV(double value)
{
    void *addr = *(void**)(&value);
    FPSV *ptr = NULL;
    FPOperandValue val;
    val.type = IEEE_Double;
    val.data.dbl = value;
    if (isSVPtr(addr)) {
        ptr = (FPSV*)addr;
    } else {
        ptr = createSV(mainPolicy->getSVType(), addr);
        ptr->setValue(val);
    }
    return ptr;
}

FPSV* FPAnalysisPointer::getOrCreateSV(long double value)
{
    void *addr = *(void**)(&value);
    FPSV *ptr = NULL;
    FPOperandValue val;
    val.type = C99_LongDouble;
    val.data.ldbl = value;
    if (isSVPtr(addr)) {
        ptr = (FPSV*)addr;
    } else {
        ptr = createSV(mainPolicy->getSVType(), addr);
        ptr->setValue(val);
    }
    return ptr;
}

FPSV* FPAnalysisPointer::getOrCreateSV(FPOperand *op)
{
    void *cptr = NULL;
    FPSV *ptr = NULL;
    if (op->isImmediate()) {
        ptr = createSV(mainPolicy->getSVType(op, currInst), op->getCurrentAddress());
        ptr->setValue(op);
    } else {
        cptr = op->getCurrentValuePtr();
        if (isSVPtr(cptr)) {
            ptr = (FPSV*)cptr;
        } else {
            ptr = createSV(mainPolicy->getSVType(op, currInst), op->getCurrentAddress());
            ptr->setValue(op);
        }
    }
    return ptr;
}

/* shadow value factory method */
FPSV* FPAnalysisPointer::createSV(FPSVType type, FPOperandAddress addr)
{
    FPSV *val = NULL;
    switch (type) {
        case SVT_IEEE_Single:
#ifdef USE_GARBAGE_COLLECTOR
            val = new (GC) FPSVSingle(addr);
            num_gcAllocations++;
            num_allocations++;
#else
            val = new FPSVSingle(addr);
            num_allocations++;
#endif
            break;
        case SVT_IEEE_Double:
#ifdef USE_GARBAGE_COLLECTOR
            val = new (GC) FPSVDouble(addr);
            num_gcAllocations++;
            num_allocations++;
#else
            val = new FPSVDouble(addr);
            num_allocations++;
#endif
            break;
        default:
            fprintf(stderr, "ERROR: invalid shadow value type in FPAnalysisPointer::createSV");
            break;
    }
    assert(val != NULL);

#ifdef USE_LIVE_PTR_LIST
    // update live pointer list
    unsigned long idx = (unsigned long)val % LIVE_PTR_TBL_SIZE;
    FPLivePtrList list = livePointers[idx];
    FPLivePtrList newPtr = (FPLivePtrList)malloc(sizeof(struct FPLivePtr));
    if (!newPtr) {
        fprintf(stderr, "ERROR: Out of memory!\n");
        assert(0);
        return NULL;
    }
    //printf("created live ptr  num=%p  live ptr=%p\n", val, newPtr);
    newPtr->ptr = (void*)val;
    newPtr->next = NULL;
    FPLivePtrList prev = NULL;
    if (list == NULL) {
        livePointers[idx] = newPtr;
    } else {
        while (list->next != NULL) {
            prev = list;
            list = list->next;
        }
        list->next = newPtr;
    }
#endif

    return val;
}

/* convert between shadow value types */
void FPAnalysisPointer::convertSV(FPSV *dest, FPSV *src)
{
    // TODO: finish as necessary
    switch (dest->type) {

        case SVT_IEEE_Single:
            switch (src->type) {
                case SVT_IEEE_Single: 
                    ((FPSVSingle*)dest)->value = ((FPSVSingle*)src)->value;
                    break;
                case SVT_IEEE_Double: 
                    ((FPSVSingle*)dest)->value = (float)((FPSVDouble*)src)->value;
                    break;
                default:
                    fprintf(stderr, "ERROR: invalid shadow value type in FPAnalysisPointer::convertSV (dest=SVT_IEEE_Single)");
                    break;
            }
            break;

        case SVT_IEEE_Double:
            switch (src->type) {
                case SVT_IEEE_Single: 
                    ((FPSVDouble*)dest)->value = (double)((FPSVSingle*)src)->value;
                    break;
                case SVT_IEEE_Double: 
                    ((FPSVDouble*)dest)->value = ((FPSVDouble*)src)->value;
                    break;
                default:
                    fprintf(stderr, "ERROR: invalid shadow value type in FPAnalysisPointer::convertSV (dest=SVT_IEEE_Double)");
                    break;
            }
            break;

        default:
            fprintf(stderr, "ERROR: invalid shadow value type in FPAnalysisPointer::convertSV");
            break;
    }
}

// save the pointer to the operand via the context
void FPAnalysisPointer::saveSVPtrToContext(FPOperand *output, FPSV *value, FPContext *context)
{
#ifdef INCLUDE_DEBUG
    if (FPAnalysisPointer::debugPrint) {
        FPOperandValue val = value->getValue(IEEE_Double);
        cout << "saving pointer to context: " 
             << val.data.dbl << " [" << value << "] => " << output->toString() << endl;
    }
#endif
    output->setCurrentValuePtr((void*)value, context, true, true);
    num_ptrWriteBacks++;
}

#define SV_SAVE_VALUE(POSTFIX, TYPE, MEMBER) output->setCurrentValue##POSTFIX((TYPE)val.data.MEMBER, context, true, true)

// save the shadow value to the given operand (coerced to its type) 
// via the context
void FPAnalysisPointer::saveSVValueToContext(FPOperand *output, FPSV *value, FPContext *context)
{
    FPOperandValue val = value->getValue(output->currentValue.type);
#ifdef INCLUDE_DEBUG
    if (FPAnalysisPointer::debugPrint) {
        cout << "saving value to context: " 
             << FPOperand::FPOpVal2Str(&val) << " [" << value << "] => " << output->toString()
             << "   " << currInst->getDisassembly() << endl;
    }
#endif
    switch (val.type) {
        case SignedInt8:     SV_SAVE_VALUE(SInt8,    int8_t,  sint8);    break;
        case UnsignedInt8:   SV_SAVE_VALUE(UInt8,   uint8_t,  uint8);    break;
        case SignedInt16:    SV_SAVE_VALUE(SInt16,  int16_t, sint16);    break;
        case UnsignedInt16:  SV_SAVE_VALUE(UInt16, uint16_t, uint16);    break;
        case SignedInt32:    SV_SAVE_VALUE(SInt32,  int32_t, sint32);    break;
        case UnsignedInt32:  SV_SAVE_VALUE(UInt32, uint32_t, uint32);    break;
        case SignedInt64:    SV_SAVE_VALUE(SInt64,  int64_t, sint64);    break;
        case UnsignedInt64:  SV_SAVE_VALUE(UInt64, uint64_t, uint64);    break;
        case IEEE_Single:    SV_SAVE_VALUE(F,   float,        flt  );    break;
        case IEEE_Double:    SV_SAVE_VALUE(D,   double,       dbl  );    break;
        case C99_LongDouble: SV_SAVE_VALUE(LD,  long double,  ldbl );    break;

        // TODO: what to do about quads?
        case SSE_Quad:       SV_SAVE_VALUE(UInt32, uint32_t, uint32);    break;
    }
    num_valWriteBacks++;
}

long FPAnalysisPointer::getNumAllocations() {
    return num_allocations;
}

long FPAnalysisPointer::getNumGCAllocations() {
    return num_gcAllocations;
}

long FPAnalysisPointer::getNumValueWriteBacks() {
    return num_valWriteBacks;
}

long FPAnalysisPointer::getNumPtrWriteBacks() {
    return num_ptrWriteBacks;
}

#ifdef USE_LIVE_PTR_LIST
void FPAnalysisPointer::printLivePtrTableHealth()
{
    static const unsigned long /*SAMPLES = 100000,*/ BINS = 10;
    unsigned long histogram[BINS];
    unsigned long max_ptrs_per_bin = 20, ptrs_per_bin, total_ptrs = 0, empty_lists = 0, bin, idx;
    FPLivePtrList lpl;
    // initialize histogram memory
    for (bin = 0; bin < BINS; bin++) {
        histogram[bin] = 0;
    }
    // sample a few bins to find approximate max ptrs per bin
    // for histogram binning scale
    for (idx = 0; idx < LIVE_PTR_TBL_SIZE; idx+=LIVE_PTR_TBL_SIZE/1000) {
        lpl = livePointers[idx];
        ptrs_per_bin = 0;
        if (lpl != NULL) {
            ptrs_per_bin++;
            while (lpl->next != NULL) {
                ptrs_per_bin++;
                lpl = lpl->next;
            }
        }
        if (ptrs_per_bin > max_ptrs_per_bin) {
            max_ptrs_per_bin = ptrs_per_bin;
        }
    }
    printf(" max ptrs per bin: %lu\n", max_ptrs_per_bin);
    // sample many bins to build distribution
    //for (idx = 0; idx < LIVE_PTR_TBL_SIZE; idx+=LIVE_PTR_TBL_SIZE/SAMPLES) {
    for (idx = 0; idx < LIVE_PTR_TBL_SIZE; idx++) {
        lpl = livePointers[idx];
        ptrs_per_bin = 0;
        if (lpl != NULL) {
            ptrs_per_bin++;
            while (lpl->next != NULL) {
                ptrs_per_bin++;
                lpl = lpl->next;
            }
        }
        total_ptrs += ptrs_per_bin;
        if (ptrs_per_bin == 0) {
            empty_lists++;
        } else {
            //bin = (unsigned long)(((double)ptrs_per_bin / (double)max_ptrs_per_bin) * (double)BINS);
            bin = ptrs_per_bin / (max_ptrs_per_bin * BINS);
            if (bin >= BINS) {
                bin = BINS-1;
            }
            histogram[bin]++;
        }
    }
    // print distribution histogram
    //printf("  total live ptrs: %lu\n", countLivePointers());
    printf("  total live ptrs: %lu\n", total_ptrs);
    printf("  empty ptr lists: %lu / %lu (%d%%)\n", empty_lists, LIVE_PTR_TBL_SIZE,
            (int)((double)empty_lists / (double)LIVE_PTR_TBL_SIZE * 100.0));
    ptrs_per_bin = max_ptrs_per_bin / BINS;
    for (bin = 0; bin < BINS-1; bin++) {
        printf("  bin %lu-%lu: %lu (%d%%)\n", (bin==0 ? 1 : bin*ptrs_per_bin), 
                (bin+1)*ptrs_per_bin-1, histogram[bin],
                (int)((double)histogram[bin] / double(LIVE_PTR_TBL_SIZE) * 100.0));
    }
    printf("  bin %lu-inf: %lu\n", (BINS-1)*ptrs_per_bin, histogram[BINS-1]);
}
#endif

// print shadow entries by location; tries to use debug info to provide better output
void FPAnalysisPointer::printShadowLocation(FPOperandAddress addr)
{
    //cout << "  printing shadow location: addr=" << hex << addr << dec << endl;

    string name("");
    Type *type;
    long asize, isize;

    // get debug info
    name = logFile->getGlobalVarName(addr);
    type = logFile->getGlobalVarType(addr);

    if (type && type->getArrayType()) {
        // debug info says it's an array
        asize = type->getSize();
        isize = logFile->getGlobalVarSize(addr);
        printShadowMatrix(name, addr, isize, asize/isize, 1);
    } else if (type) {
        // not an array, but we still have the type info
        printShadowEntry(name, addr, type->getSize());
    } else {
        // no idea what it is
        printShadowEntry(name, addr, 0);
    }
}

// print shadow matrix by base location (does not access debug info)
void FPAnalysisPointer::printShadowMatrix(string baseName, FPOperandAddress baseAddr, 
        unsigned long isize, unsigned long rows, unsigned long cols)
{
    //cout << "  printing shadow matrix: name=" << baseName << " addr=" << hex << baseAddr << dec;
    //cout << " isize=" << isize << " size=(" << rows << "," << cols << ")" << endl;

    FPOperandAddress iaddr;
    unsigned long r, c;
    stringstream sname;
    for (r = 0; r < rows; r++) {
        for (c = 0; c < cols; c++) {
            sname.clear();
            sname.str("");
            sname << baseName << "(" << r ;
            if (cols > 1) {
                sname << "," << c;
            }
            sname << ")";
            iaddr = (FPOperandAddress)((unsigned long)baseAddr + r*cols*isize + c*isize);
            printShadowEntry(sname.str(), iaddr, isize);
        }
    }
}

// print a single table entry (does not access debug info)
void FPAnalysisPointer::printShadowEntry(string name, FPOperandAddress addr, unsigned long size)
{
    //cout << "  printing shadow value entry: name=" << name << " addr=" << addr << " size=" << size << endl;

    //mpfr_t tableVal;
    long double ldbl;
    FPSV *num;
    stringstream logDetails;
    char tempBuffer[2048];

    // retrieve and check for valid shadow table entry
    FPSV* sval;
    sval = *(FPSV**)addr;
    num = NULL;
    if (sval != NULL && isSVPtr(sval)) {
        num = (FPSV*)sval;
    }

#if INCLUDE_DEBUG
    if (debugPrint) {
        if (!num) {
            cout << "  NO shadow value ";
        } else {
            cout << "  found shadow value ";
        }
        cout << " for variable \"" << name << "\" ";
        cout << "  addr=" << addr;
        cout << "  sval=" << (void*)sval;
        cout << "  num=" << (void*)num;
        cout << endl;
        cout.flush();
    }
#endif
    if (num == NULL) {
        return;
    }

    // assumptions (use address instead of name and double if size is not
    // present)
    if (name == "") {
        sprintf(tempBuffer, "%p", addr);
        name = string(tempBuffer);
    }
    if (size == 0) {
        size = 8;
    }

    // only print entries with pretty names (get rid of this check to print everything)
    if (name != "") {

        logDetails.clear();
        logDetails.str("");

        // grab and output the table value
        ldbl = num->getValue(C99_LongDouble).data.ldbl;
        sprintf(tempBuffer, "%.2Le", ldbl);
#if INCLUDE_DEBUG
        if (debugPrint) {
            cout << "    value: " << tempBuffer << endl;
            cout.flush();
        }
#endif
        logDetails << "label=" << tempBuffer << endl;
        
        // grab quad-precision version of the table value and output it
        //mpfr_init2(tableVal, DEFAULT_MPFR_PREC);
        //SVDT_GET_MPFR(tableVal, num);
        //mpfr_sprintf(tempBuffer, "%.35Rf", tableVal);
        logDetails << "value=" << num->toDetailedString() << endl;

        logDetails << "system value=0.0" << endl;
        logDetails << "abs error=0.0" << endl;
        logDetails << "rel error=0.0" << endl;
        logDetails << "address=0x" << hex << (unsigned long)addr << dec << endl;
        logDetails << "size=" << size << endl;
        logDetails << "type=" << num->getTypeString() << endl;

#if INCLUDE_DEBUG
        if (debugPrint) {
            cout << logDetails.str() << endl;
        }
#endif
        logFile->addMessage(SHVALUE, 0, name, logDetails.str(), "", NULL);
    }


    // don't output this entry again
    //doneAlready.push_back(addr);
}

void FPAnalysisPointer::finalOutput()
{
    vector<FPShadowEntry*>::iterator k;
    FPShadowEntry* entry;
    FPOperandAddress addr, maddr;
    size_t j, n, r, c, size;
    stringstream outputString;

    // shadow table output initialization
    outputString.clear();
    outputString.str("");
    doneAlready.clear();
    
    // summary output
    //cout << "finalizing pointer analysis" << endl;
    outputString << num_allocations << " unique shadow values" << endl;
    outputString << shadowEntries.size() << " config-requested shadow values" << endl;

    // config-requested shadow values
    for (k=shadowEntries.begin(); k!=shadowEntries.end(); k++) {
        entry = *k;
        if (entry->type == REPORT || entry->type ==  NOREPORT) {
            // look up variable name
            addr = logFile->getGlobalVarAddr(entry->name);
            if (addr == 0) {
                n = sscanf(entry->name.c_str(), "0x%lx", &j);
                if (n > 0) {
                    // it's a hex address
                    addr = (FPOperandAddress)j;
                } else {
                    // invalid entry
                    addr = 0;
                }
            }
#if INCLUDE_DEBUG
            if (debugPrint) {
                cout << "name=" << entry->name << "  addr=" << addr << endl;
            }
#endif
            if (addr != 0) {
                if (entry->isize == 0) {
                    // if no individual entry size is given, try to find one
                    // from debug information
                    size = logFile->getGlobalVarSize(addr);
                } else {
                    size = entry->isize;
                }
                if (entry->indirect) {
                    // pointer -- apply indirection
                    context->getMemoryValue((void*)&maddr, addr, sizeof(void*));
                    //cout << "  indirect (" << hex << addr << " -> " << maddr << dec << ")";
                    if (maddr) {
                        addr = maddr;
                        if (size == 0) {
                            size = logFile->getGlobalVarSize(addr);
                        }
                    }
                }
                if (entry->type == NOREPORT) {
                    // skip this entry
                    //cout << "skipping " << entry->name << " addr=" << hex << addr << dec << endl;
                    doneAlready.push_back(addr);
                    continue;
                }
                r = entry->size[0];
                c = entry->size[1];
                if (r == 0 && c == 0 && size != 0 && logFile->getGlobalVarType(addr)) {
                    // no dimensions provided -- try to deduce a one-dimensional
                    // size from debug information
                    r = 1;
                    //cout << logFile->getGlobalVarType(addr)->getSize() << " / " << size << endl;
                    c = (unsigned)logFile->getGlobalVarType(addr)->getSize() / size;
                }
#if INCLUDE_DEBUG
                if (debugPrint) {
                    cout << "  config entry: name=" << entry->name << " isize=" << size;
                    cout << " size=(" << entry->size[0] << "," << entry->size[1] << ")";
                    cout << " size=(" << r << "," << c << ")";
                    cout << " " << (entry->indirect ? "[indirect]" : "[direct]") << endl;
                }
#endif
                if (r > 1 || c > 1) {
                    printShadowMatrix(entry->name, addr, size, r, c);
                } else if (r > 0 || c > 0) {
                    printShadowEntry(entry->name, addr, size);
                } else {
                    // can't determine dimensions -- skip this entry
                    doneAlready.push_back(addr);
                }
            }
        }
    }

    if (reportAllGlobals) {
        // find and report global variables
        //cout << "reporting global variables" << endl;
        vector<Variable *> vars;
        vector<Variable *>::iterator v;
        Type *type;
        Variable *var;
        Symbol *sym;
        logFile->getGlobalVars(vars);
        for (v = vars.begin(); v != vars.end(); v++) {
            var = *v;
            type = var->getType();
            if (type && (type->getName() == "float" || 
                         type->getName() == "double" ||
                         type->getName() == "long double")) {
                sym = var->getFirstSymbol();
                //cout << "global: " << sym->getPrettyName();
                //cout << " (" << type->getName() << ":" << type->getSize() << ")";
                //cout << endl;
                printShadowLocation((FPOperandAddress)sym->getOffset());
            }
        }
        // TODO: handle matrices
    }

    string label("Pointer-Replacement Shadow Values");
    string detail = outputString.str();

    logFile->addMessage(SUMMARY, 0, label, detail, "");
    //cout << outputString.str();

    //cout << "done with final output for pointer analysis" << endl;
}

}

