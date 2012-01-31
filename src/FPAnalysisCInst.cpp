#include "FPAnalysisCInst.h"

namespace FPInst {

extern "C" {
    FPAnalysisCInst *_INST_Main_CInstAnalysis = NULL;
    unsigned long *_INST_main_inst_count = NULL;
}

bool FPAnalysisCInst::existsInstance()
{
    return (_INST_Main_CInstAnalysis != NULL);
}

FPAnalysisCInst* FPAnalysisCInst::getInstance()
{
    if (!_INST_Main_CInstAnalysis) {
        _INST_Main_CInstAnalysis = new FPAnalysisCInst();
    }
    return _INST_Main_CInstAnalysis;
}

FPAnalysisCInst::FPAnalysisCInst()
    : FPAnalysis()
{
    instCountSize = 0;
    instCount = NULL;
    expandInstCount(2000);
}

string FPAnalysisCInst::getTag()
{
    return "cinst";
}

string FPAnalysisCInst::getDescription()
{
    return "Instruction Count Analysis";
}

void FPAnalysisCInst::configure(FPConfig *config, FPDecoder *decoder,
        FPLog *log, FPContext *context)
{
    FPAnalysis::configure(config, decoder, log, context);
    //status << "c_inst: initialized" << endl;
}

bool FPAnalysisCInst::shouldPreInstrument(FPSemantics * /*inst*/)
{
    return true;
}

bool FPAnalysisCInst::shouldPostInstrument(FPSemantics * /*inst*/)
{
    return false;
}

bool FPAnalysisCInst::shouldReplace(FPSemantics * /*inst*/)
{
    return false;
}

Snippet::Ptr FPAnalysisCInst::buildPreInstrumentation(FPSemantics *inst,
        BPatch_addressSpace *app, bool & /*needsRegisters*/)
{
    unsigned long iidx = inst->getIndex();
    BPatch_image *img = app->getImage();
    BPatch_variableExpr *instVar = img->findVariable("_INST_main_inst_count");
    BPatch_snippet *idxExpr = new BPatch_arithExpr(BPatch_plus,
            *instVar, BPatch_constExpr(iidx*sizeof(unsigned long)));
    BPatch_snippet *countExpr = new BPatch_arithExpr(BPatch_deref,
            *idxExpr);
    BPatch_snippet *valExpr = new BPatch_arithExpr(BPatch_plus,
            *countExpr, BPatch_constExpr(1));
    BPatch_snippet *incExpr = new BPatch_arithExpr(BPatch_assign,
            *idxExpr, *valExpr);
    return PatchAPI::convert(incExpr);
}

Snippet::Ptr FPAnalysisCInst::buildPostInstrumentation(FPSemantics * /*inst*/,
        BPatch_addressSpace * /*app*/, bool & /*needsRegisters*/)
{
    return Snippet::Ptr();
}

Snippet::Ptr FPAnalysisCInst::buildReplacementCode(FPSemantics * /*inst*/,
        BPatch_addressSpace * /*app*/, bool & /*needsRegisters*/)
{
    return Snippet::Ptr();
}

void FPAnalysisCInst::expandInstCount(size_t newSize)
{
    size_t *newInstCount;
    size_t i = 0;
    newSize = (newSize > instCountSize*2) ? (newSize + 10) : (instCountSize*2 + 10);
    //printf("expand_inst_count - old size: %d    new size: %d\n", instCountSize, newSize);
    newInstCount = (size_t*)malloc(newSize * sizeof(size_t));
    if (!newInstCount) {
        fprintf(stderr, "OUT OF MEMORY!\n");
        exit(-1);
    }
    if (instCount != NULL) {
        for (; i < instCountSize; i++) {
            newInstCount[i] = instCount[i];
        }
        free(instCount);
        instCount = NULL;
    }
    for (; i < newSize; i++) {
        newInstCount[i] = 0;
    }
    instCount = newInstCount;
    instCountSize = newSize;
    _INST_main_inst_count = instCount;
}

size_t * FPAnalysisCInst::getCountArrayPtr()
{
    return instCount;
}

void FPAnalysisCInst::registerInstruction(FPSemantics *inst)
{
    long idx = inst->getIndex();
    if ((size_t)idx >= instCountSize)
        expandInstCount(idx+1);
}

void FPAnalysisCInst::handlePreInstruction(FPSemantics *inst)
{
    long idx = inst->getIndex();
    if ((size_t)idx >= instCountSize)
        expandInstCount(idx+1);
    instCount[idx]++;
    
    //printf("== %s\n", inst->getDisassembly().c_str());
    //printf("%s\n\n", inst->toString().c_str());
}

void FPAnalysisCInst::handlePostInstruction(FPSemantics * /*inst*/)
{ }

void FPAnalysisCInst::handleReplacement(FPSemantics * /*inst*/)
{ }

void FPAnalysisCInst::finalOutput()
{
    FPSemantics *inst;
    size_t i;
    stringstream ss;
    stringstream ss2;
    for (i=0; i<instCountSize; i++) {
        inst = decoder->lookup(i);
        if (inst != NULL) {
            ss.clear();
            ss.str("");
            ss2.clear();
            ss2.str("");
            //ss << hex << inst->getAddress() << dec << "  ";
            ss << inst->getDisassembly();
            ss2 << "instruction #" << i << ": count=" << instCount[i];
            logFile->addMessage(ICOUNT, instCount[i], ss.str(), ss2.str(),
                    "", inst);
        }
    }
}

}

