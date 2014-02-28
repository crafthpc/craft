
// NEWMODE: update all file and class references
//          (basically search/replace "FPAnalysisExample")

#include "FPAnalysisExample.h"

namespace FPInst {

// NEWMODE: update this boilerplate code with the new name
//          (basically search/replace "Example")
//
extern "C" {
    FPAnalysisExample *_INST_Main_ExampleAnalysis = NULL;
}
bool FPAnalysisExample::existsInstance()
{
    return (_INST_Main_ExampleAnalysis != NULL);
}
FPAnalysisExample* FPAnalysisExample::getInstance()
{
    if (!_INST_Main_ExampleAnalysis) {
        _INST_Main_ExampleAnalysis = new FPAnalysisExample();
    }
    return _INST_Main_ExampleAnalysis;
}

FPAnalysisExample::FPAnalysisExample()
    : FPAnalysis()
{
    msgTag = "";
    insnsInstrumented = 0;
    insnsExecuted = 0;
}

string FPAnalysisExample::getTag()
{
    // NEWMODE: this must be unique among all FPAnalysis subclasses
    //
    return "d_print";
}

string FPAnalysisExample::getDescription()
{
    return "Example Analysis";
}

void FPAnalysisExample::configure(FPConfig *config, FPDecoder *decoder,
        FPLog *log, FPContext *context)
{
    FPAnalysis::configure(config, decoder, log, context);
    if (config->hasValue("example_tag")) {
        msgTag = config->getValue("example_tag");
    }
}

bool FPAnalysisExample::shouldPreInstrument(FPSemantics * /*inst*/)
{
    return true;
}

bool FPAnalysisExample::shouldPostInstrument(FPSemantics * /*inst*/)
{
    return false;
}

bool FPAnalysisExample::shouldReplace(FPSemantics * /*inst*/)
{
    return false;
}

Snippet::Ptr FPAnalysisExample::buildPreInstrumentation(FPSemantics * /*inst*/,
        BPatch_addressSpace * /*app*/, bool & /*needsRegisters*/)
{
    insnsInstrumented++;
    return Snippet::Ptr();
}

Snippet::Ptr FPAnalysisExample::buildPostInstrumentation(FPSemantics * /*inst*/,
        BPatch_addressSpace * /*app*/, bool & /*needsRegisters*/)
{
    return Snippet::Ptr();
}

Snippet::Ptr FPAnalysisExample::buildReplacementCode(FPSemantics * /*inst*/,
        BPatch_addressSpace * /*app*/, bool & /*needsRegisters*/)
{
    return Snippet::Ptr();
}

string FPAnalysisExample::finalInstReport()
{
    stringstream ss;
    ss << "Example: " << insnsInstrumented << " instrumented";
    return ss.str();
}

void FPAnalysisExample::registerInstruction(FPSemantics * /*inst*/)
{ }

void FPAnalysisExample::handlePreInstruction(FPSemantics *inst)
{
    insnsExecuted++;
    printf("%s %s\n", msgTag.c_str(), inst->getDisassembly().c_str());
}

void FPAnalysisExample::handlePostInstruction(FPSemantics * /*inst*/)
{ }

void FPAnalysisExample::handleReplacement(FPSemantics * /*inst*/)
{ }

void FPAnalysisExample::finalOutput()
{
    stringstream ss;
    ss.clear();
    ss.str("");
    ss << "EXAMPLE_DATA:" << endl;
    ss << "total_digits=" << insnsInstrumented << endl;
    logFile->addMessage(SUMMARY, 0, "EXAMPLE_DATA", ss.str(), 
            "", NULL);
}

}

