#include "FPAnalysisDCancel.h"

namespace FPInst {

extern "C" {
    FPAnalysisDCancel *_INST_Main_DCancelAnalysis = NULL;
}

bool FPAnalysisDCancel::existsInstance()
{
    return (_INST_Main_DCancelAnalysis != NULL);
}

FPAnalysisDCancel* FPAnalysisDCancel::getInstance()
{
    if (!_INST_Main_DCancelAnalysis) {
        _INST_Main_DCancelAnalysis = new FPAnalysisDCancel();
    }
    return _INST_Main_DCancelAnalysis;
}

FPAnalysisDCancel::FPAnalysisDCancel()
    : FPAnalysis()
{
    min_priority = 10;
    enable_sampling = true;
    numCancelAddresses = 0;
    instCount = 0;
    instData = NULL;
    expandInstData(4096);
}

string FPAnalysisDCancel::getTag()
{
    return "dcancel";
}

string FPAnalysisDCancel::getDescription()
{
    return "Cancellation Detection Analysis";
}

void FPAnalysisDCancel::configure(FPConfig *config, FPDecoder *decoder,
        FPLog *log, FPContext *context)
{
    FPAnalysis::configure(config, decoder, log, context);
    if (config->hasValue("min_priority")) {
        long priority = atol(config->getValueC("min_priority"));
        min_priority = priority;
        //status << "d_cancel: min_priority=" << priority << endl;
    }
    if (config->getValue("enable_sampling") == "no") {
        disableSampling();
        //status << "d_cancel: sampling disabled" << endl;
    }
    configAddresses(config);
    if (isRestrictedByAddress()) {
        //status << "d_cancel: addresses=" << listAddresses();
    }
}

void FPAnalysisDCancel::setMinPriority(long minPriority)
{
    min_priority = minPriority;
}

long FPAnalysisDCancel::getMinPriority()
{
    return min_priority;
}

bool FPAnalysisDCancel::isSamplingEnabled()
{
    return enable_sampling;
}

void FPAnalysisDCancel::enableSampling()
{
    enable_sampling = true;
}

void FPAnalysisDCancel::disableSampling()
{
    enable_sampling = false;
}

void FPAnalysisDCancel::configAddresses(FPConfig *config)
{
    if (config->hasValue("dcancel_addresses")) {
        numCancelAddresses = config->getAddressList(cancelAddresses, "dcancel_addresses");
        //printf("filtering by %d addresses\n", numCancelAddresses);
    } /*else {
        printf("no filtering\n");
    }*/
}

bool FPAnalysisDCancel::isRestrictedByAddress()
{
    return numCancelAddresses > 0;
}

string FPAnalysisDCancel::listAddresses() {
    stringstream ss;
    ss.clear();
    ss.str("");
    unsigned i;
    ss << hex;
    for (i=0; i<numCancelAddresses; i++) {
        ss << cancelAddresses[i] << " ";
    }
    return ss.str();
}

bool FPAnalysisDCancel::shouldPreInstrument(FPSemantics *inst)
{
    bool addOrSub = false, handle = false;
    FPOperation *op;
    FPOperationType opt;
    unsigned i;

    // is this an add or a subtract involving floating-point numbers?
    for (i=0; i<inst->numOps; i++) {
        op = (*inst)[i];
        opt = op->type;
        if ((opt == OP_ADD || opt == OP_SUB) &&
            (op->hasOperandOfType(IEEE_Single) ||
             op->hasOperandOfType(IEEE_Double) ||
             op->hasOperandOfType(C99_LongDouble))) {
            addOrSub = true;
            break;
        }
    }

    // does it involve any floating-point numbers?

    // if so, is this on the list of addresses to handle?
    if (addOrSub && numCancelAddresses > 0) {
        void* addr = inst->getAddress();
        unsigned i;
        bool found = false;
        for (i=0; i<numCancelAddresses; i++) {
            if (cancelAddresses[i] == addr) {
                found = true;
                break;
            }
        }
        if (found) {
            handle = true;
        } else {
            handle = false;
        }
    } else {
        handle = addOrSub;
    }

    return handle;
}

bool FPAnalysisDCancel::shouldPostInstrument(FPSemantics * /*inst*/)
{
    return false;
}

bool FPAnalysisDCancel::shouldReplace(FPSemantics * /*inst*/)
{
    return false;
}

Snippet::Ptr FPAnalysisDCancel::buildPreInstrumentation(FPSemantics * /*inst*/,
        BPatch_addressSpace * /*app*/, bool &needsRegisters)
{
    needsRegisters = true;
    return Snippet::Ptr();
}

Snippet::Ptr FPAnalysisDCancel::buildPostInstrumentation(FPSemantics * /*inst*/,
        BPatch_addressSpace * /*app*/, bool & /*needsRegisters*/)
{
    return Snippet::Ptr();
}

Snippet::Ptr FPAnalysisDCancel::buildReplacementCode(FPSemantics * /*inst*/,
        BPatch_addressSpace * /*app*/, bool & /*needsRegisters*/)
{
    return Snippet::Ptr();
}

void FPAnalysisDCancel::registerInstruction(FPSemantics * /*inst*/)
{ }

void FPAnalysisDCancel::handlePreInstruction(FPSemantics *inst)
{
    FPOperation *op;
    FPOperationType opt;
    FPOperandSet *ops;
    FPOperand **ops_in;
    unsigned i, j;
    size_t nOps;

    //printf("%s\n", inst->getDisassembly().c_str());

    // for each operation ...
    for (i=0; i<inst->numOps; i++) {
        op = (*inst)[i];
        opt = op->type;

        //op->refreshInputOperands(context);
        ops = op->opSets;
        nOps = op->numOpSets;

        // for each output (and set of inputs) ...
        for (j=0; j<nOps; j++) {
            ops_in = ops[j].in;
            ops_in[0]->refresh(context);
            ops_in[1]->refresh(context);

            // hand off operands to checking function
            checkForCancellation(inst, opt, ops_in[0], ops_in[1]);
        }
    }
}

void FPAnalysisDCancel::handlePostInstruction(FPSemantics * /*inst*/)
{ }

void FPAnalysisDCancel::handleReplacement(FPSemantics * /*inst*/)
{ }

void FPAnalysisDCancel::checkForCancellation(FPSemantics *inst, FPOperationType opt, FPOperand *op1, FPOperand *op2)
{
    static char label_buffer[1024];
    static char details_buffer[1024];
    static string trace;

    long double num1, num2, numr;
    long exp1, exp2, expr, priority;
    long idx, cancels;
    FPOperandValue result;

    // increment instruction count
    incrementCount(inst);

    // old checks to make sure instruction is a candidate for cancellation
    // (removed for efficiency--all this is now checked in the instrumenter)
    /*
     *if (opt != OP_ADD && opt != OP_SUB) return;
     *if (numCancelAddresses > 0) {
     *    void* addr = inst->getAddress();
     *    //printf("%x ", (unsigned int)addr);
     *    int i;
     *    bool found = false;
     *    for (i=0; i<numCancelAddresses; i++) {
     *        if (cancelAddresses[i] == addr) {
     *            found = true;
     *            break;
     *        }
     *    }
     *    //if (found) printf(" found!\n"); else printf(" not found!!\n");
     *    if (!found) return;
     *}
     */

    // extract operand information
    num1 = op1->getCurrentValueLD();
    num2 = op2->getCurrentValueLD();
    exp1 = op1->getCurrentValueExp();
    exp2 = op2->getCurrentValueExp();


    // skip irrelevant operations and simulate valid operations
    if (num1 == 0.0L || num2 == 0.0L)
        return;
    /*if (opt == OP_ADD && ((num1 < 0.0L && num2 < 0.0L) ||
                          (num1 > 0.0L && num2 > 0.0L)))
        return;
    else*/ if (opt == OP_ADD)
        op1->add(op2, &result);
    /*if (opt == OP_SUB && ((num1 < 0.0L && num2 > 0.0L) ||
                          (num1 > 0.0L && num2 < 0.0L)))
        return;*/
    else if (opt == OP_SUB)
        op1->sub(op2, &result);
    else {
        printf("unhandled operation: %.25Lg %c %.25Lg\n", 
                num1, (opt == OP_ADD ? '+' : '-'), num2);
        return;
    }
    numr = FPOperand::FPOpValLD(&result);

    /*
     *printf("OPERATION:  %.25Lg %c %.25Lg = %.25Lg\n", 
     *       num1, (opt == OP_ADD ? '+' : '-'), num2, numr);
     *printf("            %s %c %s\n", op1->toStringV().c_str(), 
     *       (opt == OP_ADD ? '+' : '-'), op2->toStringV().c_str());
     */

    // check for catastrophic cancellation (the result is zero but the operands
    // are not)
    if (numr == 0.0L && num1 != 0.0L && num2 != 0.0L) {
        switch (result.type) {
            case IEEE_Single: priority = 24; break;
            case IEEE_Double: priority = 53; break;
            case C99_LongDouble: priority = 65; break;
            default: priority = 0; break;
        }
        idx = addCancellation(inst, priority);
        cancels = instData[idx].total_cancels;
        if (!enable_sampling || cancels < 10 || 
                (cancels < 1000 && cancels%10==0) || 
                (cancels < 100000 && cancels%1000==0) || 
                (cancels%100000==0)) {
            sprintf(label_buffer, "%Lg %c %Lg = %Lg", num1, (opt == OP_ADD ? '+' : '-'), num2, numr);
            sprintf(details_buffer, "  %.25Lg\n%c %.25Lg\n= %.25Lg", num1, (opt == OP_ADD ? '+' : '-'), num2, numr);
            if (logFile->isStackWalkingEnabled()) {
                trace = logFile->getStackTrace();
            } else {
                trace = "";
            }
            logFile->addMessage(CANCELLATION, 999, string(label_buffer), string(details_buffer), trace, inst);
        }
        //printf("  catastrophic cancellation found!\n");
        return;
    }

    // check for "normal" cancellation (result has a smaller exponent than
    // one of the operands)
    expr = FPOperand::FPOpValExp(&result);
    priority = (exp1 > exp2 ? exp1 - expr : exp2 - expr);
    //printf("CHECKING:  [priority=%ld] [exp=%ld] %c [exp=%ld] = [exp=%ld]\n", 
            //priority, exp1, (opt == OP_ADD ? '+' : '-'), exp2, expr);
    if (priority > min_priority) {
        idx = addCancellation(inst, priority);
        cancels = instData[idx].total_cancels;
        if (!enable_sampling || cancels < 10 || 
                (cancels < 1000 && cancels%10==0) || 
                (cancels < 100000 && cancels%1000==0) || 
                (cancels%100000==0)) {
            sprintf(label_buffer, "%Lg %c %Lg = %Lg", num1, (opt == OP_ADD ? '+' : '-'), num2, numr);
            sprintf(details_buffer, "  %.25Lg [exp=%ld]\n%c %.25Lg [exp=%ld]\n= %.25Lg [exp=%ld]", 
                    num1, exp1, (opt == OP_ADD ? '+' : '-'), num2, exp2, numr, expr);
            if (logFile->isStackWalkingEnabled()) {
                trace = logFile->getStackTrace();
            } else {
                trace = "";
            }
            logFile->addMessage(CANCELLATION, priority, string(label_buffer), string(details_buffer), trace, inst);
        }
        //printf("  cancellation found!\n");
    }

    //printf("%s\n", FPOperation::FPOpType2Str(opt).c_str());
    //printf("  op1: %s\n", op1->toStringV().c_str());
    //printf("  op2: %s\n", op2->toStringV().c_str());
    //printf("  %Lg  [exp=%d]\n", num1, exp1);
    //printf("  %Lg  [exp=%d]\n", num2, exp2);
    //printf("  = %Lg  [exp=%d]\n", numr, expr);
}

void FPAnalysisDCancel::expandInstData(size_t newSize)
{
    FPAnalysisDCancelInstData *newInstData;
    size_t i = 0;
    newSize = newSize > instCount*2 ? newSize : instCount*2;
    //printf("expand_inst_data - old size: %d    new size: %d\n", instCount, newSize);
    newInstData = (FPAnalysisDCancelInstData*)malloc(newSize * sizeof(FPAnalysisDCancelInstData));
    if (!newInstData) {
        fprintf(stderr, "OUT OF MEMORY!\n");
        exit(-1);
    }
    if (instData != NULL) {
        for (; i < instCount; i++) {
            newInstData[i].inst = instData[i].inst;
            newInstData[i].count = instData[i].count;
            newInstData[i].total_cancels = instData[i].total_cancels;
            newInstData[i].total_digits = instData[i].total_digits;
        }
        free(instData);
        instData = NULL;
    }
    for (; i < newSize; i++) {
        newInstData[i].inst = NULL;
        newInstData[i].count = 0;
        newInstData[i].total_cancels = 0;
        newInstData[i].total_digits = 0;
    }
    instData = newInstData;
    instCount = newSize;
}

unsigned inline FPAnalysisDCancel::incrementCount(FPSemantics *inst)
{
    unsigned idx = inst->getIndex();
    if (idx >= instCount) {
        expandInstData(idx+1);
    }
    if (instData[idx].inst == NULL) {
        instData[idx].inst = inst;
    } else {
        assert(instData[idx].inst == inst);
    }
    instData[idx].count++;
    return idx;
}

unsigned inline FPAnalysisDCancel::addCancellation(FPSemantics *inst, long digits)
{
    unsigned idx = inst->getIndex();
    /*
     * now done in incrementCount()
     *
     *if (idx >= instCount) {
     *    expandInstData(idx+1);
     *}
     *if (instData[idx].inst == NULL) {
     *    instData[idx].inst = inst;
     *} else {
     *    assert(instData[idx].inst == inst);
     *}
     */
    instData[idx].total_cancels += 1;
    instData[idx].total_digits += (unsigned)digits;
    return idx;
}

void FPAnalysisDCancel::finalOutput()
{
    stringstream ss;
    size_t i;
    unsigned long cnt;
    for (i = 0; i < instCount; i++) {
        if (instData[i].inst && instData[i].total_cancels > 0) {
            ss.clear();
            ss.str("");
            ss << "CANCEL_DATA:" << endl;
            ss << "total_cancels=" << instData[i].total_cancels << endl;
            ss << "total_digits=" << instData[i].total_digits << endl;
            ss << "average_digits=" << (double)(instData[i].total_digits) / (double)(instData[i].total_cancels) << endl;
            logFile->addMessage(SUMMARY, 0, "CANCEL_DATA", ss.str(), 
                    "", instData[i].inst);

            ss.clear(); ss.str("");
            ss << instData[i].inst->getDisassembly();
            cnt = instData[i].count;
            logFile->addMessage(ICOUNT, (long)cnt, instData[i].inst->getDisassembly(),
                    ss.str(), "", instData[i].inst);

        }
    }
}

}

