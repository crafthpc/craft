#include "FPAnalysisRPrec.h"

namespace FPInst {

extern "C" {
    FPAnalysisRPrec *_INST_Main_RPrecAnalysis = NULL;
}

bool FPAnalysisRPrec::existsInstance()
{
    return (_INST_Main_RPrecAnalysis != NULL);
}

FPAnalysisRPrec* FPAnalysisRPrec::getInstance()
{
    if (!_INST_Main_RPrecAnalysis) {
        _INST_Main_RPrecAnalysis = new FPAnalysisRPrec();
    }
    return _INST_Main_RPrecAnalysis;
}

FPAnalysisRPrec::FPAnalysisRPrec()
    : FPAnalysis()
{
    useLockPrefix = false;
    instData = NULL;
    instCount = 0;
    expandInstData(4096);
    insnsInstrumented = 0;
}

string FPAnalysisRPrec::getTag()
{
    return "rprec";
}

string FPAnalysisRPrec::getDescription()
{
    return "Reduced Precision Analysis";
}

void FPAnalysisRPrec::configure(FPConfig *config, FPDecoder *decoder,
        FPLog *log, FPContext *context)
{
    FPAnalysis::configure(config, decoder, log, context);

    if (config->getValue("use_lock_prefix") == "yes") {
        enableLockPrefix();
    }
    if (config->hasValue("r_prec_default_precision")) {
        const char *prec = config->getValueC("r_prec_default_precision");
        defaultPrecision = strtoul(prec, NULL, 10);
    }
}

bool FPAnalysisRPrec::isSupportedOp(FPOperation *op)
{
    return op->type == OP_ADD ||
           op->type == OP_SUB ||
           op->type == OP_MUL ||
           op->type == OP_DIV ||
           op->type == OP_CVT ||
           op->type == OP_SQRT;
           //op->type == OP_OR ||       // TODO: should we handle these?
           //op->type == OP_AND ||
           //op->type == OP_OR ||
           //op->type == OP_XOR ||
           //op->type == OP_MIN ||
           //op->type == OP_MAX ||
           //op->type == OP_NEG ||
           //op->type == OP_MOV;
}

bool FPAnalysisRPrec::shouldPreInstrument(FPSemantics * /*inst*/)
{
    return false;
}

bool FPAnalysisRPrec::shouldPostInstrument(FPSemantics * /*inst*/)
{
    return false;
}

bool FPAnalysisRPrec::shouldReplace(FPSemantics *inst)
{
    bool instrument = false;

    FPOperation *op;
    FPOperand *output;
    size_t i, j, k;

    // see if there's anything besides move, zero, stack, or noop operations
    for (unsigned k=0; k<inst->size(); k++) {
        op = (*inst)[k];
        if (isSupportedOp(op)) {
            instrument = true;
        }
    }

    // all output operands must be double-precision
    // (currently no support for truncating anything else)
    for (i=0; i<inst->numOps; i++) {
        op = (*inst)[i];
        for (j=0; j<op->numOpSets; j++) {
            for (k=0; k<op->opSets[j].nOut; k++) {
                output = op->opSets[j].out[k];
                if (output->getType() != IEEE_Double &&
                    output->getType() != IEEE_Single) {
                    instrument = false;
                }
            }
        }
    }

    return instrument;
}

Snippet::Ptr FPAnalysisRPrec::buildPreInstrumentation(FPSemantics * /*inst*/,
        BPatch_addressSpace * /*app*/, bool & /*needsRegisters*/)
{
    return Snippet::Ptr();
}

Snippet::Ptr FPAnalysisRPrec::buildPostInstrumentation(FPSemantics * /*inst*/,
        BPatch_addressSpace * /*app*/, bool & /*needsRegisters*/)
{
    return Snippet::Ptr();
}

Snippet::Ptr FPAnalysisRPrec::buildReplacementCode(FPSemantics *inst,
        BPatch_addressSpace *app, bool & /*needsRegisters*/)
{
    stringstream ss;
    string key, value;
    size_t idx = inst->getIndex();
    if (idx >= instCount) {
        expandInstData(idx+1);
    }

    instData[idx].precision = defaultPrecision;
    ss.clear(); ss.str(""); ss << "INSN_" << dec << idx << "_precision";
    key = ss.str();
    //cout << "key=" << key << " ";
    if (configuration->hasValue(key)) {
        ss.clear(); ss.str(configuration->getValue(key));
        ss >> instData[idx].precision;
        //cout << "  value=" << configuration->getValue(key) << " ";
    }
    //cout << endl;

    instData[idx].count_addr = app->malloc(sizeof(unsigned long))->getBaseAddr();

    ss.clear(); ss.str(""); ss << "INSN_" << dec << idx << "_count_addr";
    key = ss.str(); ss.str("");
    ss << hex << instData[idx].count_addr;
    value = ss.str(); ss.str("");
    configuration->setValue(key, value);

    insnsInstrumented++;

    FPBinaryBlobRPrec *blob = new FPBinaryBlobRPrec(inst, instData[idx]);
    if (!shouldReplace(inst)) {
        logFile->addMessage(WARNING, 0,
               "Truncation disabled", "Should not replace; no truncation code inserted",
               "", inst);
        blob->disableTruncation();
    }
    if (useLockPrefix) {
        blob->enableLockPrefix();
    }
    return Snippet::Ptr(blob);
}

FPBinaryBlobRPrec::FPBinaryBlobRPrec(FPSemantics *inst,
        FPAnalysisRPrecInstData instData)
    : FPBinaryBlob(inst)
{
    this->instData = instData;
    this->doTruncation = true;
    this->useLockPrefix = false;

    // set up bit masks for truncations
    //
    rprecConst64[0] = (uint64_t)0xFFF0000000000000;
    //printf("%lu = %lx\n", 0UL, (unsigned long)rprecConst64[0]);
    for (uint64_t i=1; i<=52; i++) {
        rprecConst64[i] = rprecConst64[i-1] | (1UL << (52-i));
        //printf("%lu = %lx\n", i, (unsigned long)rprecConst64[i]);
    }
    rprecConst32[0] = (uint32_t)0xFF800000;
    //printf("%u = %x\n", 0UL, (unsigned int)rprecConst32[0]);
    for (uint32_t i=1; i<=23; i++) {
        rprecConst32[i] = rprecConst32[i-1] | (1UL << (23-i));
        //printf("%u = %x\n", i, (unsigned int)rprecConst32[i]);
    }
}

void FPBinaryBlobRPrec::enableLockPrefix()
{
    useLockPrefix = true;
}

void FPBinaryBlobRPrec::disableLockPrefix()
{
    useLockPrefix = false;
}

void FPAnalysisRPrec::enableLockPrefix()
{
    useLockPrefix = true;
}

void FPAnalysisRPrec::disableLockPrefix()
{
    useLockPrefix = false;
}

void FPBinaryBlobRPrec::disableTruncation()
{
    doTruncation = false;
}

void FPBinaryBlobRPrec::enableTruncation()
{
    doTruncation = true;
}

bool FPBinaryBlobRPrec::generate(Point * /*pt*/, Buffer &buf)
{
    size_t origNumBytes = inst->getNumBytes();
    unsigned char *orig_code, *pos, *opos;
    FPOperation *op;
    FPOperand *input, *output, *eip_operand = NULL;
    FPRegister temp_gpr1, temp_xmm1;
    bool packed = false;
    size_t i;

    unsigned long precision = instData.precision;

    // should only be one operation
    // may need to adjust this for fused multiply-add instructions at some point
    assert(inst->numOps == 1);
    op = (*inst)[0];

    initialize();

    // allocate space for blob code
    orig_code = (unsigned char*)malloc(origNumBytes);
    inst->getBytes(orig_code);

    setBlobAddress((void*)buf.curAddr());
    pos = getBlobCode();

    // set up some class-wide variables
    temp_gpr1 = getUnusedGPR();
    temp_xmm1 = getUnusedSSE();

    /*
     *printf("building binary blob at 0%p [id=%lu]: %s\n%s\n",
     *       inst->getAddress(), inst->getIndex(), inst->getDisassembly().c_str(),
     *       inst->toString().c_str());
     *printf("  precision = %lu\n", instData.precision);
     */

    // check for an ip-relative operand
    for (i=0; i<op->opSets[0].nIn; i++) {
        input = op->opSets[0].in[i];
        if (input->getBase() == REG_EIP) {
            eip_operand = input;
        }
    }

    // emit original instruction
    opos = orig_code;
    for (i=0; i < origNumBytes; i++) {
        *pos++ = *opos++;
    }
    if (eip_operand) {
        adjustDisplacement(eip_operand->getDisp(), pos);
    }

    // binary blob header and state saving
    pos += buildHeader(pos);
    if (temp_gpr1 != REG_EAX) {
        pos += buildFakeStackPushGPR64(pos, temp_gpr1);
    }
    pos += buildFakeStackPushXMM(pos, temp_xmm1);

    if (doTruncation) {

        // is this a packed SSE instruction?
        packed = (op->numOpSets > 1);

        // grab the output operand
        output = op->opSets[0].out[0];

        assert(output->isRegisterSSE());
            //fprintf(stderr, "Cannot reduce precision--output register is not SSE: %s\n",
                    //inst->getDisassembly().c_str());

        // load temporary XMM register with truncating constants
        //
        if (output->getType() == IEEE_Double) {
            if (precision > 52) {
                precision = 52;
            }
            pos += mainGen->buildMovImm64ToGPR64(pos, rprecConst64[precision], temp_gpr1);
            pos += mainGen->buildInsertGPR64IntoXMM(pos, temp_gpr1, temp_xmm1, 0);
            if (packed) {
                pos += mainGen->buildInsertGPR64IntoXMM(pos, temp_gpr1, temp_xmm1, 2);
            } else {
                pos += mainGen->buildMovImm64ToGPR64(pos, rprecConst64[52], temp_gpr1);
                pos += mainGen->buildInsertGPR64IntoXMM(pos, temp_gpr1, temp_xmm1, 2);
            }
        } else if (output->getType() == IEEE_Single) {
            if (precision > 23) {
                precision = 23;
            }
            pos += mainGen->buildMovImm32ToGPR32(pos, rprecConst32[precision], temp_gpr1);
            pos += mainGen->buildInsertGPR32IntoXMM(pos, temp_gpr1, temp_xmm1, 0);
            if (packed) {
                pos += mainGen->buildInsertGPR32IntoXMM(pos, temp_gpr1, temp_xmm1, 1);
                pos += mainGen->buildInsertGPR32IntoXMM(pos, temp_gpr1, temp_xmm1, 2);
                pos += mainGen->buildInsertGPR32IntoXMM(pos, temp_gpr1, temp_xmm1, 3);
            } else {
                pos += mainGen->buildMovImm32ToGPR32(pos, rprecConst32[23], temp_gpr1);
                pos += mainGen->buildInsertGPR32IntoXMM(pos, temp_gpr1, temp_xmm1, 1);
                pos += mainGen->buildInsertGPR32IntoXMM(pos, temp_gpr1, temp_xmm1, 2);
                pos += mainGen->buildInsertGPR32IntoXMM(pos, temp_gpr1, temp_xmm1, 3);
            }
        }

        // perform truncation
        pos += mainGen->buildAndXMMWithXMM(pos, output->getRegister(), temp_xmm1);
    }
    
    // increment instruction count
    pos += mainGen->buildIncMem64(pos,
            (int32_t)(unsigned long)instData.count_addr, useLockPrefix);

    // binary blob state restore and footer
    pos += buildFakeStackPopXMM(pos, temp_xmm1);
    if (temp_gpr1 != REG_EAX) {
        pos += buildFakeStackPopGPR64(pos, temp_gpr1);
    }
    pos += buildFooter(pos);
    finalize();

    unsigned char *b = (unsigned char*)getBlobCode();
    for (b = (unsigned char*)getBlobCode(); b < pos; b++) {
        //printf("%02x ", *b);
        buf.push_back(*b);
    }
    //printf("buffer size=%u %s\n", buf.size(), (buf.empty() ? "empty" : "non-empty"));
    /*
     *printf("built binary blob at %p [size=%ld] for %u-byte instruction at %p: %-40s [%u]\n", 
     *       getBlobAddress(), (b-getBlobCode()), 
     *       (unsigned)inst->getNumBytes(), inst->getAddress(),
     *       inst->getDisassembly().c_str(), (unsigned)inst->getIndex());
     *printf("  precision = %lu\n", instData.precision);
     */
    return true;
}

string FPAnalysisRPrec::finalInstReport()
{
    stringstream ss;
    ss << "RPrec: " << insnsInstrumented << " instrumented";
    return ss.str();
}

void FPAnalysisRPrec::registerInstruction(FPSemantics *inst)
{
    size_t idx = inst->getIndex();
    stringstream ss("");
    string key;
    if (inst->getIndex() >= instCount) {
        expandInstData(inst->getIndex());
    }
    instData[idx].inst = inst;
    instData[idx].precision = defaultPrecision;
    instData[idx].count = 0;

    ss.clear(); ss.str(""); ss << "INSN_" << dec << idx << "_precision";
    key = ss.str();
    //cout << "key=" << key << " ";
    if (configuration->hasValue(key)) {
        ss.clear(); ss.str(configuration->getValue(key));
        ss >> instData[idx].precision;
    }

    ss.clear(); ss.str(""); ss << "INSN_" << dec << idx << "_count_addr";
    key = ss.str();
    ss.clear(); ss.str(configuration->getValue(key));
    ss >> instData[idx].count_addr;
    //cout << "key=" << key << " ";
    if (instData[idx].count_addr) {
        *(unsigned long*)(instData[idx].count_addr) = 0;
    }
}

void FPAnalysisRPrec::handlePreInstruction(FPSemantics * /*inst*/)
{ }

void FPAnalysisRPrec::handlePostInstruction(FPSemantics * /*inst*/)
{ }

void FPAnalysisRPrec::handleReplacement(FPSemantics * /*inst*/)
{ }

void FPAnalysisRPrec::reducePrecision(FPSemantics * /*inst*/, FPOperand * /*op*/)
{
    // TODO: non-binary-blob handling
}

void FPAnalysisRPrec::expandInstData(size_t newSize)
{
    FPAnalysisRPrecInstData *newInstData;
    size_t i = 0;
    newSize = newSize > instCount*2 ? newSize : instCount*2;
    //printf("expand_inst_data - old size: %u    new size: %u\n", (unsigned)instCount, (unsigned)newSize);
    newInstData = (FPAnalysisRPrecInstData*)malloc(newSize * sizeof(FPAnalysisRPrecInstData));
    if (!newInstData) {
        fprintf(stderr, "OUT OF MEMORY!\n");
        exit(-1);
    }
    if (instData != NULL) {
        for (; i < instCount; i++) {
            newInstData[i].inst = instData[i].inst;
            newInstData[i].precision = instData[i].precision;
            newInstData[i].count = instData[i].count;
            newInstData[i].count_addr = instData[i].count_addr;
        }
        free(instData);
        instData = NULL;
    }
    for (; i < newSize; i++) {
        newInstData[i].inst = NULL;
        newInstData[i].precision = 0;
        newInstData[i].count = 0;
        newInstData[i].count_addr = NULL;
    }
    instData = newInstData;
    instCount = newSize;
}

void FPAnalysisRPrec::finalOutput()
{
    size_t icount, exec_total = 0;
    size_t i;

    // instruction counts
    stringstream ss2;
    for (i=0; i<instCount; i++) {
        if (instData[i].inst) {

            // overall count
            if (instData[i].count_addr) {
                icount = *(unsigned long*)(instData[i].count_addr);
            } else {
                icount = instData[i].count;
            }

            // output individual count
            ss2.clear();
            ss2.str("");
            ss2 << "instruction #" << i << ": count=" << icount;
            ss2 << " [prec=" << instData[i].precision << "]";
            logFile->addMessage(ICOUNT, icount, instData[i].inst->getDisassembly(), ss2.str(),
                    "", instData[i].inst);

            // add to aggregate counts
            exec_total += icount;
        }
    }
    stringstream ss;
    ss << "Finished execution:" << endl;
    ss << "  Rprec: " << logFile->formatLargeCount(exec_total) << " executed";
    logFile->addMessage(SUMMARY, 0, getDescription(), ss.str(), "");
    cerr << ss.str() << endl;
}

}

