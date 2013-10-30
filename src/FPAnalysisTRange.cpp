#include "FPAnalysisTRange.h"

namespace FPInst {

extern "C" {
    FPAnalysisTRange *_INST_Main_TRangeAnalysis = NULL;
}

bool FPAnalysisTRange::existsInstance()
{
    return (_INST_Main_TRangeAnalysis != NULL);
}

FPAnalysisTRange* FPAnalysisTRange::getInstance()
{
    if (!_INST_Main_TRangeAnalysis) {
        _INST_Main_TRangeAnalysis = new FPAnalysisTRange();
    }
    return _INST_Main_TRangeAnalysis;
}

FPAnalysisTRange::FPAnalysisTRange()
    : FPAnalysis()
{
    numRangeAddresses = 0;
    instCount = 0;
    instData = NULL;
    expandInstData(4096);
    insnsInstrumented = 0;
}

string FPAnalysisTRange::getTag()
{
    return "trange";
}

string FPAnalysisTRange::getDescription()
{
    return "Range Tracking Analysis";
}

void FPAnalysisTRange::configure(FPConfig *config, FPDecoder *decoder,
        FPLog *log, FPContext *context)
{
    FPAnalysis::configure(config, decoder, log, context);
    configAddresses(config);
    if (isRestrictedByAddress()) {
        //status << "t_range: addresses=" << listAddresses();
    }
}

void FPAnalysisTRange::configAddresses(FPConfig *config)
{
    if (config->hasValue("trange_addresses")) {
        numRangeAddresses = config->getAddressList(rangeAddresses, "trange_addresses");
        //printf("filtering by %d addresses\n", numRangeAddresses);
    } /*else {
        printf("no filtering\n");
    }*/
}

bool FPAnalysisTRange::isRestrictedByAddress()
{
    return numRangeAddresses > 0;
}

string FPAnalysisTRange::listAddresses() {
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

bool FPAnalysisTRange::shouldPreInstrument(FPSemantics * /*inst*/)
{
    return false;
}

bool FPAnalysisTRange::shouldPostInstrument(FPSemantics * /*inst*/)
{
    return false;
}

bool FPAnalysisTRange::shouldReplace(FPSemantics *inst)
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
             opt != OP_MOV && opt != OP_CVT) &&
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

Snippet::Ptr FPAnalysisTRange::buildPreInstrumentation(FPSemantics * /*inst*/,
        BPatch_addressSpace * /*app*/, bool & /*needsRegisters*/)
{
    return Snippet::Ptr();
}

Snippet::Ptr FPAnalysisTRange::buildPostInstrumentation(FPSemantics * /*inst*/,
        BPatch_addressSpace * /*app*/, bool & /*needsRegisters*/)
{
    return Snippet::Ptr();
}

Snippet::Ptr FPAnalysisTRange::buildReplacementCode(FPSemantics *inst,
        BPatch_addressSpace *app, bool & /*needsRegisters*/)
{
    size_t idx = inst->getIndex();
    if (idx >= instCount) {
        expandInstData(idx+1);
    }

    instData[idx].min_addr   = app->malloc(sizeof(double))->getBaseAddr();
    instData[idx].max_addr   = app->malloc(sizeof(double))->getBaseAddr();
    instData[idx].count_addr = app->malloc(sizeof(unsigned long))->getBaseAddr();
    //printf("  min_addr=%p  max_addr=%p\n",
            //instData[idx].min_addr, instData[idx].max_addr);

    stringstream ss("");
    string key, value;

    ss << "inst" << dec << idx << "_min_addr";
    key = ss.str(); ss.str("");
    ss << hex << instData[idx].min_addr;
    value = ss.str(); ss.str("");
    configuration->setValue(key, value);

    ss << "inst" << dec << idx << "_max_addr";
    key = ss.str(); ss.str("");
    ss << hex << instData[idx].max_addr;
    value = ss.str(); ss.str("");
    configuration->setValue(key, value);

    ss << "inst" << dec << idx << "_count_addr";
    key = ss.str(); ss.str("");
    ss << hex << instData[idx].count_addr;
    value = ss.str(); ss.str("");
    configuration->setValue(key, value);

    insnsInstrumented++;

    return Snippet::Ptr(new FPBinaryBlobTRange(inst, instData[idx]));
}

FPBinaryBlobTRange::FPBinaryBlobTRange(FPSemantics *inst, 
        FPAnalysisTRangeInstData instData)
    : FPBinaryBlob(inst)
{
    this->instData = instData;
}

bool FPBinaryBlobTRange::generate(Point * /*pt*/, Buffer &buf)
{
    size_t origNumBytes = inst->getNumBytes();
    unsigned char *orig_code, *pos, *opos;
    FPRegister temp_gpr1, temp_xmm1, temp_xmm2;

    initialize();

    // allocate space for blob code
    orig_code = (unsigned char*)malloc(origNumBytes);
    inst->getBytes(orig_code);

    setBlobAddress((void*)buf.curAddr());
    pos = getBlobCode();

    // set up some class-wide variables
    temp_gpr1 = getUnusedGPR();
    temp_xmm1 = getUnusedSSE();
    temp_xmm2 = getUnusedSSE();

    /*
     *printf("building binary blob at 0%p: %s\n%s\n",
     *        inst->getAddress(), inst->getDisassembly().c_str(),
     *        inst->toString().c_str());
     *printf("  comparing against min [%p] and max [%p]\n",
     *        instData.min_addr,
     *        instData.max_addr);
     */
    
    pos += buildHeader(pos);
    if (temp_gpr1 != REG_EAX) {
        pos += buildFakeStackPushGPR64(pos, temp_gpr1);
    }
    pos += buildFakeStackPushXMM(pos, temp_xmm1);
    pos += buildFakeStackPushXMM(pos, temp_xmm2);

    FPOperation *op;
    FPOperand *input, *output;
    size_t i, j, k;
    unsigned char *skip_jmp_pos = 0;
    int32_t *skip_offset_pos = 0;
    FPOperand *eip_operand = NULL;

    // for each operation
    for (i=0; i<inst->numOps; i++) {
        op = (*inst)[i];

        if (op->numOpSets==0)
            continue;
        
        // for each operand set
        for (j=0; j<1; j++) {

            for (k=0; k<op->opSets[j].nIn; k++) {
                input = op->opSets[j].in[k];

                if (input->getBase() == REG_EIP) {
                    eip_operand = input;
                }
                
                if (input->getType() != IEEE_Single &&
                    input->getType() != IEEE_Double) {
                    continue;
                }

                // load operand value
                pos += buildOperandLoadXMM(pos, input, temp_xmm1, false);
                if (input->getType() == IEEE_Single) {
                    pos += mainGen->buildCvtss2sd(pos, temp_xmm1, temp_xmm1);
                }

                // load reference minimum value into temp_xmm2
                pos += mainGen->buildMovImm64ToGPR64(pos,
                        (uint64_t)instData.min_addr, temp_gpr1);
                pos += mainGen->buildInstruction(pos, 0xf2, false, true,
                        0x10, temp_xmm2, temp_gpr1, true, 0);

                // compare and store if new minimum
                pos += mainGen->buildInstruction(pos, 0x66, false, true,
                        0x2e, temp_xmm1, temp_xmm2, false, 0);
                pos += mainGen->buildJumpGreaterEqualNear32(pos, 0, skip_offset_pos);
                skip_jmp_pos = pos;
                pos += mainGen->buildInstruction(pos, 0xf2, false, true,
                        0x11, temp_xmm1, temp_gpr1, true, 0);
                *skip_offset_pos = (int32_t)(pos-skip_jmp_pos);

                // load reference maximum value into temp_xmm2
                pos += mainGen->buildMovImm64ToGPR64(pos,
                        (uint64_t)instData.max_addr, temp_gpr1);
                pos += mainGen->buildInstruction(pos, 0xf2, false, true,
                        0x10, temp_xmm2, temp_gpr1, true, 0);

                // compare and store if new maximum
                pos += mainGen->buildInstruction(pos, 0x66, false, true,
                        0x2e, temp_xmm1, temp_xmm2, false, 0);
                pos += mainGen->buildJumpLessEqualNear32(pos, 0, skip_offset_pos);
                skip_jmp_pos = pos;
                pos += mainGen->buildInstruction(pos, 0xf2, false, true,
                        0x11, temp_xmm1, temp_gpr1, true, 0);
                *skip_offset_pos = (int32_t)(pos-skip_jmp_pos);
            }

        }
    }

    pos += buildFakeStackPopXMM(pos, temp_xmm2);
    pos += buildFakeStackPopXMM(pos, temp_xmm1);
    if (temp_gpr1 != REG_EAX) {
        pos += buildFakeStackPopGPR64(pos, temp_gpr1);
    }
    pos += buildFooter(pos);

    // emit original instruction
    opos = orig_code;
    for (i=0; i < origNumBytes; i++) {
        *pos++ = *opos++;
    }
    if (eip_operand) {
        adjustDisplacement(eip_operand->getDisp(), pos);
    }

    pos += buildHeader(pos);
    if (temp_gpr1 != REG_EAX) {
        pos += buildFakeStackPushGPR64(pos, temp_gpr1);
    }
    pos += buildFakeStackPushXMM(pos, temp_xmm1);
    pos += buildFakeStackPushXMM(pos, temp_xmm2);

    // for each operation
    for (i=0; i<inst->numOps; i++) {
        op = (*inst)[i];

        // don't test comparison operands again
        if (op->getType() == OP_CMP  || op->getType() == OP_COMI  ||
            op->getType() == OP_UCOM || op->getType() == OP_UCOMI) continue;

        if (op->numOpSets==0)
            continue;
        
        // for each operand set
        for (j=0; j<1; j++) {
            for (k=0; k<op->opSets[j].nOut; k++) {
                output = op->opSets[j].out[k];

                if (output->getType() != IEEE_Single &&
                    output->getType() != IEEE_Double) {
                    continue;
                }

                // load operand value
                pos += buildOperandLoadXMM(pos, output, temp_xmm1, false);
                if (output->getType() == IEEE_Single) {
                    pos += mainGen->buildCvtss2sd(pos, temp_xmm1, temp_xmm1);
                }

                // load reference minimum value into temp_xmm2
                pos += mainGen->buildMovImm64ToGPR64(pos,
                        (uint64_t)instData.min_addr, temp_gpr1);
                pos += mainGen->buildInstruction(pos, 0xf2, false, true,
                        0x10, temp_xmm2, temp_gpr1, true, 0);

                // compare and store if new minimum
                pos += mainGen->buildInstruction(pos, 0x66, false, true,
                        0x2e, temp_xmm1, temp_xmm2, false, 0);
                pos += mainGen->buildJumpGreaterEqualNear32(pos, 0, skip_offset_pos);
                skip_jmp_pos = pos;
                pos += mainGen->buildInstruction(pos, 0xf2, false, true,
                        0x11, temp_xmm1, temp_gpr1, true, 0);
                *skip_offset_pos = (int32_t)(pos-skip_jmp_pos);

                // load reference maximum value into temp_xmm2
                pos += mainGen->buildMovImm64ToGPR64(pos,
                        (uint64_t)instData.max_addr, temp_gpr1);
                pos += mainGen->buildInstruction(pos, 0xf2, false, true,
                        0x10, temp_xmm2, temp_gpr1, true, 0);

                // compare and store if new maximum
                pos += mainGen->buildInstruction(pos, 0x66, false, true,
                        0x2e, temp_xmm1, temp_xmm2, false, 0);
                pos += mainGen->buildJumpLessEqualNear32(pos, 0, skip_offset_pos);
                skip_jmp_pos = pos;
                pos += mainGen->buildInstruction(pos, 0xf2, false, true,
                        0x11, temp_xmm1, temp_gpr1, true, 0);
                *skip_offset_pos = (int32_t)(pos-skip_jmp_pos);
            }
        }
    }
    
    // TODO: increment count
    pos += mainGen->buildIncMem64(pos, (int32_t)(unsigned long)instData.count_addr);

    pos += buildFakeStackPopXMM(pos, temp_xmm2);
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
     *        getBlobAddress(), (b-getBlobCode()), 
     *        (unsigned)inst->getNumBytes(), inst->getAddress(),
     *        inst->getDisassembly().c_str(), (unsigned)inst->getIndex());
     *printf("  comparing against min [%p] and max [%p]\n",
     *        instData.min_addr,
     *        instData.max_addr);
     */
    return true;
}

string FPAnalysisTRange::finalInstReport()
{
    stringstream ss;
    ss << "TRange: " << insnsInstrumented << " instrumented";
    return ss.str();
}

void FPAnalysisTRange::registerInstruction(FPSemantics *inst)
{
    size_t idx = inst->getIndex();
    stringstream ss("");
    string key;
    if (idx >= instCount) {
        expandInstData(idx+1);
    }
    instData[idx].inst = inst;
    instData[idx].min = INFINITY;
    instData[idx].max = -INFINITY;
    instData[idx].count = 0;

    ss.clear(); ss.str(""); ss << "inst" << dec << idx << "_min_addr";
    key = ss.str();
    ss.clear(); ss.str(configuration->getValue(key));
    ss >> instData[idx].min_addr;
    //cout << "key=" << key << " ";
    if (instData[idx].min_addr) {
        *(double*)(instData[idx].min_addr) = INFINITY;
    }

    ss.clear(); ss.str(""); ss << "inst" << dec << idx << "_max_addr";
    key = ss.str();
    ss.clear(); ss.str(configuration->getValue(key));
    ss >> instData[idx].max_addr;
    //cout << "key=" << key << " ";
    if (instData[idx].max_addr) {
        *(double*)(instData[idx].max_addr) = -INFINITY;
    }

    ss.clear(); ss.str(""); ss << "inst" << dec << idx << "_count_addr";
    key = ss.str();
    ss.clear(); ss.str(configuration->getValue(key));
    ss >> instData[idx].count_addr;
    //cout << "key=" << key << " ";
    if (instData[idx].count_addr) {
        *(unsigned long*)(instData[idx].count_addr) = 0;
    }

    //cout << inst->getDisassembly()
         //<< " min=" << hex << instData[idx].min_addr
         //<< " max=" << hex << instData[idx].max_addr
         //<< endl;
}

void FPAnalysisTRange::handlePreInstruction(FPSemantics *inst)
{
    FPOperation *op;
    FPOperandSet *ops;
    size_t i, j, k;

    //printf("%s\n", inst->getDisassembly().c_str());

    // for each operation ...
    for (i=0; i<inst->numOps; i++) {
        op = (*inst)[i];
        ops = op->opSets;

        // for each input ...
        for (j=0; j<op->numOpSets; j++) {
            for (k=0; k<ops[j].nIn; k++) {
                ops[j].in[k]->refresh(context);
                checkRange(inst, ops[j].in[k]);
            }
        }
    }
}

void FPAnalysisTRange::handlePostInstruction(FPSemantics *inst)
{
    FPOperation *op;
    FPOperandSet *ops;
    size_t i, j, k;

    //printf("%s\n", inst->getDisassembly().c_str());

    // for each operation ...
    for (i=0; i<inst->numOps; i++) {
        op = (*inst)[i];
        ops = op->opSets;

        // for each output ...
        for (j=0; j<op->numOpSets; j++) {
            for (k=0; k<ops[j].nOut; k++) {
                ops[j].out[k]->refresh(context);
                checkRange(inst, ops[j].out[k]);
            }
        }
    }
}

void FPAnalysisTRange::handleReplacement(FPSemantics * /*inst*/)
{ }

void FPAnalysisTRange::checkRange(FPSemantics *inst, FPOperand *op)
{
    long double num;

    // extract operand information
    num = op->getCurrentValueLD();

    size_t idx = inst->getIndex();
    if (num  < instData[idx].min) { instData[idx].min = num;  }
    if (num  > instData[idx].max) { instData[idx].max = num;  }
}

void FPAnalysisTRange::expandInstData(size_t newSize)
{
    FPAnalysisTRangeInstData *newInstData;
    size_t i = 0;
    newSize = newSize > instCount*2 ? newSize : instCount*2;
    //printf("expand_inst_data - old size: %u    new size: %u\n", (unsigned)instCount, (unsigned)newSize);
    newInstData = (FPAnalysisTRangeInstData*)malloc(newSize * sizeof(FPAnalysisTRangeInstData));
    if (!newInstData) {
        fprintf(stderr, "OUT OF MEMORY!\n");
        exit(-1);
    }
    if (instData != NULL) {
        for (; i < instCount; i++) {
            newInstData[i].inst = instData[i].inst;
            newInstData[i].min = instData[i].min;
            newInstData[i].max = instData[i].max;
            newInstData[i].count = instData[i].count;
            newInstData[i].min_addr = instData[i].min_addr;
            newInstData[i].max_addr = instData[i].max_addr;
            newInstData[i].count_addr = instData[i].count_addr;
        }
        free(instData);
        instData = NULL;
    }
    for (; i < newSize; i++) {
        newInstData[i].inst = NULL;
        newInstData[i].min = INFINITY;
        newInstData[i].max = -INFINITY;
        newInstData[i].count = 0;
        newInstData[i].min_addr = NULL;
        newInstData[i].max_addr = NULL;
        newInstData[i].count_addr = NULL;
    }
    instData = newInstData;
    instCount = newSize;
}

void FPAnalysisTRange::finalOutput()
{
    stringstream ss, ss2;
    size_t i;
    unsigned long cnt;
    for (i = 0; i < instCount; i++) {
        if (instData[i].inst) {
            ss.clear();
            ss.str("");
            ss << "RANGE_DATA:" << endl;
            if (instData[i].min_addr) {
                ss << "min=" << *(double*)(instData[i].min_addr) << endl;
            } else {
                ss << "min=" << instData[i].min << endl;
            }
            if (instData[i].max_addr) {
                ss << "max=" << *(double*)(instData[i].max_addr) << endl;
            } else {
                ss << "max=" << instData[i].max << endl;
            }
            if (instData[i].min_addr && instData[i].max_addr) {
                ss << "range=" << (*(double*)(instData[i].max_addr) - 
                                   *(double*)(instData[i].min_addr)) << endl;
            } else {
                ss << "range=" << (instData[i].max - instData[i].min) << endl;
            }
            logFile->addMessage(SUMMARY, 0, "RANGE_DATA", ss.str(), 
                    "", instData[i].inst);

            ss.clear(); ss.str("");
            ss << instData[i].inst->getDisassembly();
            if (instData[i].count_addr) {
                cnt = *(unsigned long*)(instData[i].count_addr);
                ss << "instruction #" << i << ": count=" << cnt;
            } else {
                cnt = instData[i].count;
                ss << "instruction #" << i << ": count=" << cnt;
            }
            logFile->addMessage(ICOUNT, (long)cnt, instData[i].inst->getDisassembly(),
                    ss.str(), "", instData[i].inst);
        }
    }
}

}

