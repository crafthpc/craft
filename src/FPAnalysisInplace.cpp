#include "fpflag.h"
#include "FPAnalysisInplace.h"

#define INT32_MAX  (2147483647)
#define UINT32_MAX (4294967295U)
#define INT32_MIN  (-2147483647-1)
#include "common/src/arch-x86.h"
using namespace NS_x86;

namespace FPInst {

bool FPAnalysisInplace::debugPrint = false;

void FPAnalysisInplace::enableDebugPrint()
{
    debugPrint = true;
}

void FPAnalysisInplace::disableDebugPrint()
{
    debugPrint = false;
}

bool FPAnalysisInplace::isDebugPrintEnabled()
{
    return debugPrint;
}

void FPAnalysisInplace::debugPrintCompare(double input1, double input2, bool ordered, unsigned long flags)
{
    if (FPAnalysisInplace::debugPrint) {
        cout << "FPAnalysisInplace " << (ordered ? "ordered " : "unordered ");
        cout << "compare (rflags=" << hex << flags << dec << "): " << input1 << " <=> "  << input2 << "    ";
        if ((flags & FPContext::FP_CLEAR_FLAGS) == FPContext::FP_FLAGS_UNORDERED) {
            cout << "(unordered)  ";
        } else if ((flags & FPContext::FP_CLEAR_FLAGS) == FPContext::FP_FLAGS_EQUAL) {
            cout << "(equal)  ";
        } else if ((flags & FPContext::FP_CLEAR_FLAGS) == FPContext::FP_FLAGS_GREATER) {
            cout << "(greater)  ";
        } else if ((flags & FPContext::FP_CLEAR_FLAGS) == FPContext::FP_FLAGS_LESS) {
            cout << "(less)  ";
        } else {
            cout << "(unknown?)  ";
        }
        cout << endl;
    }
}

extern "C" {
    FPAnalysisInplace *_INST_Main_InplaceAnalysis = NULL;
    size_t **_INST_svinp_inst_count_ptr_sgl = NULL;
    size_t **_INST_svinp_inst_count_ptr_dbl = NULL;
}

bool FPAnalysisInplace::existsInstance()
{
    return (_INST_Main_InplaceAnalysis != NULL);
}

FPAnalysisInplace* FPAnalysisInplace::getInstance()
{
    if (!_INST_Main_InplaceAnalysis) {
        _INST_Main_InplaceAnalysis = new FPAnalysisInplace();
    }
    return _INST_Main_InplaceAnalysis;
}


FPAnalysisInplace::FPAnalysisInplace()
{
    useLockPrefix = false;
    instCountSize = 0;
    instCountSingle = NULL;
    instCountDouble = NULL;
    reportAllGlobals = false;
    insnsInstrumentedSingle = 0;
    insnsInstrumentedDouble = 0;
}

string FPAnalysisInplace::getTag()
{
    return "sv_inp";
}

string FPAnalysisInplace::getDescription()
{
    return "In-place Shadow Value Analysis";
}

void FPAnalysisInplace::configure(FPConfig *config, FPDecoder *decoder,
        FPLog *log, FPContext *context)
{
    FPAnalysis::configure(config, decoder, log, context);
    if (config->hasReplaceTagTree()) {
        mainPolicy = new FPSVConfigPolicy(config);
    } else if (config->hasValue("sv_inp_type")) {
        string type = config->getValue("sv_inp_type");
        if (type == "single") {
            mainPolicy = new FPSVPolicy(SVT_IEEE_Single);
        } else if (type == "double") {
            mainPolicy = new FPSVPolicy(SVT_IEEE_Double);
        } else if (type == "mem_single") {
            mainPolicy = new FPSVMemPolicy(SVT_IEEE_Single);
        } else if (type == "mem_double") {
            mainPolicy = new FPSVMemPolicy(SVT_IEEE_Double);
        } else if (type == "config") {
            printf("ERROR: config shadow value specified, but no config tree present");
            mainPolicy = new FPSVConfigPolicy(config);
        } else {
            printf("ERROR: unrecognized shadow value type \"%s\"",
                    type.c_str());
            mainPolicy = new FPSVPolicy(SVT_IEEE_Single);
        }
    } else {
        mainPolicy = new FPSVPolicy(SVT_IEEE_Single);
    }
    if (config->getValue("report_all_globals") == "yes") {
        enableReportAllGlobals();
    }
    if (config->getValue("enable_debug_print") == "yes") {
        enableDebugPrint();
    }
    if (config->getValue("use_lock_prefix") == "yes") {
        enableLockPrefix();
    }
    if (config->hasValue("svinp_icount_ptr_sgl")) {
        const char *ptr = config->getValueC("svinp_icount_ptr_sgl");
        _INST_svinp_inst_count_ptr_sgl = (size_t**)strtoul(ptr, NULL, 16);
    }
    if (config->hasValue("svinp_icount_ptr_dbl")) {
        const char *ptr = config->getValueC("svinp_icount_ptr_dbl");
        _INST_svinp_inst_count_ptr_dbl = (size_t**)strtoul(ptr, NULL, 16);
    }
    vector<FPShadowEntry*> entries;
    config->getAllShadowEntries(entries);
    setShadowEntries(entries);
}

void FPAnalysisInplace::enableReportAllGlobals()
{
    reportAllGlobals = true;
}

void FPAnalysisInplace::disableReportAllGlobals()
{
    reportAllGlobals = false;
}

void FPAnalysisInplace::setShadowEntries(vector<FPShadowEntry*> &entries)
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

void FPAnalysisInplace::getShadowEntries(vector<FPShadowEntry*> &entries)
{
    // does not clear return vector first!
    vector<FPShadowEntry*>::iterator i;
    for (i=shadowEntries.begin(); i!=shadowEntries.end(); i++) {
        entries.push_back(*i);
    }
}

bool FPAnalysisInplace::shouldPreInstrument(FPSemantics * /*inst*/)
{
    return false;
}

bool FPAnalysisInplace::shouldPostInstrument(FPSemantics * /*inst*/)
{
    return false;
}

bool FPAnalysisInplace::shouldReplace(FPSemantics *inst)
{
    return mainPolicy->shouldInstrument(inst);
}

FPSVType FPAnalysisInplace::getSVType(FPSemantics *inst)
{
    return mainPolicy->getSVType(inst);
}

FPReplaceEntryTag FPAnalysisInplace::getDefaultRETag(FPSemantics *inst)
{
    return mainPolicy->getDefaultRETag(inst);
}

string FPAnalysisInplace::finalInstReport()
{
    size_t insnsInstrumentedTotal = insnsInstrumentedSingle + insnsInstrumentedDouble;
    stringstream ss;
    ss << "Inplace: " << insnsInstrumentedTotal << " instrumented";
    ss << setiosflags(ios::fixed) << setprecision(3);
    ss << "  (" << insnsInstrumentedSingle
       << " [" << (insnsInstrumentedSingle == 0 ? 0 :
                    (double)insnsInstrumentedSingle/
                    (double)insnsInstrumentedTotal*100.0) << "%]"
       << " single";
    ss << ", " << insnsInstrumentedDouble
       << " [" << (insnsInstrumentedDouble == 0 ? 0 :
                    (double)insnsInstrumentedDouble/
                    (double)insnsInstrumentedTotal*100.0) << "%]"
       << " double)";
    return ss.str();
}

void FPAnalysisInplace::registerInstruction(FPSemantics *inst)
{
    if (inst->getIndex() >= instCountSize) {
        expandInstCount(inst->getIndex());
    }
}

void FPAnalysisInplace::handlePreInstruction(FPSemantics * /*inst*/)
{ }

void FPAnalysisInplace::handlePostInstruction(FPSemantics * /*inst*/)
{ }

void FPAnalysisInplace::handleReplacement(FPSemantics *inst)
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
    if (debugPrint) {
        inst->getNeededRegisters(regs);
        cout << "started inplace analysis handling at " << hex << inst->getAddress() << dec;
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

FPBinaryBlobInplace::FPBinaryBlobInplace(FPSemantics *inst, FPSVPolicy *policy)
    : FPBinaryBlob(inst)
{
    this->mainPolicy = policy;
    this->useLockPrefix = false;
}

void FPBinaryBlobInplace::enableLockPrefix()
{
    useLockPrefix = true;
}

void FPBinaryBlobInplace::disableLockPrefix()
{
    useLockPrefix = false;
}

void FPAnalysisInplace::enableLockPrefix()
{
    useLockPrefix = true;
}

void FPAnalysisInplace::disableLockPrefix()
{
    useLockPrefix = false;
}

size_t FPBinaryBlobInplace::buildInitBlobSingle(unsigned char *pos,
        FPRegister dest, long tag)
{
    // clobbers %rax, %rbx, and $rflags
    
    unsigned char *old_pos = pos;
    unsigned char *skip_jmp_pos = 0;
    int32_t *skip_offset_pos = 0;

    // extract the value of interest
    if (tag == 2) {
        pos += buildExtractGPR64FromXMM(pos, temp_gpr1, dest, tag);
    } else {
        pos += mainGen->buildMovXmmToGPR64(pos, dest, temp_gpr1);
    }

    // check for flag
    pos += mainGen->buildMovImm64ToGPR64(pos, IEEE64_FLAG_MASK, temp_gpr2);
    pos += mainGen->buildAndGPR64WithGPR64(pos, temp_gpr2, temp_gpr1);
    pos += mainGen->buildMovImm64ToGPR64(pos, IEEE64_FLAG, temp_gpr2);
    pos += mainGen->buildCmpGPR64WithGPR64(pos, temp_gpr2, temp_gpr1);

    // skip conversion if flag is present
    pos += mainGen->buildJumpEqualNear32(pos, 0, skip_offset_pos);
    skip_jmp_pos = pos;  // save jump position for later

    // preserve low value if we're converting the high one
    if (tag == 2) {
        pos += mainGen->buildMovXmmToGPR64(pos, dest, temp_gpr2);
    }

    if (tag == 2) {
        // convert high value (clobbers low one)
        pos += mainGen->buildCvtpd2ps(pos, dest, dest);
        pos += mainGen->buildUnpcklps(pos, dest, dest);
    } else {
        // convert low value (automatically preserves high one)
        pos += mainGen->buildCvtsd2ss(pos, dest, dest);
    }

    // restore low value if we just converted the high one
    if (tag == 2) {
        pos += buildInsertGPR64IntoXMM(pos, temp_gpr2, dest, 0);
    }

    // add "replaced" flag
    pos += mainGen->buildMovImm32ToGPR32(pos, IEEE32_FLAG, temp_gpr1);
    pos += buildInsertGPR32IntoXMM(pos, temp_gpr1, dest, (tag==2 ? 3 : 1), temp_gpr3);

    // jump target
    *skip_offset_pos = (int32_t)(pos-skip_jmp_pos);  // stitch up jump

    return size_t(pos-old_pos);
}

size_t FPBinaryBlobInplace::buildInitBlobDouble(unsigned char *pos,
        FPRegister dest, long tag)
{
    // clobbers %rax, %rbx, and $rflags
    
    unsigned char *old_pos = pos;
    unsigned char *skip_jmp_pos = 0;
    int32_t *skip_offset_pos = 0;

    // extract the value of interest
    if (tag == 2) {
        pos += buildExtractGPR64FromXMM(pos, temp_gpr1, dest, tag);
    } else {
        pos += mainGen->buildMovXmmToGPR64(pos, dest, temp_gpr1);
    }

    // check for flag
    pos += mainGen->buildMovImm64ToGPR64(pos, IEEE64_FLAG_MASK, temp_gpr2);
    pos += mainGen->buildAndGPR64WithGPR64(pos, temp_gpr2, temp_gpr1);
    pos += mainGen->buildMovImm64ToGPR64(pos, IEEE64_FLAG, temp_gpr2);
    pos += mainGen->buildCmpGPR64WithGPR64(pos, temp_gpr2, temp_gpr1);

    // skip conversion if flag is not present
    pos += mainGen->buildJumpNotEqualNear32(pos, 0, skip_offset_pos);
    skip_jmp_pos = pos;  // save jump position for later

    // preserve low value if we're converting the high one
    if (tag == 2) {
        pos += mainGen->buildMovXmmToGPR64(pos, dest, temp_gpr2);
    }

    if (tag == 2) {
        // convert high value (clobbers low one)
        pos += mainGen->buildUnpckhps(pos, dest, dest);
        pos += mainGen->buildCvtps2pd(pos, dest, dest);
    } else {
        // convert low value (automatically preserves high one)
        pos += mainGen->buildCvtss2sd(pos, dest, dest);
    }

    // restore low value if we just converted the high one
    if (tag == 2) {
        pos += buildInsertGPR64IntoXMM(pos, temp_gpr2, dest, 0);
    }

    // jump target
    *skip_offset_pos = (int32_t)(pos-skip_jmp_pos);  // stitch up jump

    return size_t(pos-old_pos);
}

size_t FPBinaryBlobInplace::buildFlagTestBlob(unsigned char *pos,
        FPRegister dest, long tag)
{
    // clobbers %rax, %rbx, and $rflags
    
    assert(tag == 1 || tag == 3);
    unsigned char *old_pos = pos;
    unsigned char *skip_jmp_pos = 0;
    int32_t *skip_offset_pos = 0;

    // extract the value of interest
    if (tag == 3) {
        pos += buildExtractGPR64FromXMM(pos, temp_gpr1, dest, 2);
    } else {
        pos += mainGen->buildMovXmmToGPR64(pos, dest, temp_gpr1);
    }

    // check for flag
    pos += mainGen->buildMovImm64ToGPR64(pos, IEEE64_FLAG_MASK, temp_gpr2);
    pos += mainGen->buildAndGPR64WithGPR64(pos, temp_gpr2, temp_gpr1);
    pos += mainGen->buildMovImm64ToGPR64(pos, IEEE64_FLAG, temp_gpr2);
    pos += mainGen->buildCmpGPR64WithGPR64(pos, temp_gpr2, temp_gpr1);

    // skip communication if flag is not present
    pos += mainGen->buildJumpNotEqualNear32(pos, 0, skip_offset_pos);
    skip_jmp_pos = pos;  // save jump position for later

    // save comm flag to stack
    pos += mainGen->buildInstruction(pos, 0, false, false, 
            0x8b, temp_gpr1, REG_ESP, true, getBlobCommOffset());
    pos += mainGen->buildMovImm64ToGPR64(pos, 1<<tag, temp_gpr2);
    pos += mainGen->buildOrGPR64WithGPR64(pos, temp_gpr2, temp_gpr1);
    pos += mainGen->buildInstruction(pos, 0, false, false, 
            0x89, temp_gpr1, REG_ESP, true, getBlobCommOffset());

    // jump target
    *skip_offset_pos = (int32_t)(pos-skip_jmp_pos);  // stitch up jump

    return size_t(pos-old_pos);
}

size_t FPBinaryBlobInplace::buildOperandInitBlob(unsigned char *pos,
        FPInplaceBlobInputEntry entry, FPSVType replacementType)
{
    // clobbers %rax, %rbx, and $rflags

    FPOperand *operand = entry.input;
    FPRegister dest = entry.reg;
    bool packed = entry.packed;

    /*
     *printf("buildOperandInitBlob: %s  %s  reg=%s packed=%s\n",
     *        operand->toString().c_str(),
     *        FPContext::FPReg2Str(operand->getRegister()).c_str(),
     *        FPContext::FPReg2Str(dest).c_str(),
     *        (packed ? "yes" : "no"));
     */
    
    unsigned char *old_pos = pos;
    if (isSSE(dest)) {
        pos += buildOperandLoadXMM(pos, operand, dest, packed);
        debug_size = buildOperandLoadXMM(debug_code, operand, dest, packed);
        if (debug_size) {
            debug_assembly += FPDecoderXED::disassemble(debug_code, debug_size) + "\n";
        }
    } else if (isGPR(dest)) {
        pos += buildOperandLoadGPR(pos, operand, dest);
        debug_size = buildOperandLoadGPR(debug_code, operand, dest);
        if (debug_size) {
            debug_assembly += FPDecoderXED::disassemble(debug_code, debug_size) + "\n";
        }
    } else {
        assert(!"Unsupported temporary register");
    }

    //printf("       loading operand: %s\n", operand->toString().c_str());
    //printf("         disassembly: %s\n", debug_assembly.c_str());

    if (operand->getType() == IEEE_Double) {
        assert(isSSE(dest));

        if (replacementType == SVT_IEEE_Single) {
            // down-cast if necessary
            if (entry.packed) {
                pos += buildInitBlobSingle(pos, dest, 2);
            }
            pos += buildInitBlobSingle(pos, dest, 0);
        } else { // if SVT_IEEE_Double
            // up-cast if necessary
            if (entry.packed) {
                pos += buildInitBlobDouble(pos, dest, 2);
            }
            pos += buildInitBlobDouble(pos, dest, 0);
        }
   
    } else if (operand->getType() == SSE_Quad) {

        if (replacementType == SVT_IEEE_Single) {
            pos += buildFlagTestBlob(pos, dest, 1);
            pos += buildFlagTestBlob(pos, dest, 3);
        }
    }

    return size_t(pos-old_pos);
}

inline bool isSegmentRegister(unsigned char prefix) {
    return (prefix == 0x2e ||
            prefix == 0x3e ||
            prefix == 0x26 ||
            prefix == 0x64 ||
            prefix == 0x65 ||
            prefix == 0x36);
}

size_t FPBinaryBlobInplace::buildReplacedInstruction(unsigned char *pos,
        FPSemantics * /*inst*/, unsigned char *orig_code,
        FPSVType replacementType, FPRegister replacementRM, bool changePrecision)
{
    // emits a replaced version of an instruction that replaces any memory
    // operands with registers, and does the operation in the analysis-desired
    // precision (if the changePrecision flag is set)
    //
    unsigned char *old_pos = pos;
    unsigned char prefix, rex, modrm, opcode;
    int i;
    unsigned j;

    // decode instruction using Dyninst internal decoder
    struct ia32_memacc memacc[3];
    struct ia32_condition cond;
    struct ia32_locations loc;
    ia32_instruction orig_instr(memacc, &cond, &loc);
    ia32_decode(IA32_DECODE_MEMACCESS | IA32_DECODE_CONDITION,
        orig_code, orig_instr);
    opcode = orig_code[loc.num_prefixes+(int)loc.opcode_size-1];

    // cannot have non-segment prefixes
    for (j=0; (j < 4) && (j < orig_instr.getPrefix()->getCount()); j++) {
        prefix = orig_instr.getPrefix()->getPrefix(j);
        assert(prefix == 0 || isSegmentRegister(prefix));
    }

    assert(loc.modrm_position != -1);   // must have a ModRM byte
    assert(loc.address_size != 1);      // can't be a 16-bit instruction

    // copy all prefixes except for REX and segments, and also
    // convert 0xf2 to 0xf3 (double->single arithmetic)
    for (i=0; i<loc.num_prefixes; i++) {
        //printf("  prefix 0x%02x %s\n", orig_code[i], (i == loc.rex_position ? "[rex]" : ""));
        if (i != loc.rex_position) {
            if (orig_code[i] == 0xf2 && replacementType == SVT_IEEE_Single && changePrecision) {
                (*pos++) = 0xf3;
            } else if (!isSegmentRegister(orig_code[i])) {
                (*pos++) = orig_code[i];
            }
        }
    }

    // skip the 0x66 byte if present (double->single cvt)
    // there are a couple of exceptions
    if (orig_code[i] == 0x66) {
        if (replacementType != SVT_IEEE_Single ||
            changePrecision == false ||
            opcode == 0xdb ||      // pand
            opcode == 0xeb ||      // por
            opcode == 0xef) {      // pxor
            (*pos++) = 0x66;
        }
        i++;
    }
    
    // build/emit REX byte
    // NOTE: REX.X will always be zero (no memory operands)
    // TODO: handle REX.W bit for non-replaced instructions?
    //printf("  rex_position=%d\n", loc.rex_position);
    if (loc.rex_position != -1) {
        rex = orig_code[loc.rex_position];
        if (replacementRM != REG_NONE) {
            rex &= 0xfe;    // clear REX.B
            if (mainGen->getRegModRMId(replacementRM) > 0x7) {
                rex |= 0x01;    // set REX.B
            }
        }
        if (rex != 0x40) {
            (*pos++) = rex;
        }
    } else {
        pos += mainGen->buildREX(pos, false,
                REG_NONE, REG_NONE, replacementRM);
    }
    
    // copy/modify the opcode, 
    for ( ; i<loc.num_prefixes+(int)loc.opcode_size; i++) {
        //printf("  opcode 0x%02x\n", orig_code[i]);
        if (orig_code[i] == 0x5a && replacementType == SVT_IEEE_Single && changePrecision) {
            (*pos++) = 0x10;        // cvt ->  mov
        } else {
            (*pos++) = orig_code[i];
        }
    }
    
    // build/emit Mod/RM byte
    modrm = 0xc0;
    modrm |= (loc.modrm_reg & 0x7) << 3;    // always preserve reg
    if (replacementRM != REG_NONE) {
        // replace r/m
        modrm |= mainGen->getRegModRMId(replacementRM) & 0x7;
    } else {
        // preserve r/m
        modrm |= (loc.modrm_rm & 0x7);
    }
    (*pos++) = modrm; i++;
    
    // immediate(s)
    for (j=0; j<loc.imm_size[0]; j++) {
        (*pos++) = orig_code[loc.imm_position[0] + j]; i++;
    }
    for (j=0; j<loc.imm_size[1]; j++) {
        (*pos++) = orig_code[loc.imm_position[1] + j]; i++;
    }

    // debug output
    /*
     *unsigned char *tc;
     *printf("  new instruction for %s: ", inst->getDisassembly().c_str());
     *for (tc = orig_code; tc < (orig_code+i); tc++) {
     *  printf("%02x ", (unsigned)*tc);
     *}
     *printf(" => ");
     *for (tc = old_pos; tc < pos; tc++) {
     *  printf("%02x ", (unsigned)*tc);
     *}
     *printf("\n");
     */

    return (size_t)(pos-old_pos);
}

size_t FPBinaryBlobInplace::buildFixSSEQuadOutput(unsigned char *pos,
        FPRegister xmm)
{
    unsigned char *old_pos = pos;
    unsigned char *skip1_jmp_pos = 0, *skip2_jmp_pos = 0;
    int32_t *skip1_offset_pos = 0, *skip2_offset_pos = 0;

    // grab flags saved during initialization
    pos += mainGen->buildInstruction(pos, 0, false, false, 
            0x8b, temp_gpr1, REG_ESP, true, getBlobCommOffset());

    // check for slot 1 flag
    pos += mainGen->buildMovImm64ToGPR64(pos, 0x1, temp_gpr2);
    pos += mainGen->buildAndGPR64WithGPR64(pos, temp_gpr2, temp_gpr1);
    pos += mainGen->buildCmpGPR64WithGPR64(pos, temp_gpr2, temp_gpr1);

    // skip "replaced" flag set if saved flag is not present
    pos += mainGen->buildJumpNotEqualNear32(pos, 0, skip1_offset_pos);
    skip1_jmp_pos = pos;  // save jump position for later

    // save "replaced" flag to register
    pos += mainGen->buildMovImm64ToGPR64(pos, IEEE64_FLAG, temp_gpr1);
    buildInsertGPR32IntoXMM(pos, temp_gpr1, xmm, 1, temp_gpr3);

    // jump target
    *skip1_offset_pos = (int32_t)(pos-skip1_jmp_pos);  // stitch up jump

    // grab flags saved during initialization
    pos += mainGen->buildInstruction(pos, 0, false, false, 
            0x8b, temp_gpr1, REG_ESP, true, getBlobCommOffset());

    // check for slot 1 flag
    pos += mainGen->buildMovImm64ToGPR64(pos, 0x3, temp_gpr2);
    pos += mainGen->buildAndGPR64WithGPR64(pos, temp_gpr2, temp_gpr1);
    pos += mainGen->buildCmpGPR64WithGPR64(pos, temp_gpr2, temp_gpr1);

    // skip "replaced" flag set if saved flag is not present
    pos += mainGen->buildJumpNotEqualNear32(pos, 0, skip2_offset_pos);
    skip2_jmp_pos = pos;  // save jump position for later

    // save "replaced" flag to register
    pos += mainGen->buildMovImm64ToGPR64(pos, IEEE64_FLAG, temp_gpr1);
    buildInsertGPR32IntoXMM(pos, temp_gpr1, xmm, 3, temp_gpr3);

    // jump target
    *skip2_offset_pos = (int32_t)(pos-skip2_jmp_pos);  // stitch up jump

    return (size_t)(pos-old_pos);
}

size_t FPBinaryBlobInplace::buildInsertGPR32IntoXMM(unsigned char *pos,
        FPRegister gpr, FPRegister xmm, long tag, FPRegister tmp)
{
    assert(gpr >= REG_EAX && gpr <= REG_EDI);
    assert(xmm >= REG_XMM0 && xmm <= REG_XMM15);
    assert(tag >= 0 && tag <= 3);
    unsigned char *old_pos = pos;

    uint64_t mask = 0xFFFFFFFF;

    // keep only 32 bits of gpr
    pos += mainGen->buildMovImm64ToGPR64(pos, mask, tmp);
    pos += mainGen->buildAndGPR64WithGPR64(pos, tmp, gpr);
    if (tag == 1 || tag == 3) {
        pos += mainGen->buildInstruction(pos, 0, true, false, 0xc1, (FPRegister)0, gpr, false, 0);
        *(int8_t*)pos = 32; pos++;
    }
    
    // push xmm on stack
    fake_stack_offset -= 8;
    if (tag < 2) {
        // movlpd
        pos += mainGen->buildInstruction(pos, 0x66, false, true, 0x13, xmm, REG_ESP, true, fake_stack_offset);
    } else {
        // movhpd
        pos += mainGen->buildInstruction(pos, 0x66, false, true, 0x17, xmm, REG_ESP, true, fake_stack_offset);
    }

    // keep only 32 bits of stack
    pos += mainGen->buildMovImm64ToGPR64(pos, mask, tmp);
    if (tag == 0 || tag == 2) {
        pos += mainGen->buildInstruction(pos, 0, true, false, 0xc1, (FPRegister)0, tmp, false, 0);
        *(int8_t*)pos = 32; pos++;
    }
    pos += mainGen->buildInstruction(pos, 0, true, false, 0x21, tmp, REG_ESP, true, fake_stack_offset);

    // or/add stack and gpr
    pos += mainGen->buildInstruction(pos, 0, true, false, 0x09, gpr, REG_ESP, true, fake_stack_offset);

    // put stack back in xmm
    if (tag < 2) {
        // movlpd
        pos += mainGen->buildInstruction(pos, 0x66, false, true, 0x12, xmm, REG_ESP, true, fake_stack_offset);
    } else {
        // movhpd
        pos += mainGen->buildInstruction(pos, 0x66, false, true, 0x16, xmm, REG_ESP, true, fake_stack_offset);
    }
    fake_stack_offset += 8;

    return (size_t)(pos-old_pos);
}

size_t FPBinaryBlobInplace::buildExtractGPR64FromXMM(unsigned char *pos,
        FPRegister gpr, FPRegister xmm, long tag)
{
    assert(gpr >= REG_EAX && gpr <= REG_E15);
    assert(xmm >= REG_XMM0 && xmm <= REG_XMM15);
    assert(tag == 0 || tag == 2);
    unsigned char *old_pos = pos;

    // push xmm on stack
    fake_stack_offset -= 8;
    if (tag == 0) {
        // movlpd
        pos += mainGen->buildInstruction(pos, 0x66, false, true, 0x13, xmm, REG_ESP, true, fake_stack_offset);
    } else {
        // movhpd
        pos += mainGen->buildInstruction(pos, 0x66, false, true, 0x17, xmm, REG_ESP, true, fake_stack_offset);
    }

    // pop stack into gpr
    pos += buildFakeStackPopGPR64(pos, gpr);

    return (size_t)(pos-old_pos);
}

size_t FPBinaryBlobInplace::buildInsertGPR64IntoXMM(unsigned char *pos,
        FPRegister gpr, FPRegister xmm, long tag)
{
    assert(gpr >= REG_EAX && gpr <= REG_E15);
    assert(xmm >= REG_XMM0 && xmm <= REG_XMM15);
    assert(tag == 0 || tag == 2);
    unsigned char *old_pos = pos;
    
    // push gpr on stack
    pos += buildFakeStackPushGPR64(pos, gpr);

    // put stack back in xmm
    if (tag == 0) {
        // movlpd
        pos += mainGen->buildInstruction(pos, 0x66, false, true, 0x12, xmm, REG_ESP, true, fake_stack_offset);
    } else {
        // movhpd
        pos += mainGen->buildInstruction(pos, 0x66, false, true, 0x16, xmm, REG_ESP, true, fake_stack_offset);
    }
    fake_stack_offset += 8;

    return (size_t)(pos-old_pos);
}

size_t FPBinaryBlobInplace::buildSpecialCvtss2sd(unsigned char *pos,
        unsigned char orig_prefix, unsigned char orig_modrm)
{
    unsigned char *old_pos = pos;

    // we don't actually need to calculate the source operand because it is
    // passed transparently to the movss and we don't touch it afterwards
    //FPRegister src  = (FPRegister)(REG_XMM0 + (orig_modrm & 0x7));
    
    FPRegister dest = (FPRegister)(REG_XMM0 + ((orig_modrm & 0x38) >> 3));
    
    if (orig_prefix) {
        //src = (FPRegister)(src + 8);
        dest = (FPRegister)(dest + 8);
    }

    /*
     *printf("changing cvtss2sd %s -> %s to movss\n",
     *        FPContext::FPReg2Str(src).c_str(),
     *        FPContext::FPReg2Str(dest).c_str());
     */

    if (orig_prefix) {
        (*pos++) = orig_prefix;
    }
    (*pos++) = 0xf3;            // movss *, *
    (*pos++) = 0x0f;
    (*pos++) = 0x10;
    (*pos++) = orig_modrm;

    // add the tag
    pos += mainGen->buildPushReg(pos, temp_gpr1);
    pos += mainGen->buildMovImm32ToGPR32(pos, IEEE32_FLAG, temp_gpr1);
    pos += buildInsertGPR32IntoXMM(pos, temp_gpr1, dest, 1, temp_gpr3);
    pos += mainGen->buildPopReg(pos, temp_gpr1);

    return (size_t)(pos-old_pos);
}

size_t FPBinaryBlobInplace::buildSpecialCvtsd2ss(unsigned char *pos,
        unsigned char orig_prefix, unsigned char orig_modrm)
{
    unsigned char *old_pos = pos;

    // we don't actually need to calculate the source operand because it is
    // passed transparently to the movss and we don't touch it afterwards
    //FPRegister src  = (FPRegister)(REG_XMM0 + (orig_modrm & 0x7));
    
    FPRegister dest = (FPRegister)(REG_XMM0 + ((orig_modrm & 0x38) >> 3));
    
    if (orig_prefix) {
        //src = (FPRegister)(src + 8);
        dest = (FPRegister)(dest + 8);
    }

    /*
     *printf("changing cvtsd2ss %s -> %s to movss\n",
     *        FPContext::FPReg2Str(src).c_str(),
     *        FPContext::FPReg2Str(dest).c_str());
     */

    if (orig_prefix) {
        (*pos++) = orig_prefix;
    }
    (*pos++) = 0xf3;            // movss *, *
    (*pos++) = 0x0f;
    (*pos++) = 0x10;
    (*pos++) = orig_modrm;

    // no need to add a tag

    return (size_t)(pos-old_pos);
}

size_t FPBinaryBlobInplace::buildSpecialNegate(unsigned char *pos,
        FPOperand *src, FPOperand *dest)
{
    unsigned char *old_pos = pos;
    unsigned char *jmp32_pos = 0;
    int32_t *offset32_pos = 0;
    unsigned char *jmpend_pos = 0;
    int32_t *offsetend_pos = 0;
    FPRegister dest_gpr = dest->getRegister();

    pos += mainGen->buildPushReg(pos, temp_gpr1);
    pos += mainGen->buildPushReg(pos, temp_gpr2);

    // move to destination
    if (src->getRegister() != dest_gpr) {
        pos += mainGen->buildMovGPR64ToGPR64(pos, src->getRegister(), dest_gpr);
    }

    // check for flag
    pos += mainGen->buildMovGPR64ToGPR64(pos, dest_gpr, temp_gpr1);
    pos += mainGen->buildMovImm64ToGPR64(pos, IEEE64_FLAG_MASK, temp_gpr2);
    pos += mainGen->buildAndGPR64WithGPR64(pos, temp_gpr2, temp_gpr1);
    pos += mainGen->buildMovImm64ToGPR64(pos, IEEE64_FLAG, temp_gpr2);
    pos += mainGen->buildCmpGPR64WithGPR64(pos, temp_gpr2, temp_gpr1);
    pos += mainGen->buildJumpEqualNear32(pos, 0, offset32_pos);
    jmp32_pos = pos;  // save jump position for later

    // flip 64-bit flag (BTC)
    pos += mainGen->buildREX(pos, true, REG_NONE, REG_NONE, dest_gpr);
    (*pos++) = 0x0f;
    (*pos++) = 0xba;
    pos += mainGen->buildModRM(pos, 0x3, REG_EDI, dest_gpr);
    (*pos++) = 0x3f;
    pos += mainGen->buildJumpEqualNear32(pos, 0, offsetend_pos);

    jmpend_pos = pos;  // save jump position for later

    *offset32_pos = (int32_t)(pos-jmp32_pos);  // stitch up jump

    // flip 32-bit flag (BTC)
    pos += mainGen->buildREX(pos, true, REG_NONE, REG_NONE, dest_gpr);
    (*pos++) = 0x0f;
    (*pos++) = 0xba;
    pos += mainGen->buildModRM(pos, 0x3, REG_EDI, dest_gpr);
    (*pos++) = 0x1f;

    // jump target
    *offsetend_pos = (int32_t)(pos-jmpend_pos);  // stitch up jump

    pos += mainGen->buildPopReg(pos, temp_gpr2);
    pos += mainGen->buildPopReg(pos, temp_gpr1);

    return (size_t)(pos-old_pos);
}

size_t FPBinaryBlobInplace::buildSpecialZero(unsigned char *pos,
        FPOperand *dest)
{
    unsigned char *old_pos = pos;
    FPRegister dest_xmm = dest->getRegister();
    long dest_tag = dest->getTag();

    pos += buildHeader(pos);

    // save one temporary register
    if (temp_gpr1 != REG_EAX) {         // $rax is already saved in buildHeader()
        pos += buildFakeStackPushGPR64(pos, temp_gpr1);
    }

    switch (dest->type) {
        case IEEE_Double:
        case SignedInt64:
        case UnsignedInt64:
            pos += mainGen->buildMovImm64ToGPR64(pos, 0, temp_gpr1);
            pos += buildInsertGPR64IntoXMM(pos, temp_gpr1, dest_xmm, dest_tag);
            break;
        case IEEE_Single:
        case SignedInt32:
        case UnsignedInt32:
            // need an extra temp register for this (bit fiddling b/c of lack of PINSR)
            pos += buildFakeStackPushGPR64(pos, temp_gpr3);
            pos += mainGen->buildMovImm32ToGPR32(pos, 0, temp_gpr1);
            pos += buildInsertGPR32IntoXMM(pos, temp_gpr1, dest_xmm, dest_tag, temp_gpr3);
            pos += buildFakeStackPopGPR64(pos, temp_gpr3);
            break;
        default:
            assert(!"Unhandled operand type in special zero");
    }

    if (temp_gpr1 != REG_EAX) {
        pos += buildFakeStackPopGPR64(pos, temp_gpr1);
    }
    pos += buildFooter(pos);

    return (size_t)(pos-old_pos);
}

size_t FPBinaryBlobInplace::buildSpecialOp(unsigned char *pos,
        FPOperation *op, bool packed, bool &replaced)
{
    unsigned char *old_pos = pos;

    // special case: BTC (bit test & complement--used for fast negation)
    if (op->type == OP_NEG && !packed &&
            op->opSets[0].in[0]->isRegisterGPR() &&
            op->opSets[0].out[0]->isRegisterGPR()) {
        printf("BUILDING SPECIAL NEGATE!\n");
        pos += buildSpecialNegate(pos, op->opSets[0].in[0], op->opSets[0].out[0]);
        replaced = true;

    // special case: fast zero
    } else if (op->type == OP_ZERO && !packed &&
            op->opSets[0].out[0]->isRegisterSSE()) {
        printf("BUILDING SPECIAL ZERO!\n");
        pos += buildSpecialZero(pos, op->opSets[0].out[0]);
    }

    return (size_t)(pos-old_pos);
}

void FPBinaryBlobInplace::addBlobInputEntry(vector<FPInplaceBlobInputEntry> &inputs,
        FPOperand *input, bool packed, FPRegister &replacementRM)
{
    if (input->type == IEEE_Double) {
        // initialize all double-precision input operands
        if (input->isMemory()) {
            replacementRM = getUnusedSSE();
            inputs.push_back(FPInplaceBlobInputEntry(input, inst, replacementRM, packed));
        } else if (input->isRegisterSSE() || input->isRegisterGPR()) {
            // TODO: should probably check for identical operands
            // (no need to initialize it twice)
            inputs.push_back(FPInplaceBlobInputEntry(input, inst, input->getRegister(), packed));
        } else {
            assert(!"Can't handle double-precision non-memory and non-XMM operands");
        }

    } else if (input->type == SSE_Quad) {
        // initialize all double-precision input operands
        if (input->isMemory()) {
            replacementRM = getUnusedSSE();
            inputs.push_back(FPInplaceBlobInputEntry(input, inst, replacementRM, true));
        } else if (input->isRegisterSSE() || input->isRegisterGPR()) {
            // TODO: should probably check for identical operands
            // (no need to initialize it twice)
            inputs.push_back(FPInplaceBlobInputEntry(input, inst, input->getRegister(), true));
        } else {
            assert(!"Can't handle XMM non-memory and non-XMM operands");
        }

    } else if (input->isMemory()) {
        switch (input->type) {
            case IEEE_Single:
                // put single-precision memory operands in SSE registers
                replacementRM = getUnusedSSE();
                inputs.push_back(FPInplaceBlobInputEntry(input, inst, replacementRM, packed));
                break;
            case SignedInt32:
            case UnsignedInt32:
            case SignedInt64:
            case UnsignedInt64:
                // put integer memory operands in GPR registers
                replacementRM = getUnusedGPR();
                inputs.push_back(FPInplaceBlobInputEntry(input, inst, replacementRM, packed));
                break;

            default:
                assert(!"Can't handle non-SSE or integer memory operands");
                break;
        }
    }
}

size_t FPBinaryBlobInplace::buildReplacedOperation(unsigned char *pos,
                FPOperation *op, FPSVType replacementType,
                unsigned char *orig_code, size_t origNumBytes,
                vector<FPInplaceBlobInputEntry> &inputs, FPRegister replacementRM, 
                bool packed, bool only_movement, bool mem_output, bool xmm_output,
                bool &special, bool &replaced)
{
    unsigned char *old_pos = pos;

    FPOperand *output = op->opSets[0].out[0];
    size_t i;

    // PRE-INSTRUCTION INSTRUMENTATION
    
    // save any temporary XMM/GPR register
    // needs to be before the scratch registers because it needs to be
    // saved around the entire blob
    if (replacementRM != REG_NONE) {
        // TODO: somehow check to see if it's actually live?
        if (isGPR(replacementRM)) {
            pos += buildFakeStackPushGPR64(pos, replacementRM);
        } else {
            pos += buildFakeStackPushXMM(pos, replacementRM);
        }
    }

    // save clobbered scratch registers
    // TODO: don't push/pop scratch registers if they are RAX
    pos += buildFakeStackPushGPR64(pos, temp_gpr1);
    pos += buildFakeStackPushGPR64(pos, temp_gpr2);
    pos += buildFakeStackPushGPR64(pos, temp_gpr3);

    // initialize input operands
    vector<FPInplaceBlobInputEntry>::iterator it;
    for (it = inputs.begin(); it != inputs.end(); it++) {
        pos += buildOperandInitBlob(pos, *it, replacementType);
    }

    // find and increment appropriate instruction counter
    // TODO: pull this out of buildReplacedOperation?
    size_t **count_ptr = NULL;
    if (replacementType == SVT_IEEE_Single) {
        count_ptr = _INST_svinp_inst_count_ptr_sgl;
    } else if (replacementType == SVT_IEEE_Double) {
        count_ptr = _INST_svinp_inst_count_ptr_dbl;
    }
    unsigned char prefix = 0x0;
    if (useLockPrefix) {
        prefix = 0xf0;  // add LOCK prefix if requested
    }
    if (count_ptr != NULL) {
        // grab the array pointer
        pos += mainGen->buildMovImm64ToGPR64(pos, 
                (uint64_t)count_ptr, temp_gpr1);
        // dereference the pointer
        pos += mainGen->buildInstruction(pos, 0, true, false,
                0x8b, temp_gpr2, temp_gpr1, true, 0);
        // increment appropriate count slot
        pos += mainGen->buildInstruction(pos, prefix, true, false,
                0xff, REG_NONE, temp_gpr2, true,
                (uint32_t)(inst->getIndex() * sizeof(size_t)));
    }

    // restore clobbered registers
    pos += buildFakeStackPopGPR64(pos, temp_gpr3);
    pos += buildFakeStackPopGPR64(pos, temp_gpr2);
    pos += buildFakeStackPopGPR64(pos, temp_gpr1);

    // ORIGINAL/REPLACED INSTRUCTION
    
    // TODO: do we really need to restore flags?
    // (will original instruction ever need them?)
    
    if (FPDecoderXED::readsFlags(inst) || FPDecoderXED::writesFlags(inst)) {
        pos += mainGen->buildRestoreFlagsFast(pos, getSavedFlagsOffset());
    }

    // restore RAX (TODO: not always necessary)
    pos += mainGen->buildMovStackToGPR64(pos, REG_EAX, getSavedEAXOffset());

    if (!replaced && only_movement && xmm_output &&
            (inst->getDisassembly().find("movhpd") != string::npos ||
             inst->getDisassembly().find("movlpd") != string::npos)) {
        // this is a really ugly hack; the movhpd/movlpd instructions
        // are the only ones that don't have xmm->xmm capability,
        // which means that the mem -> xmm can't be implemented the same
        // way it is for other instructions
        pos += buildFakeStackPushGPR64(pos, temp_gpr1);
        pos += buildExtractGPR64FromXMM(pos, temp_gpr1, replacementRM, 0);
        pos += buildInsertGPR64IntoXMM(pos, temp_gpr1, output->getRegister(), output->getTag());
        pos += buildFakeStackPopGPR64(pos, temp_gpr1);
        special = true;
        replaced = true;
    }

    if (!replaced && mem_output && only_movement) {
        // re-emit original instruction (buildReplacedInstruction does not
        // handle memory output operands)
        assert(op->type == OP_MOV);
        unsigned char *orig = orig_code;
        for (i=0; i<origNumBytes; i++) {
            *pos++ = *orig++;
        }
        if (output->getBase() == REG_EIP) {
            adjustDisplacement(output->getDisp(), pos);
        }
        special = true;
        replaced = true;
    }

    if (!(special && replaced)) {

        // emit replaced (changed) instruction
        pos += buildReplacedInstruction(pos, inst, orig_code, replacementType, replacementRM, !only_movement);
        debug_size = buildReplacedInstruction(debug_code, inst, orig_code, replacementType, replacementRM, !only_movement);
        //printf("     buildReplacedInstruction; replacementRM=%s\n", FPContext::FPReg2Str(replacementRM).c_str());
        if (debug_size) {
            debug_assembly += FPDecoderXED::disassemble(debug_code, debug_size) + "\n";
        }
        replaced = true;
    }

    // save RAX (TODO: not always necessary)
    pos += mainGen->buildMovGPR64ToStack(pos, REG_EAX, getSavedEAXOffset());

    // save flags back to stack
    if (FPDecoderXED::writesFlags(inst)) {
        pos += mainGen->buildSaveFlagsFast(pos, getSavedFlagsOffset());
    }

    // POST-INSTRUCTION INSTRUMENTATION

    // fix up "replaced" flags in output
    //if (replaced && output->isRegisterSSE() && (packed || patch_output)) {
    if (replaced && !only_movement && output->isRegisterSSE() && 
            (output->getType() == IEEE_Double) && // used to be "packed || <type is double>"
            replacementType == SVT_IEEE_Single) {
        // TODO: handle GPR output operands?
        pos += buildFakeStackPushGPR64(pos, temp_gpr1);
        pos += buildFakeStackPushGPR64(pos, temp_gpr3);
        pos += mainGen->buildMovImm32ToGPR32(pos, IEEE32_FLAG, temp_gpr1);
        pos += buildInsertGPR32IntoXMM(pos, temp_gpr1, output->getRegister(), 1, temp_gpr3);
        if (packed) {
            pos += mainGen->buildMovImm32ToGPR32(pos, IEEE32_FLAG, temp_gpr1);
            pos += buildInsertGPR32IntoXMM(pos, temp_gpr1, output->getRegister(), 3, temp_gpr3);
        }
        pos += buildFakeStackPopGPR64(pos, temp_gpr3);
        pos += buildFakeStackPopGPR64(pos, temp_gpr1);
    } else if (replaced && !only_movement && output->isRegisterSSE() && output->getType() == SSE_Quad &&
               replacementType == SVT_IEEE_Single) {
        pos += buildFakeStackPushGPR64(pos, temp_gpr1);
        pos += buildFakeStackPushGPR64(pos, temp_gpr2);
        pos += buildFakeStackPushGPR64(pos, temp_gpr3);
        pos += buildFixSSEQuadOutput(pos, output->getRegister());
        pos += buildFakeStackPopGPR64(pos, temp_gpr3);
        pos += buildFakeStackPopGPR64(pos, temp_gpr2);
        pos += buildFakeStackPopGPR64(pos, temp_gpr1);
    }

    // restore any temporary XMM/GPR register
    if (replacementRM != REG_NONE) {
        // TODO: check to see if we actually saved it
        if (isGPR(replacementRM)) {
            pos += buildFakeStackPopGPR64(pos, replacementRM);
        } else {
            pos += buildFakeStackPopXMM(pos, replacementRM);
        }
    }

    return (size_t)(pos-old_pos);
}

bool FPBinaryBlobInplace::generate(Point * /*pt*/, Buffer &buf)
{
    // original instruction information
    size_t origNumBytes = inst->getNumBytes();
    unsigned char *orig_code, *pos;

    // replacement information
    FPSVType replacementType = mainPolicy->getSVType(inst);
    FPRegister replacementRM = REG_NONE;

    // flags
    bool special = false;           // exception case w/ special handling
    bool packed = false;            // instruction is packed SSE
    bool replaced = false;          // instruction has been fully replaced
    bool only_movement = true;      // instruction is only a data move
    bool mem_output = false;        // instruction has a memory output operand
    bool xmm_output = false;        // instruction has an SSE register output operand

    // temporary variables
    FPOperation *op;
    FPOperand *input = NULL, *output = NULL;
    size_t i, j, temp;

    // call the FPBinaryBlob initialization function
    initialize();

    // allocate space for blob code
    orig_code = (unsigned char*)malloc(origNumBytes);
    inst->getBytes(orig_code);

    // set up blob code markers
    setBlobAddress((void*)buf.curAddr());
    pos = getBlobCode();

    // set up some class-wide variables
    temp_gpr1 = getUnusedGPR();
    temp_gpr2 = getUnusedGPR();
    temp_gpr3 = getUnusedGPR();
    debug_assembly = string("");

    //printf("\n    building binary blob for %p: %s\n",
            //inst->getAddress(), inst->getDisassembly().c_str());
    //printf("%s\n", inst->toString().c_str());


    // for each operation
    for (i=0; i<inst->numOps; i++) {
        op = (*inst)[i];
        
        // is this a packed SSE instruction?
        packed = (op->numOpSets > 1);

        // check for pure data movement instructions (i.e., movsd, movapd)
        // NOTE: have to check for OP_ZERO as well as OP_MOV because some SSE
        // movement instructions zero out the higher-order bytes of an
        // SSE register
        if (!(op->type == OP_MOV || op->type == OP_ZERO)) {
            only_movement = false;
        }

        // check for "special" operations that should be replaced without further
        // analysis
        temp = buildSpecialOp(pos, op, packed, replaced);
        if (temp > 0) {
            special = true;
            pos += temp;
            continue;
        }

        // check for memory outputs
        for (j=0; j<op->opSets[0].nOut; j++) {
            output = op->opSets[0].out[j];
            if (output->isMemory()) {
                mem_output = true;
            }
            if (output->isRegisterSSE()) {
                xmm_output = true;
            }
        }

        // create input entries from input operands
        vector<FPInplaceBlobInputEntry> inputEntries;
        for (j=0; j<op->opSets[0].nIn; j++) {
            input = op->opSets[0].in[j];
            addBlobInputEntry(inputEntries, input, packed, replacementRM);
        }
        
        pos += buildHeader(pos);

        // handle simple single- or double-precision replacements
        if (replacementType == SVT_IEEE_Single ||
            replacementType == SVT_IEEE_Double) {

            pos += buildReplacedOperation(pos,
                    op, replacementType,
                    orig_code, origNumBytes,
                    inputEntries, replacementRM, 
                    packed, only_movement, mem_output, xmm_output,
                    special, replaced);

        } else {
            assert(!"Unhandled replacement type");
        }

        pos += buildFooter(pos);
    }

    // copy into PatchAPI buffer
    finalize();
    unsigned char *b = (unsigned char*)getBlobCode();
    for (b = (unsigned char*)getBlobCode(); b < pos; b++) {
        buf.push_back(*b);
    }

    // debut output
    /*
     *printf("    built binary blob at %p [size=%ld]: %-40s  tmp=%-4s  %12s  %12s\n", 
     *        getBlobAddress(), (b-getBlobCode()), inst->getDisassembly().c_str(),
     *        FPContext::FPReg2Str(replacementRM).c_str(),
     *        (packed ? "[packed]" : ""),
     *        (replaced ? "[replaced]" : ""));
     *printf("      original addr = %p\n", inst->getAddress());
     */

    return true;
}

bool FPAnalysisInplace::isSupportedOp(FPOperation *op)
{
    return op->type == OP_ADD ||
           op->type == OP_SUB ||
           op->type == OP_MUL ||
           op->type == OP_DIV ||
           op->type == OP_OR ||
           op->type == OP_AND ||
           op->type == OP_OR ||
           op->type == OP_XOR ||
           op->type == OP_SQRT ||
           op->type == OP_NEG ||
           op->type == OP_MIN ||
           op->type == OP_MAX ||
           op->type == OP_COMI ||
           op->type == OP_UCOMI ||
           op->type == OP_CMP ||
           op->type == OP_CVT ||
           op->type == OP_MOV;
}

bool FPAnalysisInplace::canBuildBinaryBlob(FPSemantics *inst)
{
    return ( (inst->hasOperandOfType(IEEE_Double) || inst->hasOperandOfType(SSE_Quad)) &&
        //!inst->hasOperandOfType(IEEE_Single) &&
        !inst->hasOperandOfType(C99_LongDouble) &&
        ((inst->numOps == 1 && isSupportedOp((*inst)[0])) ||
         (inst->numOps == 2 && (*inst)[0]->type == OP_ZERO && isSupportedOp((*inst)[1]))) );
}

Snippet::Ptr FPAnalysisInplace::buildPreInstrumentation(FPSemantics * /*inst*/,
        BPatch_addressSpace * /*app*/, bool & /*needsRegisters*/)
{
    return Snippet::Ptr();
}

Snippet::Ptr FPAnalysisInplace::buildPostInstrumentation(FPSemantics * /*inst*/,
        BPatch_addressSpace * /*app*/, bool & /*needsRegisters*/)
{
    return Snippet::Ptr();
}

Snippet::Ptr FPAnalysisInplace::buildReplacementCode(FPSemantics *inst,
        BPatch_addressSpace *app, bool &needsRegisters)
{
    if (mainPolicy->getSVType(inst) == SVT_IEEE_Single) {
        insnsInstrumentedSingle++;
    } else /* if (mainPolicy->getSVType(inst) == SVT_IEEE_Double)*/ {
        insnsInstrumentedDouble++;
    }
    if (canBuildBinaryBlob(inst)) {

        // add a setting for the instruction counter array
        // comment this if-statement out to disable instruction counting
        if (_INST_svinp_inst_count_ptr_sgl == NULL) {
            _INST_svinp_inst_count_ptr_sgl = (size_t**)app->malloc(sizeof(unsigned long*))->getBaseAddr();
            stringstream ss;    ss.clear();     ss.str("");
            ss << "svinp_icount_ptr_sgl=" << hex << _INST_svinp_inst_count_ptr_sgl << dec;
            configuration->addSetting(ss.str());
        }
        if (_INST_svinp_inst_count_ptr_dbl == NULL) {
            _INST_svinp_inst_count_ptr_dbl = (size_t**)app->malloc(sizeof(unsigned long*))->getBaseAddr();
            stringstream ss;    ss.clear();     ss.str("");
            ss << "svinp_icount_ptr_dbl=" << hex << _INST_svinp_inst_count_ptr_dbl << dec;
            configuration->addSetting(ss.str());
        }

        //printf("binary blob replacement: %s\n", inst->getDisassembly().c_str());
        FPBinaryBlobInplace *blob = new FPBinaryBlobInplace(inst, mainPolicy);
        if (useLockPrefix) {
            blob->enableLockPrefix();
        }
        return Snippet::Ptr(blob);

    } else {
        printf("        default replacement: %s\n", inst->getDisassembly().c_str());
        //printf("%s\n", inst->toString().c_str());
        //printf("default replacement: %s\n", inst->getDisassembly().c_str());
        needsRegisters = true;
        return Snippet::Ptr();
    }
}

void FPAnalysisInplace::expandInstCount(size_t newSize)
{
    size_t *newInstCountSingle;
    size_t *newInstCountDouble;
    size_t i = 0;
    newSize = (newSize > instCountSize*2) ? (newSize + 10) : (instCountSize*2 + 10);
    //printf("expand_inst_count - old size: %lu    new size: %lu\n", instCountSize, newSize);
    newInstCountSingle = (size_t*)malloc(newSize * sizeof(size_t));
    newInstCountDouble = (size_t*)malloc(newSize * sizeof(size_t));
    if (!newInstCountSingle || !newInstCountDouble) {
        fprintf(stderr, "OUT OF MEMORY!\n");
        exit(-1);
    }
    if (instCountSingle != NULL && instCountDouble != NULL) {
        for (; i < instCountSize; i++) {
            newInstCountSingle[i] = instCountSingle[i];
            newInstCountDouble[i] = instCountDouble[i];
        }
        free(instCountSingle);
        free(instCountDouble);
        instCountSingle = NULL;
        instCountDouble = NULL;
    }
    for (; i < newSize; i++) {
        newInstCountSingle[i] = 0;
        newInstCountDouble[i] = 0;
    }
    instCountSingle = newInstCountSingle;
    instCountDouble = newInstCountDouble;
    instCountSize = newSize;
    if (_INST_svinp_inst_count_ptr_sgl != NULL) {
        *_INST_svinp_inst_count_ptr_sgl = instCountSingle;
    }
    if (_INST_svinp_inst_count_ptr_dbl != NULL) {
        *_INST_svinp_inst_count_ptr_dbl = instCountDouble;
    }
}

void FPAnalysisInplace::handleConvert(FPOperand *output, FPOperand *input)
{
    float fval;
    double dval;
    bool singlePrec = false;

    // initialize input and output
    input->refresh(context);
    if (output) {
        output->refreshAddress(context);
    }

    // are we working in single precision (or double precision?)
    singlePrec = input->type == IEEE_Single 
              || isReplaced(input->currentValue.data.dbl);

    // save result to context
    if (singlePrec) {
        fval = extractFloat(input);
        saveFloatToContext(output, fval);
    } else {
        dval = extractDouble(input);
        saveDoubleToContext(output, dval);
    }
}

void FPAnalysisInplace::handleZero(FPOperand *output)
{
    saveDoubleToContext(output, 0.0);
}

void FPAnalysisInplace::handleUnaryOp(FPOperationType type, FPOperand *output, FPOperand *input)
{
    float fval = 0.0f, fans = 0.0f;
    double dval = 0.0, dans = 0.0;
    bool singlePrec = false;

    // initialize input and output
    input->refresh(context);
    if (output) {
        output->refreshAddress(context);
    }

    // are we working in single precision (or double precision?)
    singlePrec = input->type == IEEE_Single 
              || isReplaced(input->currentValue.data.dbl);

    // grab the actual values to use
    if (singlePrec) {
        fval = extractFloat(input);
    } else {
        dval = extractDouble(input);
    }

    // do operation
    switch (type) {
        case OP_NEG:
            if (singlePrec) { fans = -fval; } else { dans = -dval; } break;
        case OP_ABS:
            if (singlePrec) { fans = fabsf(fval); } else { dans = fabs(dval); } break;
        case OP_RCP:
            if (singlePrec) { fans = 1.0f / fval; } else { dans = 1.0 / dval; } break;
        case OP_SQRT:
            if (singlePrec) { fans = sqrtf(fval); } else { dans = sqrt(dval); } break;
        case OP_NOT:
            if (singlePrec) { *(uint32_t*)&fans = ~(*(uint32_t*)&fval); }
                       else { *(uint64_t*)&dans = ~(*(uint64_t*)&dval); } break;

        default:        // all other operations
            assert(!"Unhandled operation");
    }

    // save result to context
    if (output) {
        if (singlePrec) {
            saveFloatToContext(output, fans);
        } else {
            saveDoubleToContext(output, dans);
        }
    }
}


void FPAnalysisInplace::handleBinaryOp(FPOperationType type, FPOperand *output, FPOperand *input1, FPOperand *input2)
{
    float fval1 = 0.0f, fval2 = 0.0f, fans = 0.0f;
    double dval1 = 0.0, dval2 = 0.0, dans = 0.0;
    bool singlePrec = false;

    // initialize inputs and output
    input1->refresh(context);
    input2->refresh(context);
    if (output) {
        output->refreshAddress(context);
    }

    // are we working in single precision (or double precision?)
    singlePrec = input1->type == IEEE_Single 
              || input2->type == IEEE_Single
              || isReplaced(input1->currentValue.data.dbl)
              || isReplaced(input2->currentValue.data.dbl);


    // grab the actual values to use
    if (singlePrec) {
        fval1 = extractFloat(input1);
        fval2 = extractFloat(input2);
        //printf("binop (single): type=%s op1=%g op2=%g\n",
                //FPOperation::FPOpType2Str(type).c_str(), fval1, fval2);
    } else {
        dval1 = extractDouble(input1);
        dval2 = extractDouble(input2);
        //printf("binop (double): type=%s op1=%g op2=%g\n",
                //FPOperation::FPOpType2Str(type).c_str(), dval1, dval2);
    }

    // do operation
    switch (type) {
        case OP_ADD:
            if (singlePrec) { fans = fval1 + fval2; } else { dans = dval1 + dval2; } break;
        case OP_SUB:
            if (singlePrec) { fans = fval1 - fval2; } else { dans = dval1 - dval2; } break;
        case OP_MUL:
            if (singlePrec) { fans = fval1 * fval2; } else { dans = dval1 * dval2; } break;
        case OP_DIV:
            if (singlePrec) { fans = fval1 / fval2; } else { dans = dval1 / dval2; } break;
        case OP_AND:
            if (singlePrec) { *(uint32_t*)&fans = *(uint32_t*)&fval1 & *(uint32_t*)&fval2; }
                       else { *(uint64_t*)&dans = *(uint64_t*)&dval1 & *(uint64_t*)&dval2; } break;
        case OP_OR:
            if (singlePrec) { *(uint32_t*)&fans = *(uint32_t*)&fval1 | *(uint32_t*)&fval2; }
                       else { *(uint64_t*)&dans = *(uint64_t*)&dval1 | *(uint64_t*)&dval2; } break;
        case OP_XOR:
            if (singlePrec) { *(uint32_t*)&fans = *(uint32_t*)&fval1 ^ *(uint32_t*)&fval2; }
                       else { *(uint64_t*)&dans = *(uint64_t*)&dval1 ^ *(uint64_t*)&dval2; } break;
        case OP_COMI:
            if (singlePrec) { handleComI(fval1, fval2, true); } 
                       else { handleComI(dval1, dval2, true); } break;
        case OP_UCOMI:
            if (singlePrec) { handleComI(fval1, fval2, false); } 
                       else { handleComI(dval1, dval2, false); } break;
        //case OP_CMP:
            //if (singlePrec) { fans = handleCmp(fval1, fval2); }
                       //else { dans = handleCmp(dval1, dval2); } break;

        case OP_MIN:
            if (singlePrec) { fans = (fval1 < fval2 ? fval1 : fval2); }
                       else { dans = (dval1 < dval2 ? dval1 : dval2); } break;
        case OP_MAX:
            if (singlePrec) { fans = (fval1 > fval2 ? fval1 : fval2); }
                       else { dans = (dval1 > dval2 ? dval1 : dval2); } break;
        case OP_COPYSIGN:
            if (singlePrec) { fans = fabsf(fval1); if (fval2 < 0.0) fans = -fans; }
                       else { dans =  fabs(dval1); if (dval2 < 0.0) dans = -dans; } break;

        default:        // all other operations
            assert(!"Unhandled operation");
    }

    // save result to context
    if (output) {
        if (singlePrec) {
            saveFloatToContext(output, fans);
        } else {
            saveDoubleToContext(output, dans);
        }
    }
}


#if 0

void FPAnalysisInplace::handleAm(FPSV *input)
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
    if (FPAnalysisInplace::debugPrint) {
        cout << "FPAnalysisInplace examine " << input->toDetailedString() << ": ";
        cout << (norm ? "[norm] " : "");
        cout << (nan ? "[nan] " : ""); cout << (inf ? "[inf] " : "");
        cout << (neg ? "[neg] " : ""); cout << (zero ? "[zero] " : "");
        cout << endl;
    }
#endif

    context->fxsave_state->fsw &= (unsigned short)(~(FPContext::FP_X87_CLEAR_FLAGS));    // clear C0-C3
    context->fxsave_state->fsw |= (unsigned short)fsw;                // set C0, C1, C2, and C3 flags
}

void FPAnalysisInplace::handleCom(FPSV *input1, FPSV *input2, bool ordered)
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
    if (FPAnalysisInplace::debugPrint) {
        cout << "FPAnalysisInplace " << (ordered ? "ordered " : "unordered ");
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
#endif

void FPAnalysisInplace::handleComI(float input1, float input2, bool ordered)
{
    unsigned long flags;
    __asm__ ("movss %0, %%xmm0" : : "m" (input1) : "xmm0");
    __asm__ ("movss %0, %%xmm1" : : "m" (input2) : "xmm1");
    if (ordered) {
        __asm__ ("comiss %%xmm1, %%xmm0" : );
    } else {
        __asm__ ("ucomiss %%xmm1, %%xmm0" : );
    }
    __asm__ ("pushf" : );
    __asm__ ("pop %0" : "=m" (flags));
#if INCLUDE_DEBUG
    debugPrintCompare((double)input1, (double)input2, ordered, flags);
#endif
    context->reg_eflags &= ~(FPContext::FP_CLEAR_FLAGS);    // clear OF, SF, and AF flags
    context->reg_eflags |= flags;                // set ZF, PF, and CF flags
}

void FPAnalysisInplace::handleComI(double input1, double input2, bool ordered)
{
    unsigned long flags;
    __asm__ ("movsd %0, %%xmm0" : : "m" (input1) : "xmm0");
    __asm__ ("movsd %0, %%xmm1" : : "m" (input2) : "xmm1");
    if (ordered) {
        __asm__ ("comisd %%xmm1, %%xmm0" : );
    } else {
        __asm__ ("ucomisd %%xmm1, %%xmm0" : );
    }
    __asm__ ("pushf" : );
    __asm__ ("pop %0" : "=m" (flags));
#if INCLUDE_DEBUG
    debugPrintCompare(input1, input2, ordered, flags);
#endif
    context->reg_eflags &= ~(FPContext::FP_CLEAR_FLAGS);    // clear OF, SF, and AF flags
    context->reg_eflags |= flags;                // set ZF, PF, and CF flags
}

/*
unsigned long FPAnalysisInplace::handleCmp(float input1, float input2)
{
    uint8_t flag = currFlag->data.uint8;
    unsigned long result;
    __asm__ ("movss %0, %%xmm0" : : "m" (input1) : "xmm0");
    __asm__ ("movss %0, %%xmm1" : : "m" (input2) : "xmm1");
    __asm__ ("cmpss %%xmm1, %%xmm0, %0" : : "m" (flag) : "xmm0");
    __asm__ ("movss %%xmm0, %0" : "=m" (result));
    return result;
}

unsigned long FPAnalysisInplace::handleCmp(double input1, double input2)
{
    unsigned long flags;
    __asm__ ("movsd %0, %%xmm0" : : "m" (input1) : "xmm0");
    __asm__ ("movsd %0, %%xmm1" : : "m" (input2) : "xmm1");
    __asm__ ("cmpsd %%xmm1, %%xmm0" : );
    __asm__ ("pushf" : );
    __asm__ ("pop %0" : "=m" (flags));
    return flags;
}
*/


/*
float FPAnalysisInplace::handleUnaryReplacedFunc(FPOperationType type, float input)
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
*/

#define HANDLE_UNARY_CASE_DBL(TYPE,S_FUNC,D_FUNC) \
    case TYPE: \
        if (singlePrec) { fans = S_FUNC(fval); } \
                   else { dans = D_FUNC(input); } break;

double FPAnalysisInplace::handleUnaryReplacedFunc(FPOperationType type, double input)
{
    float fval = 0.0f, fans = 0.0f;
    double dans = 0.0;
    bool singlePrec = false;

    // are we working in single precision (or double precision?)
    singlePrec = isReplaced(input);

    // grab the actual values to use
    if (singlePrec) {
        fval = extractFloat(input);
        //printf("unop (single): type=%s op=%g\n",
                //FPOperation::FPOpType2Str(type).c_str(), fval);
    } else {
        //printf("unop (double): type=%s op=%g\n",
                //FPOperation::FPOpType2Str(type).c_str(), input);
    }

    // do operation
    switch (type) {
        HANDLE_UNARY_CASE_DBL(OP_ABS,     fabsf,    fabs);
        HANDLE_UNARY_CASE_DBL(OP_CEIL,    ceilf,    ceil);
        HANDLE_UNARY_CASE_DBL(OP_ERF,     erff,     erf);
        HANDLE_UNARY_CASE_DBL(OP_ERFC,    erfcf,    erfc);
        HANDLE_UNARY_CASE_DBL(OP_EXP,     expf,     exp);
        HANDLE_UNARY_CASE_DBL(OP_EXP2,    exp2f,    exp2);
        HANDLE_UNARY_CASE_DBL(OP_FLOOR,   floorf,   floor);
        HANDLE_UNARY_CASE_DBL(OP_LOG,     logf,     log);
        HANDLE_UNARY_CASE_DBL(OP_LOGB,    logbf,    logb);
        HANDLE_UNARY_CASE_DBL(OP_LOG10,   log10f,   log10);
        HANDLE_UNARY_CASE_DBL(OP_SQRT,    sqrtf,    sqrt);
        HANDLE_UNARY_CASE_DBL(OP_CBRT,    cbrtf,    cbrt);
        HANDLE_UNARY_CASE_DBL(OP_TRUNC,   truncf,   trunc);

        HANDLE_UNARY_CASE_DBL(OP_SIN,   sinf,   sin);
        HANDLE_UNARY_CASE_DBL(OP_COS,   cosf,   cos);
        HANDLE_UNARY_CASE_DBL(OP_TAN,   tanf,   tan);
        HANDLE_UNARY_CASE_DBL(OP_ASIN,  asinf,  asin);
        HANDLE_UNARY_CASE_DBL(OP_ACOS,  acosf,  acos);
        HANDLE_UNARY_CASE_DBL(OP_ATAN,  atanf,  atan);
        HANDLE_UNARY_CASE_DBL(OP_SINH,  sinhf,  sinh);
        HANDLE_UNARY_CASE_DBL(OP_COSH,  coshf,  cosh);
        HANDLE_UNARY_CASE_DBL(OP_TANH,  tanhf,  tanh);
        HANDLE_UNARY_CASE_DBL(OP_ASINH, asinhf, asinh);
        HANDLE_UNARY_CASE_DBL(OP_ACOSH, acoshf, acosh);
        HANDLE_UNARY_CASE_DBL(OP_ATANH, atanhf, atanh);

        case OP_CLASS:
            if (singlePrec) { return fpclassify(fval); } else { return fpclassify(input); } break;
        case OP_ISFIN:
            if (singlePrec) { return finitef(fval); }    else { return finite(input); } break;
        case OP_ISNORM:
            if (singlePrec) { return isnormal(fval); }   else { return isnormal(input); } break;
        case OP_ISNAN:
            if (singlePrec) { return isnanf(fval); }     else { return isnan(input); } break;
        case OP_ISINF:
            if (singlePrec) { return isinff(fval); }     else { return isinf(input); } break;
        case OP_NEG:
            if (singlePrec) { return -fval; }            else { return -input; } break;

        default:        // all other operations
            printf("Unhandled operation: %s\n", FPOperation::FPOpType2Str(type).c_str());
            assert(!"Unhandled operation");
    }

    // add "replaced" flag if necessary
    if (singlePrec) {
        dans = replaceDouble(fans);
    }

    return dans;
}

/*
long double FPAnalysisInplace::handleUnaryReplacedFunc(FPOperationType type, long double input)
{
    float fval, fans;
    long double dans;
    bool singlePrec = false;

    // are we working in single precision (or double precision?)
    singlePrec = isReplaced(input);

    // grab the actual values to use
    if (singlePrec) {
        fval = extractFloat(input);
    }

    // do operation
    switch (type) {
        HANDLE_UNARY_CASE_DBL(OP_SIN, sinf, sinl);
        HANDLE_UNARY_CASE_DBL(OP_COS, cosf, cosl);
        HANDLE_UNARY_CASE_DBL(OP_TAN, tanf, tanl);
        HANDLE_UNARY_CASE_DBL(OP_LOG, logf, logl);
        HANDLE_UNARY_CASE_DBL(OP_EXP, expf, expl);

        default:        // all other operations
            assert("Unhandled operation");
    }

    // add "replaced" flag if necessary
    if (singlePrec) {
        dans = 0.0L;
        *(uint64_t*)(&dans) = FPBinaryBlobInplace::IEEE64_FLAG | (uint64_t)(*(uint32_t*)(&fans));
    }

    return dans;
}
*/

/*
float FPAnalysisInplace::handleBinaryReplacedFunc(FPOperationType type, float input1, float input2)
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
*/

#define HANDLE_BINARY_CASE_DBL(TYPE,S_FUNC,D_FUNC) \
    case TYPE: \
        if (singlePrec) { fans = S_FUNC(fval1, fval2); } \
                   else { dans = D_FUNC(input1, input2); } break;

double FPAnalysisInplace::handleBinaryReplacedFunc(FPOperationType type, double input1, double input2)
{
    float fval1 = 0.0f, fval2 = 0.0f, fans = 0.0f;
    double dans = 0.0;
    bool singlePrec = false;

    // are we working in single precision (or double precision?)
    singlePrec = isReplaced(input1) || isReplaced(input2);

    // grab the actual values to use
    if (singlePrec) {
        fval1 = extractFloat(input1);
        fval2 = extractFloat(input2);
        //printf("binop (single): type=%s op1=%g op2=%g\n",
                //FPOperation::FPOpType2Str(type).c_str(), fval1, fval2);
    } else {
        //printf("binop (double): type=%s op1=%g op2=%g\n",
                //FPOperation::FPOpType2Str(type).c_str(), input1, input2);
    }

    // do operation
    switch (type) {
        HANDLE_BINARY_CASE_DBL(OP_ATAN2, atan2f, atan2);
        HANDLE_BINARY_CASE_DBL(OP_FMOD,  fmodf,  fmod);
        HANDLE_BINARY_CASE_DBL(OP_POW,   powf,   pow);
        HANDLE_BINARY_CASE_DBL(OP_COPYSIGN, copysignf, copysign);

        default:        // all other operations
            printf("Unhandled operation: %s\n", FPOperation::FPOpType2Str(type).c_str());
            assert(!"Unhandled operation");
    }

    // add "replaced" flag if necessary
    if (singlePrec) {
        dans = replaceDouble(fans);
    }

    return dans;
}

/*
long double FPAnalysisInplace::handleBinaryReplacedFunc(FPOperationType type, long double input1, long double input2)
{
    float fval1, fval2, fans;
    long double dans;
    bool singlePrec = false;

    // are we working in single precision (or double precision?)
    singlePrec = isReplaced(input1) || isReplaced(input2);

    // grab the actual values to use
    if (singlePrec) {
        fval1 = extractFloat(input1);
        fval2 = extractFloat(input2);
        //printf("binop (single): type=%s op1=%g op2=%g\n",
                //FPOperation::FPOpType2Str(type).c_str(), fval1, fval2);
    }

    // do operation
    switch (type) {
        HANDLE_BINARY_CASE_DBL(OP_FMOD, fmodf, fmodl);

        default:        // all other operations
            assert("Unhandled operation");
    }

    // add "replaced" flag if necessary
    if (singlePrec) {
        dans = 0.0L;
        *(uint64_t*)(&dans) = FPBinaryBlobInplace::IEEE64_FLAG | (uint64_t)(*(uint32_t*)(&fans));
    }

    return dans;
}
*/

// example of custom handler

void FPAnalysisInplace::handleReplacedFuncSinCos(float input, float *sin_output, float *cos_output)
{ sincosf(input, sin_output, cos_output); }

void FPAnalysisInplace::handleReplacedFuncSinCos(double input, double *sin_output, double *cos_output)
{
    float fval, fsin, fcos;
    if (isReplaced(input)) {
        fval = extractFloat(input);
        sincosf(fval, &fsin, &fcos);
        *sin_output = replaceDouble(fsin);
        *cos_output = replaceDouble(fcos);
    } else {
        sincos(input, sin_output, cos_output);
    }
}

void FPAnalysisInplace::handleReplacedFuncSinCos(long double /*input*/, long double * /*sin_output*/, long double * /*cos_output*/)
{ assert(!"unsupported sincosl call"); }

float FPAnalysisInplace::handleReplacedFuncModF(float input, float *int_output)
{ return modff(input, int_output); }

double FPAnalysisInplace::handleReplacedFuncModF(double input, double *int_output)
{
    float fval, ffrac, fint;
    if (isReplaced(input)) {
        fval = extractFloat(input);
        ffrac = modff(fval, &fint);
        *int_output = replaceDouble(fint);
        return replaceDouble(ffrac);
    } else {
        return modf(input, int_output);
    }
}

long double FPAnalysisInplace::handleReplacedFuncModF(long double /*input*/, long double * /*int_output*/)
{ assert(!"unsupported modfl call"); return 0.0L; }

float FPAnalysisInplace::handleReplacedFuncFrExp(float input, int *exp_output)
{ return frexpf(input, exp_output); }

double FPAnalysisInplace::handleReplacedFuncFrExp(double input, int *exp_output)
{
    float fval, ffrac;
    if (isReplaced(input)) {
        fval = extractFloat(input);
        ffrac = frexpf(fval, exp_output);
        return replaceDouble(ffrac);
    } else {
        return frexp(input, exp_output);
    }
}

long double FPAnalysisInplace::handleReplacedFuncFrExp(long double /*input*/, int * /*exp_output*/)
{ assert(!"unsupported frexpl call"); return 0.0L; }

float FPAnalysisInplace::handleReplacedFuncLdExp(float sig_input, int exp_input)
{ return ldexpf(sig_input, exp_input); }

double FPAnalysisInplace::handleReplacedFuncLdExp(double sig_input, int exp_input)
{
    if (isReplaced(sig_input)) {
        return replaceDouble(ldexpf(extractFloat(sig_input), exp_input));
    } else {
        return ldexp(sig_input, exp_input);
    }
}

long double FPAnalysisInplace::handleReplacedFuncLdExp(long double /*sig_input*/, int /*exp_input*/)
{ assert(!"unsupported ldexpl call"); return 0.0L; }

inline
double FPAnalysisInplace::replaceDouble(float value)
{
    double ret = 0.0;
    *(uint64_t*)(&ret) = FPBinaryBlobInplace::IEEE64_FLAG | 
        (((uint64_t)(*(uint32_t*)(&value))) & FPBinaryBlobInplace::IEEE64_VALUE_MASK);
    return ret;
}

inline
double FPAnalysisInplace::replaceDouble(double value)
{
    return replaceDouble((float)value);
}

bool FPAnalysisInplace::isReplaced(double value) {
    return (((*(uint64_t*)(&value)) & FPBinaryBlobInplace::IEEE64_FLAG_MASK) == FPBinaryBlobInplace::IEEE64_FLAG);
}

bool FPAnalysisInplace::isReplaced(void* value) {
    /*
     *printf("isReplaced(%p) = %s\n", value,
     *    (((*(uint64_t*)(&value)) & FPBinaryBlobInplace::IEEE64_FLAG_MASK) == FPBinaryBlobInplace::IEEE64_FLAG) ? "yes" : "no");
     */
    return (((*(uint64_t*)(&value)) & FPBinaryBlobInplace::IEEE64_FLAG_MASK) == FPBinaryBlobInplace::IEEE64_FLAG);
}

float FPAnalysisInplace::extractFloat(double input)
{
    float val;
    if (isReplaced(input)) {
        val = *(float*)(&input);    // use low bits
    } else {
        if (input == 0x8000000000000000) {
            *(uint32_t*)&val = 0x80000000;
        } else if (input == 0x7fffffffffffffff) {
            *(uint32_t*)&val = 0x7fffffff;
        } else {
            val = (float)input;         // down-cast
        }
    }
    return val;
}

inline
float FPAnalysisInplace::extractFloat(long double input)
{
    float val;
    double tmp;
    *(uint64_t*)(&tmp) = *(uint64_t*)(&input);
    if (isReplaced(tmp)) {
        val = *(float*)(&input);    // use low bits
    } else {
        val = (float)input;         // down-cast
    }
    return val;
}

inline
float FPAnalysisInplace::extractFloat(FPOperand *op)
{
    float val;
    switch (op->type) {
        case IEEE_Single:
            val = op->currentValue.data.flt; break;
        case IEEE_Double:
            if (isReplaced(op->currentValue.data.dbl)) {
                val = *(float*)(&op->currentValue.data.dbl);    // use low bits
            } else {
                if (op->currentValue.data.uint64 == 0x8000000000000000) {
                    *(uint32_t*)&val = 0x80000000;
                } else if (op->currentValue.data.uint64 == 0x7fffffffffffffff) {
                    *(uint32_t*)&val = 0x7fffffff;
                } else {
                    val = (float)op->currentValue.data.dbl;         // down-cast
                }
            }
            break;
        case C99_LongDouble:
            if (isReplaced(op->currentValue.data.dbl)) {
                val = *(float*)(&op->currentValue.data.dbl);    // use low bits
            } else {
                val = (float)op->currentValue.data.ldbl;        // down-cast
            }
            break;
        default:
            assert(!"Operand is not of valid type.");
    }
    return val;
}

inline
double FPAnalysisInplace::extractDouble(FPOperand *op)
{
    double val;
    switch (op->type) {
        case IEEE_Single:
            if (op->currentValue.data.uint32 == 0x80000000) {
                *(uint64_t*)&val = 0x8000000000000000;
            } else if (op->currentValue.data.uint32 == 0x7fffffff) {
                *(uint64_t*)&val = 0x7fffffffffffffff;
            } else {
                val = (double)op->currentValue.data.dbl;        // up-cast
            }
        case IEEE_Double:
            val = op->currentValue.data.dbl; break;
        case C99_LongDouble:
            val = (double)op->currentValue.data.ldbl; break;    // down-cast
        default:
            assert(!"Operand is not of valid type.");
    }
    //printf("extracted double (%g) from %s\n", val, op->toStringV().c_str());
    return val;
}

inline
void FPAnalysisInplace::saveFloatToContext(FPOperand *output, float value)
{
    switch (output->type) {
        case IEEE_Single:
            output->setCurrentValueF(value, context, true, true); break;
        case IEEE_Double:
            if (mainPolicy->shouldReplaceWithPtr(output, currInst)) {
                saveReplacedDoubleToContext(output, value);
            } else {
                output->setCurrentValueD((double)value, context, true, true);
            }
            break;
        case C99_LongDouble:
            if (mainPolicy->shouldReplaceWithPtr(output, currInst)) {
                // TODO: need to tack on extra bits for long doubles?
                saveReplacedDoubleToContext(output, value);
            } else {
                output->setCurrentValueLD((long double)value, context, true, true);
            }
            break;
        default:
            assert(!"Unhandled operand type."); break;
    }
}

inline
void FPAnalysisInplace::saveDoubleToContext(FPOperand *output, double value)
{
    switch (output->type) {
        case IEEE_Single:
            output->setCurrentValueF((float)value, context, true, true); break;
        case IEEE_Double:
            if (mainPolicy->shouldReplaceWithPtr(output, currInst)) {
                saveReplacedDoubleToContext(output, *(float*)(&value));
            } else {
                output->setCurrentValueD((double)value, context, true, true);
            }
            break;
        case C99_LongDouble:
            if (mainPolicy->shouldReplaceWithPtr(output, currInst)) {
                // TODO: need to tack on extra bits for long doubles?
                saveReplacedDoubleToContext(output, *(float*)(&value));
            } else {
                output->setCurrentValueLD((long double)value, context, true, true);
            }
            break;
        default:
            assert(!"Unhandled operand type."); break;
    }
}

inline
void FPAnalysisInplace::saveReplacedDoubleToContext(FPOperand *output, float value)
{
    double buffer = replaceDouble(value);
    output->setCurrentValueUInt64(*(uint64_t*)(&buffer), context, true, true);
}

// print shadow entries by location; tries to use debug info to provide better output
void FPAnalysisInplace::printShadowLocation(FPOperandAddress addr)
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
void FPAnalysisInplace::printShadowMatrix(string baseName, FPOperandAddress baseAddr, 
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
void FPAnalysisInplace::printShadowEntry(string name, FPOperandAddress addr, unsigned long size)
{
    //cout << "  printing shadow value entry: name=" << name << " addr=" << addr << " size=" << size << endl;

    stringstream logDetails;
    char tempBuffer[2048];

    // retrieve and check for valid shadow table entry
    // TODO: do this properly with "size"
    double dval = *(double*)addr;
    uint64_t raw = *(uint64_t*)(&dval);
    float fval = *(float*)(&dval);

#if INCLUDE_DEBUG
    if (debugPrint) {
        if (!isReplaced(dval)) {
            cout << "  NO shadow value ";
        } else {
            cout << "  found shadow value ";
        }
        cout << " for variable \"" << name << "\" ";
        cout << "  addr=" << addr;
        cout << "  size=" << size;
        cout << "  val=" << hex << raw << dec;
        cout << "  fval=" << fval;
        cout << endl;
        cout.flush();
    }
#endif
    if (!isReplaced(dval)) {
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

        // output the shadow value
        sprintf(tempBuffer, "%.2e", fval);
        logDetails << "label=" << tempBuffer << endl;
        
        logDetails << "value=" << hex << raw << dec << endl;

        logDetails << "system value=0.0" << endl;
        logDetails << "abs error=0.0" << endl;
        logDetails << "rel error=0.0" << endl;
        logDetails << "address=0x" << hex << (unsigned long)addr << dec << endl;
        logDetails << "size=" << size << endl;
        logDetails << "type=Inplace Single-Precision" << endl;

#if INCLUDE_DEBUG
        if (debugPrint) {
            //cout << logDetails.str() << endl;
        }
#endif
        logFile->addMessage(SHVALUE, 0, name, logDetails.str(), "", NULL);
    }


    // don't output this entry again
    //doneAlready.push_back(addr);
}


void FPAnalysisInplace::finalOutput()
{
    vector<FPShadowEntry*>::iterator k;
    FPShadowEntry* entry;
    FPOperandAddress addr, maddr;
    size_t i, j, n, r, c, size, icount;
    size_t exec_single = 0;
    size_t exec_double = 0;
    size_t exec_total = 0;
    stringstream outputString;

    // instruction counts
    FPSemantics *inst;
    stringstream ss2;
    if (_INST_svinp_inst_count_ptr_sgl != NULL &&
        _INST_svinp_inst_count_ptr_dbl != NULL) {
        for (i=0; i<instCountSize; i++) {
            inst = decoder->lookup(i);
            if (inst != NULL) {

                // overall count
                icount = instCountSingle[i] + instCountDouble[i];

                // output individual count
                ss2.clear();
                ss2.str("");
                ss2 << "instruction #" << i << ": count=" << icount;
                if (mainPolicy->getSVType(inst) == SVT_IEEE_Single) {
                    ss2 << " [single]";
                } else if (mainPolicy->getSVType(inst) == SVT_IEEE_Double) {
                    ss2 << " [double]";
                } else {
                    ss2 << " [none]";
                }
                logFile->addMessage(ICOUNT, icount, inst->getDisassembly(), ss2.str(),
                        "", inst);

                // add to aggregate counts
                exec_single += instCountSingle[i];
                exec_double += instCountDouble[i];
                exec_total += icount;
            }
        }
    }
    stringstream ss;
    ss << "Finished execution:" << endl;
    ss << "  Inplace: " << logFile->formatLargeCount(exec_total) << " executed";
    ss << setiosflags(ios::fixed) << setprecision(3);
    ss << "  (" << logFile->formatLargeCount(exec_single)
       << " [" << (exec_single == 0 ? 0 : 
               (double)exec_single/(double)exec_total*100.0) << "%]"
       << " single";
    ss << ", " << logFile->formatLargeCount(exec_double)
       << " [" << (exec_double == 0 ? 0 :
               (double)exec_double/(double)exec_total*100.0) << "%]"
       << " double)";
    logFile->addMessage(SUMMARY, 0, getDescription(), ss.str(), "");
    cerr << ss.str() << endl;

    // shadow table output initialization
    outputString.clear();
    outputString.str("");
    doneAlready.clear();
    
    // summary output
    //cout << "finalizing pointer analysis" << endl;
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

    string label("Inplace-Replacement Shadow Values");
    string detail = outputString.str();

    logFile->addMessage(SUMMARY, 0, label, detail, "");
    //cout << outputString.str();

    //cout << "done with final output for pointer analysis" << endl;
}

}

