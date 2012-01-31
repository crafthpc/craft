#include "FPAnalysisDNan.h"

namespace FPInst {

extern "C" {
    FPAnalysisDNan *_INST_Main_DNanAnalysis = NULL;
}

bool FPAnalysisDNan::existsInstance()
{
    return (_INST_Main_DNanAnalysis != NULL);
}

FPAnalysisDNan* FPAnalysisDNan::getInstance()
{
    if (!_INST_Main_DNanAnalysis) {
        _INST_Main_DNanAnalysis = new FPAnalysisDNan();
    }
    return _INST_Main_DNanAnalysis;
}

FPAnalysisDNan::FPAnalysisDNan()
    : FPAnalysis()
{
    numRangeAddresses = 0;
}

string FPAnalysisDNan::getTag()
{
    return "dnan";
}

string FPAnalysisDNan::getDescription()
{
    return "NaN Detection Analysis";
}

void FPAnalysisDNan::configure(FPConfig *config, FPDecoder *decoder,
        FPLog *log, FPContext *context)
{
    FPAnalysis::configure(config, decoder, log, context);
    configAddresses(config);
    if (isRestrictedByAddress()) {
        //status << "d_nan: addresses=" << listAddresses();
    }
}

void FPAnalysisDNan::configAddresses(FPConfig *config)
{
    if (config->hasValue("dnan_addresses")) {
        numRangeAddresses = config->getAddressList(rangeAddresses, "dnan_addresses");
        //printf("filtering by %d addresses\n", numRangeAddresses);
    } /*else {
        printf("no filtering\n");
    }*/
}

bool FPAnalysisDNan::isRestrictedByAddress()
{
    return numRangeAddresses > 0;
}

string FPAnalysisDNan::listAddresses() {
    stringstream ss;
    ss.clear();
    ss.str("");
    size_t i;
    ss << hex;
    for (i=0; i<numRangeAddresses; i++) {
        ss << rangeAddresses[i] << " ";
    }
    return ss.str();
}

bool FPAnalysisDNan::shouldPreInstrument(FPSemantics *inst)
{
    bool isOperation = false, handle = false;
    FPOperation *op;
    FPOperationType opt;
    size_t i;

    // is this an operation?
    for (i=0; i<inst->numOps; i++) {
        op = (*inst)[i];
        opt = op->type;
        if ((opt != OP_INVALID && opt != OP_NONE && 
             opt != OP_MOV && opt != OP_CVT &&
             opt != OP_COM && opt != OP_COMI &&
             opt != OP_UCOM && opt != OP_UCOMI) &&
            (op->hasOperandOfType(IEEE_Single) ||
             op->hasOperandOfType(IEEE_Double) ||
             op->hasOperandOfType(C99_LongDouble))) {
            isOperation = true;
            break;
        }
    }

    // does it involve any floating-point numbers?

    // if so, is this on the list of addresses to handle?
    if (isOperation && numRangeAddresses > 0) {
        void* addr = inst->getAddress();
        size_t i;
        bool found = false;
        for (i=0; i<numRangeAddresses; i++) {
            if (rangeAddresses[i] == addr) {
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
        handle = isOperation;
    }

    return handle;
}

bool FPAnalysisDNan::shouldPostInstrument(FPSemantics * /*inst*/)
{
    return false;
}

bool FPAnalysisDNan::shouldReplace(FPSemantics * /*inst*/)
{
    return false;
}

Snippet::Ptr FPAnalysisDNan::buildPreInstrumentation(FPSemantics * /*inst*/,
        BPatch_addressSpace * /*app*/, bool &needsRegisters)
{
    needsRegisters = true;
    return Snippet::Ptr();
}

Snippet::Ptr FPAnalysisDNan::buildPostInstrumentation(FPSemantics * /*inst*/,
        BPatch_addressSpace * /*app*/, bool & /*needsRegisters*/)
{
    return Snippet::Ptr();
}

Snippet::Ptr FPAnalysisDNan::buildReplacementCode(FPSemantics * /*inst*/,
        BPatch_addressSpace * /*app*/, bool & /*needsRegisters*/)
{
    return Snippet::Ptr();
}

void FPAnalysisDNan::registerInstruction(FPSemantics * /*inst*/)
{ }

void FPAnalysisDNan::handlePreInstruction(FPSemantics *inst)
{
    FPOperation *op;
    FPOperandSet *ops;
    FPOperand *input;
    size_t i, j, k;
    double val;

    // for each operation ...
    for (i=0; i<inst->numOps; i++) {
        op = (*inst)[i];
        ops = op->opSets;

        // for each input ...
        for (j=0; j<op->numOpSets; j++) {
            for (k=0; k<ops[j].nIn; k++) {
                input = ops[j].in[k];
                input->refresh(context);
                val = input->getCurrentValueD();
                if (isnan(val)) {
                    string lbl = "NaN detected: " + input->toStringV();
                    logFile->addMessage(WARNING, 999, lbl, lbl, "", inst);
                }
            }
        }
    }
}

void FPAnalysisDNan::handlePostInstruction(FPSemantics * /*inst*/)
{ }

void FPAnalysisDNan::handleReplacement(FPSemantics * /*inst*/)
{ }

void FPAnalysisDNan::finalOutput()
{ }

}

