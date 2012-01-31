#include "FPAnalysis.h"

namespace FPInst {

FPAnalysis::FPAnalysis()
{ }

string FPAnalysis::getTag()
{
    return "null";
}

string FPAnalysis::getDescription()
{
    return "Null Analysis";
}

void FPAnalysis::configure(FPConfig *config, FPDecoder *decoder,
        FPLog *log, FPContext *context)
{
    this->configuration = config;
    this->decoder = decoder;
    this->logFile = log;
    this->context = context;
}

bool FPAnalysis::shouldPreInstrument(FPSemantics * /*inst*/)
{
    return true;
}

bool FPAnalysis::shouldPostInstrument(FPSemantics * /*inst*/)
{
    return false;
}

bool FPAnalysis::shouldReplace(FPSemantics * /*inst*/)
{
    return false;
}

Snippet::Ptr FPAnalysis::buildPreInstrumentation(FPSemantics * /*inst*/,
        BPatch_addressSpace * /*app*/, bool & /*needsRegisters*/)
{
    return PatchAPI::convert(new BPatch_nullExpr());
}

Snippet::Ptr FPAnalysis::buildPostInstrumentation(FPSemantics * /*inst*/,
        BPatch_addressSpace * /*app*/, bool & /*needsRegisters*/)
{
    return Snippet::Ptr();
}

Snippet::Ptr FPAnalysis::buildReplacementCode(FPSemantics * /*inst*/,
        BPatch_addressSpace * /*app*/, bool & /*needsRegisters*/)
{
    return Snippet::Ptr();
}

void FPAnalysis::registerInstruction(FPSemantics *)
{ }

void FPAnalysis::handlePreInstruction(FPSemantics *)
{ }

void FPAnalysis::handlePostInstruction(FPSemantics *)
{ }

void FPAnalysis::handleReplacement(FPSemantics *)
{ }

void FPAnalysis::finalOutput()
{ }

FPAnalysis::~FPAnalysis()
{ }

}

