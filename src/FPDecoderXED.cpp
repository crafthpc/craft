#include "FPDecoderXED.h"

namespace FPInst {

xed_state_t dstate;


FPDecoderXED::FPDecoderXED()
{
    // initialize the XED library
    xed_tables_init();
    xed_state_zero(&dstate);
#ifdef __X86_64__
    dstate.mmode=XED_MACHINE_MODE_LONG_64;
    //xed_state_init(&dstate, XED_MACHINE_MODE_LONG_64,
        //XED_ADDRESS_WIDTH_64b, XED_ADDRESS_WIDTH_64b);
#else
    xed_state_init(&dstate, XED_MACHINE_MODE_LEGACY_32,
        XED_ADDRESS_WIDTH_32b, XED_ADDRESS_WIDTH_32b);
#endif

    // initialize decoded instruction cache
    instCacheSize = 0;
    instCacheArray = NULL;
    expandInstCache(2000);
}

FPDecoderXED::~FPDecoderXED()
{
    unsigned i;
    for (i = 0; i < instCacheSize; i++) {
        if (instCacheArray[i])
            delete instCacheArray[i];
    }
    delete instCacheArray;
}

string FPDecoderXED::Bytes2Str(unsigned char *bytes, size_t nbytes)
{
    char *buffer = (char*)malloc((nbytes*3+1)*sizeof(char));
    if (!buffer) {
        return string("");
    }
    unsigned char *ptr;
    unsigned i;
    ptr = (unsigned char*)bytes;
    char *tmp = buffer;
    for (i=0; i<nbytes; i++) {
        tmp += sprintf(tmp, "%s%02x", (i>0 ? " " : ""), *ptr);
        ptr++;
    }
    *tmp = '\0';
    return string(buffer);
}

void FPDecoderXED::printInstBytes(FILE *file, unsigned char *bytes, size_t nbytes)
{
    unsigned char *ptr;
    unsigned i;
    ptr = (unsigned char*)bytes;
    for (i=0; i<nbytes; i++) {
        fprintf(file, "%s%02x", (i>0 ? " " : " "), *ptr);
        ptr++;
    }
}

FPRegister FPDecoderXED::xedReg2FPReg(xed_reg_enum_t reg)
{
    switch (reg) {
        // {{{ XED -> FPRegister conversions
        case XED_REG_ST0: return REG_ST0;
        case XED_REG_ST1: return REG_ST1;
        case XED_REG_ST2: return REG_ST2;
        case XED_REG_ST3: return REG_ST3;
        case XED_REG_ST4: return REG_ST4;
        case XED_REG_ST5: return REG_ST5;
        case XED_REG_ST6: return REG_ST6;
        case XED_REG_ST7: return REG_ST7;
        case XED_REG_XMM0:  return REG_XMM0;
        case XED_REG_XMM1:  return REG_XMM1;
        case XED_REG_XMM2:  return REG_XMM2;
        case XED_REG_XMM3:  return REG_XMM3;
        case XED_REG_XMM4:  return REG_XMM4;
        case XED_REG_XMM5:  return REG_XMM5;
        case XED_REG_XMM6:  return REG_XMM6;
        case XED_REG_XMM7:  return REG_XMM7;
        case XED_REG_XMM8:  return REG_XMM8;
        case XED_REG_XMM9:  return REG_XMM9;
        case XED_REG_XMM10: return REG_XMM10;
        case XED_REG_XMM11: return REG_XMM11;
        case XED_REG_XMM12: return REG_XMM12;
        case XED_REG_XMM13: return REG_XMM13;
        case XED_REG_XMM14: return REG_XMM14;
        case XED_REG_XMM15: return REG_XMM15;
        case XED_REG_EIP:  case XED_REG_RIP: return REG_EIP;
        case XED_REG_EAX:  case XED_REG_RAX: return REG_EAX;
        case XED_REG_EBX:  case XED_REG_RBX: return REG_EBX;
        case XED_REG_ECX:  case XED_REG_RCX: return REG_ECX;
        case XED_REG_EDX:  case XED_REG_RDX: return REG_EDX;
        case XED_REG_ESP:  case XED_REG_RSP: return REG_ESP;
        case XED_REG_EBP:  case XED_REG_RBP: return REG_EBP;
        case XED_REG_ESI:  case XED_REG_RSI: return REG_ESI;
        case XED_REG_EDI:  case XED_REG_RDI: return REG_EDI;
        case XED_REG_R8:  case XED_REG_R8D:  case XED_REG_R8W:  case XED_REG_R8B:  return REG_E8;
        case XED_REG_R9:  case XED_REG_R9D:  case XED_REG_R9W:  case XED_REG_R9B:  return REG_E9;
        case XED_REG_R10: case XED_REG_R10D: case XED_REG_R10W: case XED_REG_R10B: return REG_E10;
        case XED_REG_R11: case XED_REG_R11D: case XED_REG_R11W: case XED_REG_R11B: return REG_E11;
        case XED_REG_R12: case XED_REG_R12D: case XED_REG_R12W: case XED_REG_R12B: return REG_E12;
        case XED_REG_R13: case XED_REG_R13D: case XED_REG_R13W: case XED_REG_R13B: return REG_E13;
        case XED_REG_R14: case XED_REG_R14D: case XED_REG_R14W: case XED_REG_R14B: return REG_E14;
        case XED_REG_R15: case XED_REG_R15D: case XED_REG_R15W: case XED_REG_R15B: return REG_E15;
        case XED_REG_CS:   return REG_CS;
        case XED_REG_DS:   return REG_DS;
        case XED_REG_ES:   return REG_ES;
        case XED_REG_FS:   return REG_FS;
        case XED_REG_GS:   return REG_GS;
        case XED_REG_SS:   return REG_SS;
        default:           return REG_NONE;
        // }}}
    }
}

void FPDecoderXED::expandInstCache(size_t newSize) {
    FPSemantics **instCacheTemp;
    unsigned i = 0;
    newSize = newSize > instCacheSize*2 ? newSize : instCacheSize*2;
    //printf("expand_inst_cache - old size: %d    new size: %d\n", instCacheSize, newSize);
    instCacheTemp = (FPSemantics**)malloc(newSize * sizeof(FPSemantics*));
    if (!instCacheTemp) {
        fprintf(stderr, "OUT OF MEMORY!\n");
        exit(-1);
    }
    if (instCacheArray != NULL) {
        for (; i < instCacheSize; i++) {
            instCacheTemp[i] = instCacheArray[i];
        }
        free(instCacheArray);
        instCacheArray = NULL;
    }
    for (; i < newSize; i++) {
        instCacheTemp[i] = NULL;
    }
    instCacheArray = instCacheTemp;
    instCacheSize = newSize;
}

bool FPDecoderXED::filter(unsigned char *bytes, size_t nbytes)
{
    char buffer[1048];
    bool add = false;
    xed_decoded_inst_t xedd;
    xed_error_enum_t xed_error;
    xed_iclass_enum_t iclass;
    xed_category_enum_t icategory;
    xed_extension_enum_t iextension;
    //xed_isa_set_enum_t iisaset;
    xed_decoded_inst_zero_set_mode(&xedd, &dstate);
    xed_decoded_inst_set_input_chip(&xedd, XED_CHIP_INVALID);
    xed_error = xed_decode(&xedd, (const xed_uint8_t*) bytes, nbytes);
    if (xed_error == XED_ERROR_NONE) { 
        xed_format_intel(&xedd, buffer, 1048, (xed_uint64_t)(long)bytes);
        iclass = xed_decoded_inst_get_iclass(&xedd);
        icategory = xed_decoded_inst_get_category(&xedd);
        iextension = xed_decoded_inst_get_extension(&xedd);
        //iisaset = xed_decoded_inst_get_isa_set(&xedd);
        if (icategory == XED_CATEGORY_SSE ||
            icategory == XED_CATEGORY_X87_ALU ||
            iextension == XED_EXTENSION_MMX ||
            iextension == XED_EXTENSION_SSE ||
            iextension == XED_EXTENSION_SSE2 ||
            iextension == XED_EXTENSION_SSE3 ||
            iextension == XED_EXTENSION_SSE4 ||
            iextension == XED_EXTENSION_SSE4A ||
            iextension == XED_EXTENSION_X87 ||
            iclass == XED_ICLASS_BTC) {
            add = true;
        }
        // TODO: decode and check instruction type & ISA extension
        /*
         *cout << "      XED2 " << (add ? "FP instruction: " : "ignored: ");
         *cout << "\"" << buffer << "\" ";
         *cout << "iclass="
         *    << xed_iclass_enum_t2str(iclass) << " ";
         *cout << "category="
         *    << xed_category_enum_t2str(icategory) << " ";
         *cout << "ISA-extension="
         *    << xed_extension_enum_t2str(iextension) << " ";
         *cout << "ISA-set="
         *    << xed_isa_set_enum_t2str(iisaset) << " ";
         *cout << "[" << Bytes2Str(bytes, nbytes) << "]" << endl;
         */
    }
    return add;
}

FPSemantics* FPDecoderXED::decode(unsigned long iidx, void *addr, unsigned char *bytes, size_t nbytes)
{
    FPSemantics *inst;
    if (instCacheSize <= iidx) {
        expandInstCache(iidx >= instCacheSize*2 ? iidx+1 : instCacheSize*2);
        inst = build(iidx, addr, bytes, nbytes);
        instCacheArray[iidx] = inst;
    } else {
        inst = instCacheArray[iidx];
        if (inst == NULL) {
            inst = build(iidx, addr, bytes, nbytes);
            instCacheArray[iidx] = inst;
        }
    }
    return inst;
}

FPSemantics* FPDecoderXED::lookup(unsigned long iidx)
{
    return (iidx < instCacheSize) ? instCacheArray[iidx] : NULL;
}

FPSemantics* FPDecoderXED::build(unsigned long index, void *addr, unsigned char *bytes, size_t nbytes)
{
    // {{{ decoder data

    // xed data structures
    xed_decoded_inst_t xedd;
    xed_error_enum_t xed_error;
    xed_inst_t* inst;
    //xed_iclass_enum_t iclass;
    xed_iform_enum_t iform;
    xed_operand_t *c_operand;
    xed_operand_enum_t c_op_name;
    //xed_operand_type_enum_t c_op_type;
    //xed_operand_width_enum_t c_op_width;
    xed_reg_enum_t c_reg_enum;
    stringstream debugData;
    debugData.str("");

    // misc data structures
    FPSemantics *semantics;
    FPOperation *operation, *temp_op;
    FPOperand *temp_opr;
    //unsigned long flags;
    char buffer[1048];

    // counters
    unsigned i;
    unsigned mi /*, memops*/;

    // }}}

    // {{{ initialize structures and extract general information

    // initialize our framework
    semantics = new FPSemantics();
    operation = new FPOperation();
    semantics->setIndex(index);
    semantics->setAddress(addr);
    semantics->setBytes(bytes, nbytes);

    // call XED to build its internal structures
    xed_decoded_inst_zero_set_mode(&xedd, &dstate);
    xed_decoded_inst_set_input_chip(&xedd, XED_CHIP_INVALID);
    xed_error = xed_decode(&xedd, (const xed_uint8_t*) bytes, nbytes);
    if (xed_error != XED_ERROR_NONE) { 
        //fprintf(stderr, "error - unable to decode instruction [code=%s]:  ", xed_error_enum_t2str(xed_error));
        //printInstBytes(stderr, bytes, nbytes);
        //fprintf(stderr, "\n");
        semantics->setDisassembly(Bytes2Str(bytes, nbytes));
        return semantics;
    }
    
    // save disassembly
    xed_format_intel(&xedd, buffer, 1048, (xed_uint64_t)(long)bytes);
    //printf("decoded assembly: %s\n", buffer);
    semantics->setDisassembly(string(buffer));

    debugData << "XED2 iclass "
        << xed_iclass_enum_t2str(xed_decoded_inst_get_iclass(&xedd))  << "\t";
    debugData << "category "
        << xed_category_enum_t2str(xed_decoded_inst_get_category(&xedd))  << "\t";
    debugData << "ISA-extension "
        << xed_extension_enum_t2str(xed_decoded_inst_get_extension(&xedd)) << "\t";
    debugData << "ISA-set "
        << xed_isa_set_enum_t2str(xed_decoded_inst_get_isa_set(&xedd))  << endl;
    debugData << "instruction-length "
        << xed_decoded_inst_get_length(&xedd) << endl;
    debugData << "operand-width "
        << xed_decoded_inst_get_operand_width(&xedd)  << endl;
    debugData << "effective-operand-width "
        << xed_operand_values_get_effective_operand_width(xed_decoded_inst_operands_const(&xedd))
        << endl;
    debugData << "effective-address-width "
        << xed_operand_values_get_effective_address_width(xed_decoded_inst_operands_const(&xedd))
        << endl;
    debugData << "stack-address-width "
        << xed_operand_values_get_stack_address_width(xed_decoded_inst_operands_const(&xedd))
        << endl;
    debugData << "iform-enum-name "
        << xed_iform_enum_t2str(xed_decoded_inst_get_iform_enum(&xedd)) << endl;
    debugData << "iform-enum-name-dispatch (zero based) "
        << xed_decoded_inst_get_iform_enum_dispatch(&xedd) << endl;
    debugData << "iclass-max-iform-dispatch "
        << xed_iform_max_per_iclass(xed_decoded_inst_get_iclass(&xedd))  << endl;

    // extract type and flags
    inst = (xed_inst_t*)xed_decoded_inst_inst(&xedd);
    iform = XED_IFORM_INVALID;
    if (!inst) {
        //fprintf(stderr, "error - unable to extract instruction: %s [%p]\n",
               //semantics->getDisassembly().c_str(), 
               //semantics->getAddress());
        operation = new FPOperation(OP_INVALID);
    } else {
        //iclass = xed_decoded_inst_get_iclass(&xedd);
        iform = xed_decoded_inst_get_iform_enum(&xedd);
    }
    /*if (operation->getType() == OP_NONE || operation->getType() == OP_INVALID) {
        //fprintf(stderr, "error - unhandled instruction: %s [%p]\n",
               //semantics->getDisassembly().c_str(), 
               //semantics->getAddress());
        semantics->add(operation);
        return semantics;
    }*/

    // }}}

    // add extra semantics to handle a x87 stack push
    for (i = 0; i < xed_inst_noperands(inst); i++) {
        c_operand = (xed_operand_t*)xed_inst_operand(inst, i);
        c_op_name = xed_operand_name(c_operand);
        c_reg_enum = xed_decoded_inst_get_reg(&xedd, c_op_name);
        debugData << "OP" << i 
            << " op-name " << xed_operand_enum_t2str(c_op_name) 
            << " reg-enum " << xed_reg_enum_t2str(c_reg_enum) 
            << endl;
        if (c_reg_enum == XED_REG_X87PUSH) {
            temp_op = new FPOperation();
            temp_op->setType(OP_PUSH_X87);
            semantics->add(temp_op);
        }
    }

#define ADD_X87_PUSH temp_op = new FPOperation(); temp_op->setType(OP_PUSH_X87); semantics->add(temp_op)
#define ADD_X87_POP temp_op = new FPOperation(); temp_op->setType(OP_POP_X87); semantics->add(temp_op)

#define ZERO_OP(TYPE,OP,TAG) temp_op = new FPOperation(OP_ZERO); temp_op->addOutputOperand(new FPOperand(TYPE, OP, TAG)); semantics->add(temp_op)

#define TYPE_INVALID operation->setType(OP_INVALID)
#define OP_TYPE(TYPE) operation->setType(TYPE)

#define INPUT_OP(TYPE,OP,TAG) operation->addInputOperand(new FPOperand(TYPE, OP, TAG))
#define OUTPUT_OP(TYPE,OP,TAG) operation->addOutputOperand(new FPOperand(TYPE, OP, TAG))

#define REG_OP(IDX) xedReg2FPReg(xed_decoded_inst_get_reg(&xedd, xed_operand_name(xed_inst_operand(inst,(IDX)))))

#define MEMORY_OP      xedReg2FPReg(xed_decoded_inst_get_base_reg(&xedd,mi)), \
                       xedReg2FPReg(xed_decoded_inst_get_index_reg(&xedd,mi)), \
                       (xed_decoded_inst_get_memory_displacement_width(&xedd,mi) ? \
                          (long)xed_decoded_inst_get_memory_displacement(&xedd,mi) : 0), \
                       (xedReg2FPReg(xed_decoded_inst_get_index_reg(&xedd,mi)) != REG_NONE ? \
                          (long)xed_decoded_inst_get_scale(&xedd,mi) : 1), \
                       xedReg2FPReg(xed_decoded_inst_get_seg_reg(&xedd,mi))

    /*memops = xed_decoded_inst_number_of_memory_operands(&xedd);*/
    mi = 0;
    switch (iform) {

        // {{{ x87 stack loads
        case XED_IFORM_FLD_ST0_MEMm64real:
            //ADD_X87_PUSH;
            OP_TYPE(OP_CVT); INPUT_OP(IEEE_Double, MEMORY_OP, 0); mi++;
                             OUTPUT_OP(C99_LongDouble, REG_ST0, 0); break;
        case XED_IFORM_FLD_ST0_MEMmem32real:
            //ADD_X87_PUSH;
            OP_TYPE(OP_CVT); INPUT_OP(IEEE_Single, MEMORY_OP, 0); mi++;
                             OUTPUT_OP(C99_LongDouble, REG_ST0, 0); break;
        case XED_IFORM_FLD_ST0_MEMmem80real:
            //ADD_X87_PUSH;
            OP_TYPE(OP_MOV); INPUT_OP(C99_LongDouble, MEMORY_OP, 0); mi++;
                             OUTPUT_OP(C99_LongDouble, REG_ST0, 0); break;
        case XED_IFORM_FLD_ST0_X87:
            //ADD_X87_PUSH;
            OP_TYPE(OP_MOV); INPUT_OP(C99_LongDouble, REG_OP(1), 0);
                             OUTPUT_OP(C99_LongDouble, REG_ST0, 0); break;
        case XED_IFORM_FLD1_ST0:
            //ADD_X87_PUSH;
            OP_TYPE(OP_MOV); operation->addInputOperand(new FPOperand(1.0L));
                             OUTPUT_OP(C99_LongDouble, REG_ST0, 0); break;
        case XED_IFORM_FLDZ_ST0:
            //ADD_X87_PUSH;
            OP_TYPE(OP_MOV); operation->addInputOperand(new FPOperand(0.0L));
                             OUTPUT_OP(C99_LongDouble, REG_ST0, 0); break;
        case XED_IFORM_FILD_ST0_MEMm64int:
            //ADD_X87_PUSH;
            OP_TYPE(OP_MOV); INPUT_OP(SignedInt64, MEMORY_OP, 0); mi++;
                             OUTPUT_OP(C99_LongDouble, REG_ST0, 0); break;
        case XED_IFORM_FILD_ST0_MEMmem16int:
            //ADD_X87_PUSH;
            OP_TYPE(OP_MOV); INPUT_OP(SignedInt16, MEMORY_OP, 0); mi++;
                             OUTPUT_OP(C99_LongDouble, REG_ST0, 0); break;
        case XED_IFORM_FILD_ST0_MEMmem32int:
            //ADD_X87_PUSH;
            OP_TYPE(OP_MOV); INPUT_OP(SignedInt32, MEMORY_OP, 0); mi++;
                             OUTPUT_OP(C99_LongDouble, REG_ST0, 0); break;
        // }}}

        // {{{ x87 stack stores
        case XED_IFORM_FST_MEMm64real_ST0:
            OP_TYPE(OP_CVT); INPUT_OP(C99_LongDouble, REG_ST0, 0);
                             OUTPUT_OP(IEEE_Double, MEMORY_OP, 0); mi++; break;
        case XED_IFORM_FST_MEMmem32real_ST0:
            OP_TYPE(OP_CVT); INPUT_OP(C99_LongDouble, REG_ST0, 0);
                             OUTPUT_OP(IEEE_Single, MEMORY_OP, 0); mi++; break;
        case XED_IFORM_FST_X87_ST0:
            OP_TYPE(OP_MOV); INPUT_OP(C99_LongDouble, REG_ST0, 0);
                             OUTPUT_OP(C99_LongDouble, REG_OP(0), 0); break;
        case XED_IFORM_FSTP_MEMm64real_ST0:
            OP_TYPE(OP_CVT); INPUT_OP(C99_LongDouble, REG_ST0, 0);
                             OUTPUT_OP(IEEE_Double, MEMORY_OP, 0); mi++;
            //ADD_X87_POP;
            break;
        case XED_IFORM_FSTP_MEMmem32real_ST0:
            OP_TYPE(OP_CVT); INPUT_OP(C99_LongDouble, REG_ST0, 0);
                             OUTPUT_OP(IEEE_Single, MEMORY_OP, 0); mi++;
            //ADD_X87_POP;
            break;
        case XED_IFORM_FSTP_MEMmem80real_ST0:
            OP_TYPE(OP_MOV); INPUT_OP(C99_LongDouble, REG_ST0, 0);
                             OUTPUT_OP(C99_LongDouble, MEMORY_OP, 0); mi++;
            //ADD_X87_POP;
            break;
        case XED_IFORM_FSTP_X87_ST0:
        case XED_IFORM_FSTP_X87_ST0_DFD0:
        case XED_IFORM_FSTP_X87_ST0_DFD1:
            OP_TYPE(OP_MOV); INPUT_OP(C99_LongDouble, REG_ST0, 0);
                             OUTPUT_OP(C99_LongDouble, REG_OP(0), 0);
            //ADD_X87_POP;
            break;
        case XED_IFORM_FIST_MEMmem16int_ST0:
            OP_TYPE(OP_CVT); INPUT_OP(C99_LongDouble, REG_ST0, 0);
                             OUTPUT_OP(SignedInt16, MEMORY_OP, 0); mi++; break;
        case XED_IFORM_FIST_MEMmem32int_ST0:
            OP_TYPE(OP_CVT); INPUT_OP(C99_LongDouble, REG_ST0, 0);
                             OUTPUT_OP(SignedInt32, MEMORY_OP, 0); mi++; break;
        case XED_IFORM_FISTP_MEMmem16int_ST0:
        case XED_IFORM_FISTTP_MEMmem16int_ST0:
            OP_TYPE(OP_CVT); INPUT_OP(C99_LongDouble, REG_ST0, 0);
                             OUTPUT_OP(SignedInt16, MEMORY_OP, 0); mi++;
            //ADD_X87_POP;
            break;
        case XED_IFORM_FISTP_MEMmem32int_ST0:
        case XED_IFORM_FISTTP_MEMmem32int_ST0:
            OP_TYPE(OP_CVT); INPUT_OP(C99_LongDouble, REG_ST0, 0);
                             OUTPUT_OP(SignedInt32, MEMORY_OP, 0); mi++;
            //ADD_X87_POP;
            break;
        case XED_IFORM_FISTP_MEMm64int_ST0:
        case XED_IFORM_FISTTP_MEMm64int_ST0:
            OP_TYPE(OP_CVT); INPUT_OP(C99_LongDouble, REG_ST0, 0);
                             OUTPUT_OP(SignedInt64, MEMORY_OP, 0); mi++;
            //ADD_X87_POP;
            break;
        // }}}

        // {{{ x87 arithmetic
        case XED_IFORM_FADD_ST0_MEMm64real:
            OP_TYPE(OP_ADD); INPUT_OP(C99_LongDouble, REG_ST0, 0); INPUT_OP(IEEE_Double, MEMORY_OP, 0); mi++;
                             OUTPUT_OP(C99_LongDouble, REG_ST0, 0); break;
        case XED_IFORM_FADD_ST0_MEMmem32real:
            OP_TYPE(OP_ADD); INPUT_OP(C99_LongDouble, REG_ST0, 0); INPUT_OP(IEEE_Single, MEMORY_OP, 0); mi++;
                             OUTPUT_OP(C99_LongDouble, REG_ST0, 0); break;
        case XED_IFORM_FIADD_ST0_MEMmem16int:
            OP_TYPE(OP_ADD); INPUT_OP(C99_LongDouble, REG_ST0, 0); INPUT_OP(SignedInt16, MEMORY_OP, 0); mi++;
                             OUTPUT_OP(C99_LongDouble, REG_ST0, 0); break;
        case XED_IFORM_FIADD_ST0_MEMmem32int:
            OP_TYPE(OP_ADD); INPUT_OP(C99_LongDouble, REG_ST0, 0); INPUT_OP(SignedInt32, MEMORY_OP, 0); mi++;
                             OUTPUT_OP(C99_LongDouble, REG_ST0, 0); break;
        case XED_IFORM_FADD_ST0_X87:
            OP_TYPE(OP_ADD); INPUT_OP(C99_LongDouble, REG_ST0, 0); INPUT_OP(C99_LongDouble, REG_OP(1), 0);
                             OUTPUT_OP(C99_LongDouble, REG_ST0, 0); break;
        case XED_IFORM_FADD_X87_ST0:
            OP_TYPE(OP_ADD); INPUT_OP(C99_LongDouble, REG_OP(0), 0); INPUT_OP(C99_LongDouble, REG_ST0, 0);
                             OUTPUT_OP(C99_LongDouble, REG_OP(0), 0); break;
        case XED_IFORM_FADDP_X87_ST0:
            OP_TYPE(OP_ADD); INPUT_OP(C99_LongDouble, REG_OP(0), 0); INPUT_OP(C99_LongDouble, REG_ST0, 0);
                             OUTPUT_OP(C99_LongDouble, REG_OP(0), 0);
            //ADD_X87_POP;
            break;

        case XED_IFORM_FSUB_ST0_MEMm64real:
            OP_TYPE(OP_SUB); INPUT_OP(C99_LongDouble, REG_ST0, 0); INPUT_OP(IEEE_Double, MEMORY_OP, 0); mi++;
                             OUTPUT_OP(C99_LongDouble, REG_ST0, 0); break;
        case XED_IFORM_FSUB_ST0_MEMmem32real:
            OP_TYPE(OP_SUB); INPUT_OP(C99_LongDouble, REG_ST0, 0); INPUT_OP(IEEE_Single, MEMORY_OP, 0); mi++;
                             OUTPUT_OP(C99_LongDouble, REG_ST0, 0); break;
        case XED_IFORM_FISUB_ST0_MEMmem16int:
            OP_TYPE(OP_SUB); INPUT_OP(C99_LongDouble, REG_ST0, 0); INPUT_OP(SignedInt16, MEMORY_OP, 0); mi++;
                             OUTPUT_OP(C99_LongDouble, REG_ST0, 0); break;
        case XED_IFORM_FISUB_ST0_MEMmem32int:
            OP_TYPE(OP_SUB); INPUT_OP(C99_LongDouble, REG_ST0, 0); INPUT_OP(SignedInt32, MEMORY_OP, 0); mi++;
                             OUTPUT_OP(C99_LongDouble, REG_ST0, 0); break;
        case XED_IFORM_FSUB_ST0_X87:
            OP_TYPE(OP_SUB); INPUT_OP(C99_LongDouble, REG_ST0, 0); INPUT_OP(C99_LongDouble, REG_OP(1), 0);
                             OUTPUT_OP(C99_LongDouble, REG_ST0, 0); break;
        case XED_IFORM_FSUB_X87_ST0:
            OP_TYPE(OP_SUB); INPUT_OP(C99_LongDouble, REG_OP(0), 0); INPUT_OP(C99_LongDouble, REG_ST0, 0);
                             OUTPUT_OP(C99_LongDouble, REG_OP(0), 0); break;
        case XED_IFORM_FSUBP_X87_ST0:
            OP_TYPE(OP_SUB); INPUT_OP(C99_LongDouble, REG_OP(0), 0); INPUT_OP(C99_LongDouble, REG_ST0, 0);
                             OUTPUT_OP(C99_LongDouble, REG_OP(0), 0);
            //ADD_X87_POP;
            break;
        case XED_IFORM_FSUBR_ST0_MEMm64real:
            OP_TYPE(OP_SUB); INPUT_OP(IEEE_Double, MEMORY_OP, 0); mi++; INPUT_OP(C99_LongDouble, REG_ST0, 0);
                             OUTPUT_OP(C99_LongDouble, REG_ST0, 0); break;
        case XED_IFORM_FSUBR_ST0_MEMmem32real:
            OP_TYPE(OP_SUB); INPUT_OP(IEEE_Single, MEMORY_OP, 0); mi++; INPUT_OP(C99_LongDouble, REG_ST0, 0);
                             OUTPUT_OP(C99_LongDouble, REG_ST0, 0); break;
        case XED_IFORM_FISUBR_ST0_MEMmem16int:
            OP_TYPE(OP_SUB); INPUT_OP(SignedInt16, MEMORY_OP, 0); mi++; INPUT_OP(C99_LongDouble, REG_ST0, 0);
                             OUTPUT_OP(C99_LongDouble, REG_ST0, 0); break;
        case XED_IFORM_FISUBR_ST0_MEMmem32int:
            OP_TYPE(OP_SUB); INPUT_OP(SignedInt32, MEMORY_OP, 0); mi++; INPUT_OP(C99_LongDouble, REG_ST0, 0);
                             OUTPUT_OP(C99_LongDouble, REG_ST0, 0); break;
        case XED_IFORM_FSUBR_ST0_X87:
            OP_TYPE(OP_SUB); INPUT_OP(C99_LongDouble, REG_OP(1), 0); INPUT_OP(C99_LongDouble, REG_ST0, 0);
                             OUTPUT_OP(C99_LongDouble, REG_ST0, 0); break;
        case XED_IFORM_FSUBR_X87_ST0:
            OP_TYPE(OP_SUB); INPUT_OP(C99_LongDouble, REG_ST0, 0); INPUT_OP(C99_LongDouble, REG_OP(0), 0);
                             OUTPUT_OP(C99_LongDouble, REG_OP(0), 0); break;
        case XED_IFORM_FSUBRP_X87_ST0:
            OP_TYPE(OP_SUB); INPUT_OP(C99_LongDouble, REG_ST0, 0); INPUT_OP(C99_LongDouble, REG_OP(0), 0);
                             OUTPUT_OP(C99_LongDouble, REG_OP(0), 0);
            //ADD_X87_POP;
            break;

        case XED_IFORM_FMUL_ST0_MEMm64real:
            OP_TYPE(OP_MUL); INPUT_OP(C99_LongDouble, REG_ST0, 0); INPUT_OP(IEEE_Double, MEMORY_OP, 0); mi++;
                             OUTPUT_OP(C99_LongDouble, REG_ST0, 0); break;
        case XED_IFORM_FMUL_ST0_MEMmem32real:
            OP_TYPE(OP_MUL); INPUT_OP(C99_LongDouble, REG_ST0, 0); INPUT_OP(IEEE_Single, MEMORY_OP, 0); mi++;
                             OUTPUT_OP(C99_LongDouble, REG_ST0, 0); break;
        case XED_IFORM_FIMUL_ST0_MEMmem16int:
            OP_TYPE(OP_MUL); INPUT_OP(C99_LongDouble, REG_ST0, 0); INPUT_OP(SignedInt16, MEMORY_OP, 0); mi++;
                             OUTPUT_OP(C99_LongDouble, REG_ST0, 0); break;
        case XED_IFORM_FIMUL_ST0_MEMmem32int:
            OP_TYPE(OP_MUL); INPUT_OP(C99_LongDouble, REG_ST0, 0); INPUT_OP(SignedInt32, MEMORY_OP, 0); mi++;
                             OUTPUT_OP(C99_LongDouble, REG_ST0, 0); break;
        case XED_IFORM_FMUL_ST0_X87:
            OP_TYPE(OP_MUL); INPUT_OP(C99_LongDouble, REG_ST0, 0); INPUT_OP(C99_LongDouble, REG_OP(1), 0);
                             OUTPUT_OP(C99_LongDouble, REG_ST0, 0); break;
        case XED_IFORM_FMUL_X87_ST0:
            OP_TYPE(OP_MUL); INPUT_OP(C99_LongDouble, REG_OP(0), 0); INPUT_OP(C99_LongDouble, REG_ST0, 0);
                             OUTPUT_OP(C99_LongDouble, REG_OP(0), 0); break;
        case XED_IFORM_FMULP_X87_ST0:
            OP_TYPE(OP_MUL); INPUT_OP(C99_LongDouble, REG_OP(0), 0); INPUT_OP(C99_LongDouble, REG_ST0, 0);
                             OUTPUT_OP(C99_LongDouble, REG_OP(0), 0);
            //ADD_X87_POP;
            break;

        case XED_IFORM_FDIV_ST0_MEMm64real:
            OP_TYPE(OP_DIV); INPUT_OP(C99_LongDouble, REG_ST0, 0); INPUT_OP(IEEE_Double, MEMORY_OP, 0); mi++;
                             OUTPUT_OP(C99_LongDouble, REG_ST0, 0); break;
        case XED_IFORM_FDIV_ST0_MEMmem32real:
            OP_TYPE(OP_DIV); INPUT_OP(C99_LongDouble, REG_ST0, 0); INPUT_OP(IEEE_Single, MEMORY_OP, 0); mi++;
                             OUTPUT_OP(C99_LongDouble, REG_ST0, 0); break;
        case XED_IFORM_FIDIV_ST0_MEMmem16int:
            OP_TYPE(OP_DIV); INPUT_OP(C99_LongDouble, REG_ST0, 0); INPUT_OP(SignedInt16, MEMORY_OP, 0); mi++;
                             OUTPUT_OP(C99_LongDouble, REG_ST0, 0); break;
        case XED_IFORM_FIDIV_ST0_MEMmem32int:
            OP_TYPE(OP_DIV); INPUT_OP(C99_LongDouble, REG_ST0, 0); INPUT_OP(SignedInt32, MEMORY_OP, 0); mi++;
                             OUTPUT_OP(C99_LongDouble, REG_ST0, 0); break;
        case XED_IFORM_FDIV_ST0_X87:
            OP_TYPE(OP_DIV); INPUT_OP(C99_LongDouble, REG_ST0, 0); INPUT_OP(C99_LongDouble, REG_OP(1), 0);
                             OUTPUT_OP(C99_LongDouble, REG_ST0, 0); break;
        case XED_IFORM_FDIV_X87_ST0:
            OP_TYPE(OP_DIV); INPUT_OP(C99_LongDouble, REG_OP(0), 0); INPUT_OP(C99_LongDouble, REG_ST0, 0);
                             OUTPUT_OP(C99_LongDouble, REG_OP(0), 0); break;
        case XED_IFORM_FDIVP_X87_ST0:
            OP_TYPE(OP_DIV); INPUT_OP(C99_LongDouble, REG_OP(0), 0); INPUT_OP(C99_LongDouble, REG_ST0, 0);
                             OUTPUT_OP(C99_LongDouble, REG_OP(0), 0);
            //ADD_X87_POP;
            break;
        case XED_IFORM_FDIVR_ST0_MEMm64real:
            OP_TYPE(OP_DIV); INPUT_OP(IEEE_Double, MEMORY_OP, 0); mi++; INPUT_OP(C99_LongDouble, REG_ST0, 0);
                             OUTPUT_OP(C99_LongDouble, REG_ST0, 0); break;
        case XED_IFORM_FDIVR_ST0_MEMmem32real:
            OP_TYPE(OP_DIV); INPUT_OP(IEEE_Single, MEMORY_OP, 0); mi++; INPUT_OP(C99_LongDouble, REG_ST0, 0);
                             OUTPUT_OP(C99_LongDouble, REG_ST0, 0); break;
        case XED_IFORM_FIDIVR_ST0_MEMmem16int:
            OP_TYPE(OP_DIV); INPUT_OP(SignedInt16, MEMORY_OP, 0); mi++; INPUT_OP(C99_LongDouble, REG_ST0, 0);
                             OUTPUT_OP(C99_LongDouble, REG_ST0, 0); break;
        case XED_IFORM_FIDIVR_ST0_MEMmem32int:
            OP_TYPE(OP_DIV); INPUT_OP(SignedInt32, MEMORY_OP, 0); mi++; INPUT_OP(C99_LongDouble, REG_ST0, 0);
                             OUTPUT_OP(C99_LongDouble, REG_ST0, 0); break;
        case XED_IFORM_FDIVR_ST0_X87:
            OP_TYPE(OP_DIV); INPUT_OP(C99_LongDouble, REG_OP(1), 0); INPUT_OP(C99_LongDouble, REG_ST0, 0);
                             OUTPUT_OP(C99_LongDouble, REG_ST0, 0); break;
        case XED_IFORM_FDIVR_X87_ST0:
            OP_TYPE(OP_DIV); INPUT_OP(C99_LongDouble, REG_ST0, 0); INPUT_OP(C99_LongDouble, REG_OP(0), 0);
                             OUTPUT_OP(C99_LongDouble, REG_OP(0), 0); break;
        case XED_IFORM_FDIVRP_X87_ST0:
            OP_TYPE(OP_DIV); INPUT_OP(C99_LongDouble, REG_ST0, 0); INPUT_OP(C99_LongDouble, REG_OP(0), 0);
                             OUTPUT_OP(C99_LongDouble, REG_OP(0), 0);
            //ADD_X87_POP;
            break;
        // }}}

        // {{{ x87 comparisons
        // TODO: retain more architecture-specific comparison information
        case XED_IFORM_FCOM_ST0_X87:
            OP_TYPE(OP_COM); INPUT_OP(C99_LongDouble, REG_ST0, 0); INPUT_OP(C99_LongDouble, REG_OP(1), 0);
                              operation->addOutputOperand(new FPOperand(UnsignedInt32, REG_EFLAGS)); break;
        case XED_IFORM_FCOMI_ST0_X87:
            OP_TYPE(OP_COMI); INPUT_OP(C99_LongDouble, REG_ST0, 0); INPUT_OP(C99_LongDouble, REG_OP(1), 0);
                              operation->addOutputOperand(new FPOperand(UnsignedInt32, REG_EFLAGS)); break;
        case XED_IFORM_FCOMIP_ST0_X87:
            OP_TYPE(OP_COMI); INPUT_OP(C99_LongDouble, REG_ST0, 0); INPUT_OP(C99_LongDouble, REG_OP(1), 0);
                              operation->addOutputOperand(new FPOperand(UnsignedInt32, REG_EFLAGS)); break;
        case XED_IFORM_FCOMP_ST0_X87:
            OP_TYPE(OP_COM); INPUT_OP(C99_LongDouble, REG_ST0, 0); INPUT_OP(C99_LongDouble, REG_OP(1), 0);
                              operation->addOutputOperand(new FPOperand(UnsignedInt32, REG_EFLAGS)); break;
        case XED_IFORM_FCOMPP_ST0_ST1:
            OP_TYPE(OP_COM); INPUT_OP(C99_LongDouble, REG_ST0, 0); INPUT_OP(C99_LongDouble, REG_OP(1), 0);
                              operation->addOutputOperand(new FPOperand(UnsignedInt32, REG_EFLAGS)); break;
        case XED_IFORM_FUCOM_ST0_X87:
            OP_TYPE(OP_UCOM); INPUT_OP(C99_LongDouble, REG_ST0, 0); INPUT_OP(C99_LongDouble, REG_OP(1), 0);
                              operation->addOutputOperand(new FPOperand(UnsignedInt32, REG_EFLAGS)); break;
        case XED_IFORM_FUCOMI_ST0_X87:
            OP_TYPE(OP_UCOMI); INPUT_OP(C99_LongDouble, REG_ST0, 0); INPUT_OP(C99_LongDouble, REG_OP(1), 0);
                              operation->addOutputOperand(new FPOperand(UnsignedInt32, REG_EFLAGS)); break;
        case XED_IFORM_FUCOMIP_ST0_X87:
            OP_TYPE(OP_UCOMI); INPUT_OP(C99_LongDouble, REG_ST0, 0); INPUT_OP(C99_LongDouble, REG_OP(1), 0);
                              operation->addOutputOperand(new FPOperand(UnsignedInt32, REG_EFLAGS)); break;
        case XED_IFORM_FUCOMP_ST0_X87:
            OP_TYPE(OP_UCOM); INPUT_OP(C99_LongDouble, REG_ST0, 0); INPUT_OP(C99_LongDouble, REG_OP(1), 0);
                              operation->addOutputOperand(new FPOperand(UnsignedInt32, REG_EFLAGS)); break;
        case XED_IFORM_FUCOMPP_ST0_ST1:
            OP_TYPE(OP_UCOM); INPUT_OP(C99_LongDouble, REG_ST0, 0); INPUT_OP(C99_LongDouble, REG_OP(1), 0);
                              operation->addOutputOperand(new FPOperand(UnsignedInt32, REG_EFLAGS)); break;

        // }}}

        // {{{ x87 misc operations
        case XED_IFORM_FABS_ST0:
            OP_TYPE(OP_ABS); INPUT_OP(C99_LongDouble, REG_ST0, 0); OUTPUT_OP(C99_LongDouble, REG_ST0, 0); break;
        case XED_IFORM_FCHS_ST0:
            OP_TYPE(OP_NEG); INPUT_OP(C99_LongDouble, REG_ST0, 0); OUTPUT_OP(C99_LongDouble, REG_ST0, 0); break;
        case XED_IFORM_FPREM_ST0_ST1:
        case XED_IFORM_FPREM1_ST0_ST1:
            OP_TYPE(OP_FMOD); INPUT_OP(C99_LongDouble, REG_ST0, 0); INPUT_OP(C99_LongDouble, REG_ST1, 0);
                              OUTPUT_OP(C99_LongDouble, REG_ST0, 0); break;
        case XED_IFORM_FSQRT_ST0:
            OP_TYPE(OP_SQRT); INPUT_OP(C99_LongDouble, REG_ST0, 0); OUTPUT_OP(C99_LongDouble, REG_ST0, 0); break;
        case XED_IFORM_FXAM_ST0:
            OP_TYPE(OP_AM); INPUT_OP(C99_LongDouble, REG_ST0, 0);
                            operation->addOutputOperand(new FPOperand(UnsignedInt32, REG_EFLAGS)); break;
        case XED_IFORM_FXCH_ST0_X87:
        case XED_IFORM_FXCH_ST0_X87_DDC1:
        case XED_IFORM_FXCH_ST0_X87_DFC1:
            OP_TYPE(OP_MOV); INPUT_OP(C99_LongDouble, REG_ST0, 0);
                             OUTPUT_OP(C99_LongDouble, REG_OP(1), 0);
                             INPUT_OP(C99_LongDouble, REG_OP(1), 0);
                             OUTPUT_OP(C99_LongDouble, REG_ST0, 0);
            break;
        // }}}

        // {{{ SSE single data movement
        case XED_IFORM_MOVSS_MEMss_XMMss:
            OP_TYPE(OP_MOV); INPUT_OP(IEEE_Single, REG_OP(1), 0);
                             OUTPUT_OP(IEEE_Single, MEMORY_OP, 0); mi++; break;
        case XED_IFORM_MOVSS_XMMdq_MEMss:
            OP_TYPE(OP_MOV); INPUT_OP(IEEE_Single, MEMORY_OP, 0); mi++;
                             ZERO_OP(SignedInt32, REG_OP(0), 1); ZERO_OP(SignedInt64, REG_OP(0), 2);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 0); break;
        case XED_IFORM_MOVSS_XMMss_XMMss_0F10:
        case XED_IFORM_MOVSS_XMMss_XMMss_0F11:
            OP_TYPE(OP_MOV); INPUT_OP(IEEE_Single, REG_OP(1), 0);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 0); break;
        case XED_IFORM_MOVAPS_MEMps_XMMps:
        case XED_IFORM_MOVUPS_MEMps_XMMps:
            OP_TYPE(OP_MOV); INPUT_OP(IEEE_Single, REG_OP(0), 0); OUTPUT_OP(IEEE_Single, MEMORY_OP, 0);
                             INPUT_OP(IEEE_Single, REG_OP(0), 1); OUTPUT_OP(IEEE_Single, MEMORY_OP, 1);
                             INPUT_OP(IEEE_Single, REG_OP(0), 2); OUTPUT_OP(IEEE_Single, MEMORY_OP, 2);
                             INPUT_OP(IEEE_Single, REG_OP(0), 3); OUTPUT_OP(IEEE_Single, MEMORY_OP, 3);
                             mi++; break;
        case XED_IFORM_MOVAPS_XMMps_MEMps:
        case XED_IFORM_MOVUPS_XMMps_MEMps:
            OP_TYPE(OP_MOV); INPUT_OP(IEEE_Single, MEMORY_OP, 0); OUTPUT_OP(IEEE_Single, REG_OP(0), 0);
                             INPUT_OP(IEEE_Single, MEMORY_OP, 1); OUTPUT_OP(IEEE_Single, REG_OP(0), 1);
                             INPUT_OP(IEEE_Single, MEMORY_OP, 2); OUTPUT_OP(IEEE_Single, REG_OP(0), 2);
                             INPUT_OP(IEEE_Single, MEMORY_OP, 3); OUTPUT_OP(IEEE_Single, REG_OP(0), 3);
                             mi++; break;
        case XED_IFORM_MOVAPS_XMMps_XMMps_0F28:
        case XED_IFORM_MOVAPS_XMMps_XMMps_0F29:
        case XED_IFORM_MOVUPS_XMMps_XMMps_0F10:
        case XED_IFORM_MOVUPS_XMMps_XMMps_0F11:
            OP_TYPE(OP_MOV); INPUT_OP(IEEE_Single, REG_OP(1), 0); OUTPUT_OP(IEEE_Single, REG_OP(0), 0);
                             INPUT_OP(IEEE_Single, REG_OP(1), 1); OUTPUT_OP(IEEE_Single, REG_OP(0), 1);
                             INPUT_OP(IEEE_Single, REG_OP(1), 2); OUTPUT_OP(IEEE_Single, REG_OP(0), 2);
                             INPUT_OP(IEEE_Single, REG_OP(1), 3); OUTPUT_OP(IEEE_Single, REG_OP(0), 3);
                             break;
        case XED_IFORM_MOVHPS_MEMq_XMMps:
            OP_TYPE(OP_MOV); INPUT_OP(IEEE_Single, REG_OP(0), 2); OUTPUT_OP(IEEE_Single, MEMORY_OP, 0);
                             INPUT_OP(IEEE_Single, REG_OP(0), 3); OUTPUT_OP(IEEE_Single, MEMORY_OP, 1);
                             mi++; break;
        case XED_IFORM_MOVHPS_XMMq_MEMq:
            OP_TYPE(OP_MOV); INPUT_OP(IEEE_Single, MEMORY_OP, 0); OUTPUT_OP(IEEE_Single, REG_OP(0), 2);
                             INPUT_OP(IEEE_Single, MEMORY_OP, 1); OUTPUT_OP(IEEE_Single, REG_OP(0), 3);
                             mi++; break;
        case XED_IFORM_MOVLPS_MEMq_XMMps:
            OP_TYPE(OP_MOV); INPUT_OP(IEEE_Single, REG_OP(0), 0); OUTPUT_OP(IEEE_Single, MEMORY_OP, 0);
                             INPUT_OP(IEEE_Single, REG_OP(0), 1); OUTPUT_OP(IEEE_Single, MEMORY_OP, 1);
                             mi++; break;
        case XED_IFORM_MOVLPS_XMMq_MEMq:
            OP_TYPE(OP_MOV); INPUT_OP(IEEE_Single, MEMORY_OP, 0); OUTPUT_OP(IEEE_Single, REG_OP(0), 0);
                             INPUT_OP(IEEE_Single, MEMORY_OP, 1); OUTPUT_OP(IEEE_Single, REG_OP(0), 1);
                             mi++; break;
        case XED_IFORM_MOVHLPS_XMMq_XMMq:
            OP_TYPE(OP_MOV); INPUT_OP(IEEE_Single, REG_OP(1), 2); OUTPUT_OP(IEEE_Single, REG_OP(0), 0);
                             INPUT_OP(IEEE_Single, REG_OP(1), 3); OUTPUT_OP(IEEE_Single, REG_OP(0), 1);
                             break;
        case XED_IFORM_MOVLHPS_XMMq_XMMq:
            OP_TYPE(OP_MOV); INPUT_OP(IEEE_Single, REG_OP(1), 0); OUTPUT_OP(IEEE_Single, REG_OP(0), 2);
                             INPUT_OP(IEEE_Single, REG_OP(1), 1); OUTPUT_OP(IEEE_Single, REG_OP(0), 3);
                             break;
        case XED_IFORM_UNPCKHPS_XMMps_MEMdq:
            OP_TYPE(OP_MOV); INPUT_OP(IEEE_Single, REG_OP(0), 2); OUTPUT_OP(IEEE_Single, REG_OP(0), 0);
                             INPUT_OP(IEEE_Single, MEMORY_OP, 2); OUTPUT_OP(IEEE_Single, REG_OP(0), 1);
                             INPUT_OP(IEEE_Single, REG_OP(0), 3); OUTPUT_OP(IEEE_Single, REG_OP(0), 2);
                             INPUT_OP(IEEE_Single, MEMORY_OP, 3); OUTPUT_OP(IEEE_Single, REG_OP(0), 3);
                             mi++; break;
        case XED_IFORM_PUNPCKHDQ_XMMdq_MEMdq:
            OP_TYPE(OP_MOV); INPUT_OP(SignedInt32, REG_OP(0), 2); OUTPUT_OP(SignedInt32, REG_OP(0), 0);
                             INPUT_OP(SignedInt32, MEMORY_OP, 2); OUTPUT_OP(SignedInt32, REG_OP(0), 1);
                             INPUT_OP(SignedInt32, REG_OP(0), 3); OUTPUT_OP(SignedInt32, REG_OP(0), 2);
                             INPUT_OP(SignedInt32, MEMORY_OP, 3); OUTPUT_OP(SignedInt32, REG_OP(0), 3);
                             mi++; break;
        case XED_IFORM_UNPCKHPS_XMMps_XMMdq:
            OP_TYPE(OP_MOV); INPUT_OP(IEEE_Single, REG_OP(0), 2); OUTPUT_OP(IEEE_Single, REG_OP(0), 0);
                             INPUT_OP(IEEE_Single, REG_OP(1), 2); OUTPUT_OP(IEEE_Single, REG_OP(0), 1);
                             INPUT_OP(IEEE_Single, REG_OP(0), 3); OUTPUT_OP(IEEE_Single, REG_OP(0), 2);
                             INPUT_OP(IEEE_Single, REG_OP(1), 3); OUTPUT_OP(IEEE_Single, REG_OP(0), 3);
                             break;
        case XED_IFORM_PUNPCKHDQ_XMMdq_XMMq:
            OP_TYPE(OP_MOV); INPUT_OP(SignedInt32, REG_OP(0), 2); OUTPUT_OP(SignedInt32, REG_OP(0), 0);
                             INPUT_OP(SignedInt32, REG_OP(1), 2); OUTPUT_OP(SignedInt32, REG_OP(0), 1);
                             INPUT_OP(SignedInt32, REG_OP(0), 3); OUTPUT_OP(SignedInt32, REG_OP(0), 2);
                             INPUT_OP(SignedInt32, REG_OP(1), 3); OUTPUT_OP(SignedInt32, REG_OP(0), 3);
                             break;
        case XED_IFORM_UNPCKLPS_XMMps_MEMdq:
            OP_TYPE(OP_MOV); INPUT_OP(IEEE_Single, REG_OP(0), 0); OUTPUT_OP(IEEE_Single, REG_OP(0), 0);
                             INPUT_OP(IEEE_Single, MEMORY_OP, 0); OUTPUT_OP(IEEE_Single, REG_OP(0), 1);
                             INPUT_OP(IEEE_Single, REG_OP(0), 1); OUTPUT_OP(IEEE_Single, REG_OP(0), 2);
                             INPUT_OP(IEEE_Single, MEMORY_OP, 1); OUTPUT_OP(IEEE_Single, REG_OP(0), 3);
                             mi++; break;
        case XED_IFORM_PUNPCKLDQ_XMMdq_MEMdq:
            OP_TYPE(OP_MOV); INPUT_OP(SignedInt32, REG_OP(0), 0); OUTPUT_OP(SignedInt32, REG_OP(0), 0);
                             INPUT_OP(SignedInt32, MEMORY_OP, 0); OUTPUT_OP(SignedInt32, REG_OP(0), 1);
                             INPUT_OP(SignedInt32, REG_OP(0), 1); OUTPUT_OP(SignedInt32, REG_OP(0), 2);
                             INPUT_OP(SignedInt32, MEMORY_OP, 1); OUTPUT_OP(SignedInt32, REG_OP(0), 3);
                             mi++; break;
        case XED_IFORM_UNPCKLPS_XMMps_XMMq:
            OP_TYPE(OP_MOV); INPUT_OP(IEEE_Single, REG_OP(0), 0); OUTPUT_OP(IEEE_Single, REG_OP(0), 0);
                             INPUT_OP(IEEE_Single, REG_OP(1), 0); OUTPUT_OP(IEEE_Single, REG_OP(0), 1);
                             INPUT_OP(IEEE_Single, REG_OP(0), 1); OUTPUT_OP(IEEE_Single, REG_OP(0), 2);
                             INPUT_OP(IEEE_Single, REG_OP(1), 1); OUTPUT_OP(IEEE_Single, REG_OP(0), 3);
                             break;
        case XED_IFORM_PUNPCKLDQ_XMMdq_XMMq:
            OP_TYPE(OP_MOV); INPUT_OP(SignedInt32, REG_OP(0), 0); OUTPUT_OP(SignedInt32, REG_OP(0), 0);
                             INPUT_OP(SignedInt32, REG_OP(1), 0); OUTPUT_OP(SignedInt32, REG_OP(0), 1);
                             INPUT_OP(SignedInt32, REG_OP(0), 1); OUTPUT_OP(SignedInt32, REG_OP(0), 2);
                             INPUT_OP(SignedInt32, REG_OP(1), 1); OUTPUT_OP(SignedInt32, REG_OP(0), 3);
                             break;

        // SSE shuffle; ignore for now b/c we don't have an analysis that cares
        // TODO: fix this at some point (possibly for pointer-based analysis)
        case XED_IFORM_SHUFPS_XMMps_MEMps_IMMb:
        case XED_IFORM_SHUFPS_XMMps_XMMps_IMMb:
            OP_TYPE(OP_NONE); break;

        //case XED_IFORM_PUNPCKHBW_MMXq_MEMq:
        //case XED_IFORM_PUNPCKHBW_MMXq_MMXd:
        //case XED_IFORM_PUNPCKHDQ_MMXq_MEMq:
        //case XED_IFORM_PUNPCKHDQ_MMXq_MMXd:
        //case XED_IFORM_PUNPCKHWD_MMXq_MEMq:
        //case XED_IFORM_PUNPCKHWD_MMXq_MMXd:
        //case XED_IFORM_PUNPCKLBW_MMXq_MEMd:
        //case XED_IFORM_PUNPCKLBW_MMXq_MMXd:
        //case XED_IFORM_PUNPCKLDQ_MMXq_MEMd:
        //case XED_IFORM_PUNPCKLDQ_MMXq_MMXd:
        //case XED_IFORM_PUNPCKLWD_MMXq_MEMd:
        //case XED_IFORM_PUNPCKLWD_MMXq_MMXd:


        // }}}

        // {{{ SSE single arithmetic
        case XED_IFORM_RCPSS_XMMss_MEMss:
            OP_TYPE(OP_RCP); INPUT_OP(IEEE_Single, MEMORY_OP, 0); OUTPUT_OP(IEEE_Single, REG_OP(0), 0); mi++; break;
        case XED_IFORM_RCPSS_XMMss_XMMss:
            OP_TYPE(OP_RCP); INPUT_OP(IEEE_Single, REG_OP(1), 0); OUTPUT_OP(IEEE_Single, REG_OP(0), 0); break;
        case XED_IFORM_ADDSS_XMMss_MEMss:
            OP_TYPE(OP_ADD); INPUT_OP(IEEE_Single, REG_OP(0), 0); INPUT_OP(IEEE_Single, MEMORY_OP, 0); mi++;
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 0); break;
        case XED_IFORM_ADDSS_XMMss_XMMss:
            OP_TYPE(OP_ADD); INPUT_OP(IEEE_Single, REG_OP(0), 0); INPUT_OP(IEEE_Single, REG_OP(1), 0);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 0); break;
        case XED_IFORM_SUBSS_XMMss_MEMss:
            OP_TYPE(OP_SUB); INPUT_OP(IEEE_Single, REG_OP(0), 0); INPUT_OP(IEEE_Single, MEMORY_OP, 0); mi++;
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 0); break;
        case XED_IFORM_SUBSS_XMMss_XMMss:
            OP_TYPE(OP_SUB); INPUT_OP(IEEE_Single, REG_OP(0), 0); INPUT_OP(IEEE_Single, REG_OP(1), 0);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 0); break;
        case XED_IFORM_MULSS_XMMss_MEMss:
            OP_TYPE(OP_MUL); INPUT_OP(IEEE_Single, REG_OP(0), 0); INPUT_OP(IEEE_Single, MEMORY_OP, 0); mi++;
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 0); break;
        case XED_IFORM_MULSS_XMMss_XMMss:
            OP_TYPE(OP_MUL); INPUT_OP(IEEE_Single, REG_OP(0), 0); INPUT_OP(IEEE_Single, REG_OP(1), 0);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 0); break;
        case XED_IFORM_DIVSS_XMMss_MEMss:
            OP_TYPE(OP_DIV); INPUT_OP(IEEE_Single, REG_OP(0), 0); INPUT_OP(IEEE_Single, MEMORY_OP, 0); mi++;
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 0); break;
        case XED_IFORM_DIVSS_XMMss_XMMss:
            OP_TYPE(OP_DIV); INPUT_OP(IEEE_Single, REG_OP(0), 0); INPUT_OP(IEEE_Single, REG_OP(1), 0);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 0); break;

        case XED_IFORM_RCPPS_XMMps_MEMps:
            OP_TYPE(OP_RCP); INPUT_OP(IEEE_Single, MEMORY_OP, 0); OUTPUT_OP(IEEE_Single, REG_OP(0), 0);
                             INPUT_OP(IEEE_Single, MEMORY_OP, 1); OUTPUT_OP(IEEE_Single, REG_OP(0), 1);
                             INPUT_OP(IEEE_Single, MEMORY_OP, 2); OUTPUT_OP(IEEE_Single, REG_OP(0), 2);
                             INPUT_OP(IEEE_Single, MEMORY_OP, 3); OUTPUT_OP(IEEE_Single, REG_OP(0), 3);
                             mi++; break;
        case XED_IFORM_RCPPS_XMMps_XMMps:
            OP_TYPE(OP_RCP); INPUT_OP(IEEE_Single, REG_OP(1), 0); OUTPUT_OP(IEEE_Single, REG_OP(0), 0);
                             INPUT_OP(IEEE_Single, REG_OP(1), 1); OUTPUT_OP(IEEE_Single, REG_OP(0), 1);
                             INPUT_OP(IEEE_Single, REG_OP(1), 2); OUTPUT_OP(IEEE_Single, REG_OP(0), 2);
                             INPUT_OP(IEEE_Single, REG_OP(1), 3); OUTPUT_OP(IEEE_Single, REG_OP(0), 3);
                             break;
        case XED_IFORM_ADDPS_XMMps_MEMps:
            OP_TYPE(OP_ADD); INPUT_OP(IEEE_Single, REG_OP(0), 0); INPUT_OP(IEEE_Single, MEMORY_OP, 0);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 0);
                             INPUT_OP(IEEE_Single, REG_OP(0), 1); INPUT_OP(IEEE_Single, MEMORY_OP, 1);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 1);
                             INPUT_OP(IEEE_Single, REG_OP(0), 2); INPUT_OP(IEEE_Single, MEMORY_OP, 2);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 2);
                             INPUT_OP(IEEE_Single, REG_OP(0), 3); INPUT_OP(IEEE_Single, MEMORY_OP, 3);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 3);
                             mi++; break;
        case XED_IFORM_ADDPS_XMMps_XMMps:
            OP_TYPE(OP_ADD); INPUT_OP(IEEE_Single, REG_OP(0), 0); INPUT_OP(IEEE_Single, REG_OP(1), 0);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 0);
                             INPUT_OP(IEEE_Single, REG_OP(0), 1); INPUT_OP(IEEE_Single, REG_OP(1), 1);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 1);
                             INPUT_OP(IEEE_Single, REG_OP(0), 2); INPUT_OP(IEEE_Single, REG_OP(1), 2);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 2);
                             INPUT_OP(IEEE_Single, REG_OP(0), 3); INPUT_OP(IEEE_Single, REG_OP(1), 3);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 3);
                             break;
        case XED_IFORM_SUBPS_XMMps_MEMps:
            OP_TYPE(OP_SUB); INPUT_OP(IEEE_Single, REG_OP(0), 0); INPUT_OP(IEEE_Single, MEMORY_OP, 0);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 0);
                             INPUT_OP(IEEE_Single, REG_OP(0), 1); INPUT_OP(IEEE_Single, MEMORY_OP, 1);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 1);
                             INPUT_OP(IEEE_Single, REG_OP(0), 2); INPUT_OP(IEEE_Single, MEMORY_OP, 2);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 2);
                             INPUT_OP(IEEE_Single, REG_OP(0), 3); INPUT_OP(IEEE_Single, MEMORY_OP, 3);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 3);
                             mi++; break;
        case XED_IFORM_SUBPS_XMMps_XMMps:
            OP_TYPE(OP_SUB); INPUT_OP(IEEE_Single, REG_OP(0), 0); INPUT_OP(IEEE_Single, REG_OP(1), 0);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 0);
                             INPUT_OP(IEEE_Single, REG_OP(0), 1); INPUT_OP(IEEE_Single, REG_OP(1), 1);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 1);
                             INPUT_OP(IEEE_Single, REG_OP(0), 2); INPUT_OP(IEEE_Single, REG_OP(1), 2);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 2);
                             INPUT_OP(IEEE_Single, REG_OP(0), 3); INPUT_OP(IEEE_Single, REG_OP(1), 3);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 3);
                             break;
        case XED_IFORM_PSUBD_XMMdq_MEMdq:
            OP_TYPE(OP_SUB); INPUT_OP(SignedInt32, REG_OP(0), 0); INPUT_OP(SignedInt32, MEMORY_OP, 0);
                             OUTPUT_OP(SignedInt32, REG_OP(0), 0);
                             INPUT_OP(SignedInt32, REG_OP(0), 1); INPUT_OP(SignedInt32, MEMORY_OP, 1);
                             OUTPUT_OP(SignedInt32, REG_OP(0), 1);
                             INPUT_OP(SignedInt32, REG_OP(0), 2); INPUT_OP(SignedInt32, MEMORY_OP, 2);
                             OUTPUT_OP(SignedInt32, REG_OP(0), 2);
                             INPUT_OP(SignedInt32, REG_OP(0), 3); INPUT_OP(SignedInt32, MEMORY_OP, 3);
                             OUTPUT_OP(SignedInt32, REG_OP(0), 3);
                             mi++; break;
        case XED_IFORM_PSUBD_XMMdq_XMMdq:
            OP_TYPE(OP_SUB); INPUT_OP(SignedInt32, REG_OP(0), 0); INPUT_OP(SignedInt32, MEMORY_OP, 0);
                             OUTPUT_OP(SignedInt32, REG_OP(0), 0);
                             INPUT_OP(SignedInt32, REG_OP(0), 1); INPUT_OP(SignedInt32, MEMORY_OP, 1);
                             OUTPUT_OP(SignedInt32, REG_OP(0), 1);
                             INPUT_OP(SignedInt32, REG_OP(0), 2); INPUT_OP(SignedInt32, MEMORY_OP, 2);
                             OUTPUT_OP(SignedInt32, REG_OP(0), 2);
                             INPUT_OP(SignedInt32, REG_OP(0), 3); INPUT_OP(SignedInt32, MEMORY_OP, 3);
                             OUTPUT_OP(SignedInt32, REG_OP(0), 3);
                             break;
        case XED_IFORM_MULPS_XMMps_MEMps:
            OP_TYPE(OP_MUL); INPUT_OP(IEEE_Single, REG_OP(0), 0); INPUT_OP(IEEE_Single, MEMORY_OP, 0);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 0);
                             INPUT_OP(IEEE_Single, REG_OP(0), 1); INPUT_OP(IEEE_Single, MEMORY_OP, 1);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 1);
                             INPUT_OP(IEEE_Single, REG_OP(0), 2); INPUT_OP(IEEE_Single, MEMORY_OP, 2);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 2);
                             INPUT_OP(IEEE_Single, REG_OP(0), 3); INPUT_OP(IEEE_Single, MEMORY_OP, 3);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 3);
                             mi++; break;
        case XED_IFORM_MULPS_XMMps_XMMps:
            OP_TYPE(OP_MUL); INPUT_OP(IEEE_Single, REG_OP(0), 0); INPUT_OP(IEEE_Single, REG_OP(1), 0);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 0);
                             INPUT_OP(IEEE_Single, REG_OP(0), 1); INPUT_OP(IEEE_Single, REG_OP(1), 1);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 1);
                             INPUT_OP(IEEE_Single, REG_OP(0), 2); INPUT_OP(IEEE_Single, REG_OP(1), 2);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 2);
                             INPUT_OP(IEEE_Single, REG_OP(0), 3); INPUT_OP(IEEE_Single, REG_OP(1), 3);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 3);
                             break;
        case XED_IFORM_DIVPS_XMMps_MEMps:
            OP_TYPE(OP_DIV); INPUT_OP(IEEE_Single, REG_OP(0), 0); INPUT_OP(IEEE_Single, MEMORY_OP, 0);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 0);
                             INPUT_OP(IEEE_Single, REG_OP(0), 1); INPUT_OP(IEEE_Single, MEMORY_OP, 1);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 1);
                             INPUT_OP(IEEE_Single, REG_OP(0), 2); INPUT_OP(IEEE_Single, MEMORY_OP, 2);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 2);
                             INPUT_OP(IEEE_Single, REG_OP(0), 3); INPUT_OP(IEEE_Single, MEMORY_OP, 3);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 3);
                             mi++; break;
        case XED_IFORM_DIVPS_XMMps_XMMps:
            OP_TYPE(OP_DIV); INPUT_OP(IEEE_Single, REG_OP(0), 0); INPUT_OP(IEEE_Single, REG_OP(1), 0);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 0);
                             INPUT_OP(IEEE_Single, REG_OP(0), 1); INPUT_OP(IEEE_Single, REG_OP(1), 1);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 1);
                             INPUT_OP(IEEE_Single, REG_OP(0), 2); INPUT_OP(IEEE_Single, REG_OP(1), 2);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 2);
                             INPUT_OP(IEEE_Single, REG_OP(0), 3); INPUT_OP(IEEE_Single, REG_OP(1), 3);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 3);
                             break;

        // }}}

        // {{{ SSE double data movement
        case XED_IFORM_MOVSD_XMM_MEMsd_XMMsd:
            OP_TYPE(OP_MOV); INPUT_OP(IEEE_Double, REG_OP(1), 0);
                             OUTPUT_OP(IEEE_Double, MEMORY_OP, 0); mi++; break;
        case XED_IFORM_MOVSD_XMM_XMMdq_MEMsd:
            OP_TYPE(OP_MOV); INPUT_OP(IEEE_Double, MEMORY_OP, 0); mi++;
                             ZERO_OP(SignedInt64, REG_OP(0), 2);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 0); break;
        case XED_IFORM_MOVSD_XMM_XMMsd_XMMsd_0F10:
        case XED_IFORM_MOVSD_XMM_XMMsd_XMMsd_0F11:
            OP_TYPE(OP_MOV); INPUT_OP(IEEE_Double, REG_OP(1), 0);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 0); break;
        case XED_IFORM_MOVDDUP_XMMdq_MEMq:
            OP_TYPE(OP_MOV); INPUT_OP(IEEE_Double, REG_OP(0), 0); OUTPUT_OP(IEEE_Double, MEMORY_OP, 0);
                             INPUT_OP(IEEE_Double, REG_OP(0), 0); OUTPUT_OP(IEEE_Double, MEMORY_OP, 2);
                             mi++; break;
        case XED_IFORM_MOVDDUP_XMMdq_XMMq:
            OP_TYPE(OP_MOV); INPUT_OP(IEEE_Double, REG_OP(1), 0); OUTPUT_OP(IEEE_Double, REG_OP(0), 0);
                             INPUT_OP(IEEE_Double, REG_OP(1), 0); OUTPUT_OP(IEEE_Double, REG_OP(0), 2);
                             break;
        case XED_IFORM_MOVAPD_MEMpd_XMMpd:
        case XED_IFORM_MOVUPD_MEMpd_XMMpd:
            OP_TYPE(OP_MOV); INPUT_OP(IEEE_Double, REG_OP(0), 0); OUTPUT_OP(IEEE_Double, MEMORY_OP, 0);
                             INPUT_OP(IEEE_Double, REG_OP(0), 2); OUTPUT_OP(IEEE_Double, MEMORY_OP, 2);
                             mi++; break;
        case XED_IFORM_MOVAPD_XMMpd_MEMpd:
        case XED_IFORM_MOVUPD_XMMpd_MEMpd:
            OP_TYPE(OP_MOV); INPUT_OP(IEEE_Double, MEMORY_OP, 0); OUTPUT_OP(IEEE_Double, REG_OP(0), 0);
                             INPUT_OP(IEEE_Double, MEMORY_OP, 2); OUTPUT_OP(IEEE_Double, REG_OP(0), 2);
                             mi++; break;
        case XED_IFORM_MOVAPD_XMMpd_XMMpd_0F28:
        case XED_IFORM_MOVAPD_XMMpd_XMMpd_0F29:
        case XED_IFORM_MOVUPD_XMMpd_XMMpd_0F10:
        case XED_IFORM_MOVUPD_XMMpd_XMMpd_0F11:
            OP_TYPE(OP_MOV); INPUT_OP(IEEE_Double, REG_OP(1), 0); OUTPUT_OP(IEEE_Double, REG_OP(0), 0);
                             INPUT_OP(IEEE_Double, REG_OP(1), 2); OUTPUT_OP(IEEE_Double, REG_OP(0), 2);
                             break;
        case XED_IFORM_MOVHPD_MEMq_XMMsd:
            OP_TYPE(OP_MOV); INPUT_OP(IEEE_Double, REG_OP(0), 2); OUTPUT_OP(IEEE_Double, MEMORY_OP, 0);
                             mi++; break;
        case XED_IFORM_MOVHPD_XMMsd_MEMq:
            OP_TYPE(OP_MOV); INPUT_OP(IEEE_Double, MEMORY_OP, 0); OUTPUT_OP(IEEE_Double, REG_OP(0), 2);
                             mi++; break;
        case XED_IFORM_MOVLPD_MEMq_XMMsd:
            OP_TYPE(OP_MOV); INPUT_OP(IEEE_Double, REG_OP(0), 0); OUTPUT_OP(IEEE_Double, MEMORY_OP, 0);
                             mi++; break;
        case XED_IFORM_MOVLPD_XMMsd_MEMq:
            OP_TYPE(OP_MOV); INPUT_OP(IEEE_Double, MEMORY_OP, 0); OUTPUT_OP(IEEE_Double, REG_OP(0), 0);
                             mi++; break;
        case XED_IFORM_UNPCKHPD_XMMpd_MEMdq:
            OP_TYPE(OP_MOV); INPUT_OP(IEEE_Double, REG_OP(0), 2); OUTPUT_OP(IEEE_Double, REG_OP(0), 0);
                             INPUT_OP(IEEE_Double, MEMORY_OP, 2); OUTPUT_OP(IEEE_Double, REG_OP(0), 2);
                             mi++; break;
        case XED_IFORM_PUNPCKHQDQ_XMMdq_MEMdq:
            OP_TYPE(OP_MOV); INPUT_OP(SignedInt64, REG_OP(0), 2); OUTPUT_OP(SignedInt64, REG_OP(0), 0);
                             INPUT_OP(SignedInt64, MEMORY_OP, 2); OUTPUT_OP(SignedInt64, REG_OP(0), 2);
                             mi++; break;
        case XED_IFORM_UNPCKHPD_XMMpd_XMMq:
            OP_TYPE(OP_MOV); INPUT_OP(IEEE_Double, REG_OP(0), 2); OUTPUT_OP(IEEE_Double, REG_OP(0), 0);
                             INPUT_OP(IEEE_Double, REG_OP(1), 2); OUTPUT_OP(IEEE_Double, REG_OP(0), 2);
                             break;
        case XED_IFORM_PUNPCKHQDQ_XMMdq_XMMq:
            OP_TYPE(OP_MOV); INPUT_OP(SignedInt64, REG_OP(0), 2); OUTPUT_OP(SignedInt64, REG_OP(0), 0);
                             INPUT_OP(SignedInt64, REG_OP(1), 2); OUTPUT_OP(SignedInt64, REG_OP(0), 2);
                             break;
        case XED_IFORM_UNPCKLPD_XMMpd_MEMdq:
            OP_TYPE(OP_MOV); INPUT_OP(IEEE_Double, REG_OP(0), 0); OUTPUT_OP(IEEE_Double, REG_OP(0), 0);
                             INPUT_OP(IEEE_Double, MEMORY_OP, 0); OUTPUT_OP(IEEE_Double, REG_OP(0), 2);
                             mi++; break;
        case XED_IFORM_PUNPCKLQDQ_XMMdq_MEMdq:
            OP_TYPE(OP_MOV); INPUT_OP(SignedInt64, REG_OP(0), 0); OUTPUT_OP(SignedInt64, REG_OP(0), 0);
                             INPUT_OP(SignedInt64, MEMORY_OP, 0); OUTPUT_OP(SignedInt64, REG_OP(0), 2);
                             mi++; break;
        case XED_IFORM_UNPCKLPD_XMMpd_XMMq:
            OP_TYPE(OP_MOV); INPUT_OP(IEEE_Double, REG_OP(0), 0); OUTPUT_OP(IEEE_Double, REG_OP(0), 0);
                             INPUT_OP(IEEE_Double, REG_OP(1), 0); OUTPUT_OP(IEEE_Double, REG_OP(0), 2);
                             break;
        case XED_IFORM_PUNPCKLQDQ_XMMdq_XMMq:
            OP_TYPE(OP_MOV); INPUT_OP(SignedInt64, REG_OP(0), 0); OUTPUT_OP(SignedInt64, REG_OP(0), 0);
                             INPUT_OP(SignedInt64, REG_OP(1), 0); OUTPUT_OP(SignedInt64, REG_OP(0), 2);
                             break;
        case XED_IFORM_MOVQ_MEMq_XMMq_0F7E:
        case XED_IFORM_MOVQ_MEMq_XMMq_0FD6:
            OP_TYPE(OP_MOV); INPUT_OP(IEEE_Double, REG_OP(0), 0); OUTPUT_OP(IEEE_Double, MEMORY_OP, 0);
                             mi++; break;
        case XED_IFORM_MOVQ_XMMdq_MEMq_0F6E:
        case XED_IFORM_MOVQ_XMMdq_MEMq_0F7E:
            OP_TYPE(OP_MOV); INPUT_OP(IEEE_Double, MEMORY_OP, 0); mi++;
                             ZERO_OP(SignedInt64, REG_OP(0), 2);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 0); break;
        case XED_IFORM_MOVQ_XMMdq_XMMq_0F7E:
        case XED_IFORM_MOVQ_XMMdq_XMMq_0FD6:
            OP_TYPE(OP_MOV); INPUT_OP(IEEE_Double, REG_OP(1), 0);
                             ZERO_OP(SignedInt64, REG_OP(0), 2);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 0); break;
        //case XED_IFORM_MOVD_MEMd_MMXd:
        //case XED_IFORM_MOVD_MMXq_MEMd:
        case XED_IFORM_MOVD_GPR32_XMMd:
        case XED_IFORM_MOVD_XMMdq_GPR32:
            OP_TYPE(OP_MOV); INPUT_OP(IEEE_Single, REG_OP(1), 0);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 0); break;
        case XED_IFORM_MOVD_MEMd_XMMd:
            OP_TYPE(OP_MOV); INPUT_OP(IEEE_Single, REG_OP(0), 0);
                             OUTPUT_OP(IEEE_Single, MEMORY_OP, 0); mi++; break;
        case XED_IFORM_MOVD_XMMdq_MEMd:
            OP_TYPE(OP_MOV); INPUT_OP(IEEE_Single, MEMORY_OP, 0);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 0); mi++; break;
        case XED_IFORM_MOVDQA_MEMdq_XMMdq:
        case XED_IFORM_MOVDQU_MEMdq_XMMdq:
            OP_TYPE(OP_MOV); INPUT_OP(SSE_Quad, REG_OP(0), 0);
                             OUTPUT_OP(SSE_Quad, MEMORY_OP, 0); mi++; break;
        case XED_IFORM_MOVDQA_XMMdq_MEMdq:
        case XED_IFORM_MOVDQU_XMMdq_MEMdq:
            OP_TYPE(OP_MOV); INPUT_OP(SSE_Quad, MEMORY_OP, 0); mi++;
                             OUTPUT_OP(SSE_Quad, REG_OP(0), 0); break;
        case XED_IFORM_MOVDQA_XMMdq_XMMdq_0F6F:
        case XED_IFORM_MOVDQA_XMMdq_XMMdq_0F7F:
        case XED_IFORM_MOVDQU_XMMdq_XMMdq_0F6F:
        case XED_IFORM_MOVDQU_XMMdq_XMMdq_0F7F:
            OP_TYPE(OP_MOV); INPUT_OP(SSE_Quad, REG_OP(1), 0);
                             OUTPUT_OP(SSE_Quad, REG_OP(0), 0); break;

        // SSE shuffle; ignore for now b/c we don't have an analysis that cares
        // TODO: fix this at some point (probably for pointer-based analysis)
        case XED_IFORM_SHUFPD_XMMpd_MEMpd_IMMb:
        case XED_IFORM_SHUFPD_XMMpd_XMMpd_IMMb:
            OP_TYPE(OP_NONE); break;


        // }}}

        // {{{ SSE double arithmetic
        case XED_IFORM_ADDSD_XMMsd_MEMsd:
            OP_TYPE(OP_ADD); INPUT_OP(IEEE_Double, REG_OP(0), 0); INPUT_OP(IEEE_Double, MEMORY_OP, 0); mi++;
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 0); break;
        case XED_IFORM_ADDSD_XMMsd_XMMsd:
            OP_TYPE(OP_ADD); INPUT_OP(IEEE_Double, REG_OP(0), 0); INPUT_OP(IEEE_Double, REG_OP(1), 0);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 0); break;
        case XED_IFORM_SUBSD_XMMsd_MEMsd:
            OP_TYPE(OP_SUB); INPUT_OP(IEEE_Double, REG_OP(0), 0); INPUT_OP(IEEE_Double, MEMORY_OP, 0); mi++;
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 0); break;
        case XED_IFORM_SUBSD_XMMsd_XMMsd:
            OP_TYPE(OP_SUB); INPUT_OP(IEEE_Double, REG_OP(0), 0); INPUT_OP(IEEE_Double, REG_OP(1), 0);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 0); break;
        case XED_IFORM_MULSD_XMMsd_MEMsd:
            OP_TYPE(OP_MUL); INPUT_OP(IEEE_Double, REG_OP(0), 0); INPUT_OP(IEEE_Double, MEMORY_OP, 0); mi++;
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 0); break;
        case XED_IFORM_MULSD_XMMsd_XMMsd:
            OP_TYPE(OP_MUL); INPUT_OP(IEEE_Double, REG_OP(0), 0); INPUT_OP(IEEE_Double, REG_OP(1), 0);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 0); break;
        case XED_IFORM_DIVSD_XMMsd_MEMsd:
            OP_TYPE(OP_DIV); INPUT_OP(IEEE_Double, REG_OP(0), 0); INPUT_OP(IEEE_Double, MEMORY_OP, 0); mi++;
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 0); break;
        case XED_IFORM_DIVSD_XMMsd_XMMsd:
            OP_TYPE(OP_DIV); INPUT_OP(IEEE_Double, REG_OP(0), 0); INPUT_OP(IEEE_Double, REG_OP(1), 0);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 0); break;

        case XED_IFORM_ADDPD_XMMpd_MEMpd:
            OP_TYPE(OP_ADD); INPUT_OP(IEEE_Double, REG_OP(0), 0); INPUT_OP(IEEE_Double, MEMORY_OP, 0);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 0);
                             INPUT_OP(IEEE_Double, REG_OP(0), 1); INPUT_OP(IEEE_Double, MEMORY_OP, 1);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 1);
                             mi++; break;
        case XED_IFORM_ADDPD_XMMpd_XMMpd:
            OP_TYPE(OP_ADD); INPUT_OP(IEEE_Double, REG_OP(0), 0); INPUT_OP(IEEE_Double, REG_OP(1), 0);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 0);
                             INPUT_OP(IEEE_Double, REG_OP(0), 1); INPUT_OP(IEEE_Double, REG_OP(1), 1);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 1);
                             break;
        case XED_IFORM_SUBPD_XMMpd_MEMpd:
            OP_TYPE(OP_SUB); INPUT_OP(IEEE_Double, REG_OP(0), 0); INPUT_OP(IEEE_Double, MEMORY_OP, 0);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 0);
                             INPUT_OP(IEEE_Double, REG_OP(0), 1); INPUT_OP(IEEE_Double, MEMORY_OP, 1);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 1);
                             mi++; break;
        case XED_IFORM_SUBPD_XMMpd_XMMpd:
            OP_TYPE(OP_SUB); INPUT_OP(IEEE_Double, REG_OP(0), 0); INPUT_OP(IEEE_Double, REG_OP(1), 0);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 0);
                             INPUT_OP(IEEE_Double, REG_OP(0), 1); INPUT_OP(IEEE_Double, REG_OP(1), 1);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 1);
                             break;
        case XED_IFORM_PSUBQ_XMMdq_MEMdq:
            OP_TYPE(OP_SUB); INPUT_OP(SignedInt64, REG_OP(0), 0); INPUT_OP(SignedInt64, MEMORY_OP, 0);
                             OUTPUT_OP(SignedInt64, REG_OP(0), 0);
                             INPUT_OP(SignedInt64, REG_OP(0), 1); INPUT_OP(SignedInt64, MEMORY_OP, 1);
                             OUTPUT_OP(SignedInt64, REG_OP(0), 1);
                             mi++; break;
        case XED_IFORM_PSUBQ_XMMdq_XMMdq:
            OP_TYPE(OP_SUB); INPUT_OP(SignedInt64, REG_OP(0), 0); INPUT_OP(SignedInt64, MEMORY_OP, 0);
                             OUTPUT_OP(SignedInt64, REG_OP(0), 0);
                             INPUT_OP(SignedInt64, REG_OP(0), 1); INPUT_OP(SignedInt64, MEMORY_OP, 1);
                             OUTPUT_OP(SignedInt64, REG_OP(0), 1);
                             INPUT_OP(SignedInt64, REG_OP(0), 2); INPUT_OP(SignedInt64, MEMORY_OP, 2);
                             break;
        case XED_IFORM_MULPD_XMMpd_MEMpd:
            OP_TYPE(OP_MUL); INPUT_OP(IEEE_Double, REG_OP(0), 0); INPUT_OP(IEEE_Double, MEMORY_OP, 0);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 0);
                             INPUT_OP(IEEE_Double, REG_OP(0), 1); INPUT_OP(IEEE_Double, MEMORY_OP, 1);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 1);
                             mi++; break;
        case XED_IFORM_MULPD_XMMpd_XMMpd:
            OP_TYPE(OP_MUL); INPUT_OP(IEEE_Double, REG_OP(0), 0); INPUT_OP(IEEE_Double, REG_OP(1), 0);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 0);
                             INPUT_OP(IEEE_Double, REG_OP(0), 1); INPUT_OP(IEEE_Double, REG_OP(1), 1);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 1);
                             break;
        case XED_IFORM_DIVPD_XMMpd_MEMpd:
            OP_TYPE(OP_DIV); INPUT_OP(IEEE_Double, REG_OP(0), 0); INPUT_OP(IEEE_Double, MEMORY_OP, 0);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 0);
                             INPUT_OP(IEEE_Double, REG_OP(0), 1); INPUT_OP(IEEE_Double, MEMORY_OP, 1);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 1);
                             mi++; break;
        case XED_IFORM_DIVPD_XMMpd_XMMpd:
            OP_TYPE(OP_DIV); INPUT_OP(IEEE_Double, REG_OP(0), 0); INPUT_OP(IEEE_Double, REG_OP(1), 0);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 0);
                             INPUT_OP(IEEE_Double, REG_OP(0), 1); INPUT_OP(IEEE_Double, REG_OP(1), 1);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 1);
                             break;

        // }}}

        // {{{ SSE bitwise operations
        case XED_IFORM_ANDPD_XMMpd_MEMpd:
            OP_TYPE(OP_AND); INPUT_OP(IEEE_Double, REG_OP(0), 0); INPUT_OP(IEEE_Double, MEMORY_OP, 0);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 0);
                             INPUT_OP(IEEE_Double, REG_OP(0), 2); INPUT_OP(IEEE_Double, MEMORY_OP, 2);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 2);
                             mi++; break;
        case XED_IFORM_ANDPD_XMMpd_XMMpd:
            OP_TYPE(OP_AND); INPUT_OP(IEEE_Double, REG_OP(0), 0); INPUT_OP(IEEE_Double, REG_OP(1), 0);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 0);
                             INPUT_OP(IEEE_Double, REG_OP(0), 2); INPUT_OP(IEEE_Double, REG_OP(1), 2);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 2);
                             break;
        case XED_IFORM_ANDPS_XMMps_MEMps:
            OP_TYPE(OP_AND); INPUT_OP(SSE_Quad, REG_OP(0), 0); INPUT_OP(SSE_Quad, MEMORY_OP, 0);
                             OUTPUT_OP(SSE_Quad, REG_OP(0), 0);
                             mi++; break;
            /*
             *OP_TYPE(OP_AND); INPUT_OP(IEEE_Single, REG_OP(0), 0); INPUT_OP(IEEE_Single, MEMORY_OP, 0);
             *                 OUTPUT_OP(IEEE_Single, REG_OP(0), 0);
             *                 INPUT_OP(IEEE_Single, REG_OP(0), 1); INPUT_OP(IEEE_Single, MEMORY_OP, 1);
             *                 OUTPUT_OP(IEEE_Single, REG_OP(0), 1);
             *                 INPUT_OP(IEEE_Single, REG_OP(0), 2); INPUT_OP(IEEE_Single, MEMORY_OP, 2);
             *                 OUTPUT_OP(IEEE_Single, REG_OP(0), 2);
             *                 INPUT_OP(IEEE_Single, REG_OP(0), 3); INPUT_OP(IEEE_Single, MEMORY_OP, 3);
             *                 OUTPUT_OP(IEEE_Single, REG_OP(0), 3);
             *                 mi++; break;
             */
        case XED_IFORM_ANDPS_XMMps_XMMps:
            OP_TYPE(OP_AND); INPUT_OP(SSE_Quad, REG_OP(0), 0); INPUT_OP(SSE_Quad, REG_OP(1), 0);
                             OUTPUT_OP(SSE_Quad, REG_OP(0), 0);
                             break;
            /*
             *OP_TYPE(OP_AND); INPUT_OP(IEEE_Single, REG_OP(0), 0); INPUT_OP(IEEE_Single, REG_OP(1), 0);
             *                 OUTPUT_OP(IEEE_Single, REG_OP(0), 0);
             *                 INPUT_OP(IEEE_Single, REG_OP(0), 1); INPUT_OP(IEEE_Single, REG_OP(1), 1);
             *                 OUTPUT_OP(IEEE_Single, REG_OP(0), 1);
             *                 INPUT_OP(IEEE_Single, REG_OP(0), 2); INPUT_OP(IEEE_Single, REG_OP(1), 2);
             *                 OUTPUT_OP(IEEE_Single, REG_OP(0), 2);
             *                 INPUT_OP(IEEE_Single, REG_OP(0), 3); INPUT_OP(IEEE_Single, REG_OP(1), 3);
             *                 OUTPUT_OP(IEEE_Single, REG_OP(0), 3);
             *                 break;
             */
        case XED_IFORM_PAND_XMMdq_MEMdq:
            OP_TYPE(OP_AND); INPUT_OP(SSE_Quad, REG_OP(0), 0); INPUT_OP(SSE_Quad, MEMORY_OP, 0);
                             OUTPUT_OP(SSE_Quad, REG_OP(0), 0);
                             mi++; break;
        case XED_IFORM_PAND_XMMdq_XMMdq:
            OP_TYPE(OP_AND); INPUT_OP(SSE_Quad, REG_OP(0), 0); INPUT_OP(SSE_Quad, REG_OP(1), 0);
                             OUTPUT_OP(SSE_Quad, REG_OP(0), 0);
                             break;
        case XED_IFORM_ANDNPD_XMMpd_MEMpd:
            OP_TYPE(OP_AND); temp_opr = new FPOperand(IEEE_Double, REG_OP(0), 0);
                             temp_opr->setInverted(true);
                             operation->addInputOperand(temp_opr);
                             INPUT_OP(IEEE_Double, MEMORY_OP, 0);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 0);
                             temp_opr = new FPOperand(IEEE_Double, REG_OP(0), 2);
                             temp_opr->setInverted(true);
                             operation->addInputOperand(temp_opr);
                             INPUT_OP(IEEE_Double, MEMORY_OP, 2);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 2);
                             mi++; break;
        case XED_IFORM_ANDNPD_XMMpd_XMMpd:
            OP_TYPE(OP_AND); temp_opr = new FPOperand(IEEE_Double, REG_OP(0), 0);
                             temp_opr->setInverted(true);
                             operation->addInputOperand(temp_opr);
                             INPUT_OP(IEEE_Double, REG_OP(1), 0);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 0);
                             temp_opr = new FPOperand(IEEE_Double, REG_OP(0), 2);
                             temp_opr->setInverted(true);
                             operation->addInputOperand(temp_opr);
                             INPUT_OP(IEEE_Double, REG_OP(1), 2);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 2);
                             break;
        case XED_IFORM_ANDNPS_XMMps_MEMps:
            OP_TYPE(OP_AND); temp_opr = new FPOperand(IEEE_Single, REG_OP(0), 0);
                             temp_opr->setInverted(true);
                             operation->addInputOperand(temp_opr);
                             INPUT_OP(IEEE_Single, MEMORY_OP, 0);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 0);
                             temp_opr = new FPOperand(IEEE_Single, REG_OP(0), 1);
                             temp_opr->setInverted(true);
                             operation->addInputOperand(temp_opr);
                             INPUT_OP(IEEE_Single, MEMORY_OP, 1);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 1);
                             temp_opr = new FPOperand(IEEE_Single, REG_OP(0), 2);
                             temp_opr->setInverted(true);
                             operation->addInputOperand(temp_opr);
                             INPUT_OP(IEEE_Single, MEMORY_OP, 2);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 2);
                             temp_opr = new FPOperand(IEEE_Single, REG_OP(0), 3);
                             temp_opr->setInverted(true);
                             operation->addInputOperand(temp_opr);
                             INPUT_OP(IEEE_Single, MEMORY_OP, 3);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 3);
                             mi++; break;
        case XED_IFORM_ANDNPS_XMMps_XMMps:
            OP_TYPE(OP_AND); temp_opr = new FPOperand(IEEE_Single, REG_OP(0), 0);
                             temp_opr->setInverted(true);
                             operation->addInputOperand(temp_opr);
                             INPUT_OP(IEEE_Single, REG_OP(1), 0);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 0);
                             temp_opr = new FPOperand(IEEE_Single, REG_OP(0), 1);
                             temp_opr->setInverted(true);
                             operation->addInputOperand(temp_opr);
                             INPUT_OP(IEEE_Single, REG_OP(1), 1);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 1);
                             temp_opr = new FPOperand(IEEE_Single, REG_OP(0), 2);
                             temp_opr->setInverted(true);
                             operation->addInputOperand(temp_opr);
                             INPUT_OP(IEEE_Single, REG_OP(1), 2);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 2);
                             temp_opr = new FPOperand(IEEE_Single, REG_OP(0), 3);
                             temp_opr->setInverted(true);
                             operation->addInputOperand(temp_opr);
                             INPUT_OP(IEEE_Single, REG_OP(1), 3);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 3);
                             break;
        case XED_IFORM_PANDN_XMMdq_MEMdq:
            OP_TYPE(OP_AND); temp_opr = new FPOperand(SSE_Quad, REG_OP(0), 0);
                             temp_opr->setInverted(true);
                             operation->addInputOperand(temp_opr);
                             INPUT_OP(SSE_Quad, MEMORY_OP, 0);
                             OUTPUT_OP(SSE_Quad, REG_OP(0), 0);
                             mi++; break;
        case XED_IFORM_PANDN_XMMdq_XMMdq:
            OP_TYPE(OP_AND); temp_opr = new FPOperand(SSE_Quad, REG_OP(0), 0);
                             temp_opr->setInverted(true);
                             operation->addInputOperand(temp_opr);
                             INPUT_OP(SSE_Quad, REG_OP(1), 0);
                             OUTPUT_OP(SSE_Quad, REG_OP(0), 0);
                             break;
        case XED_IFORM_ORPD_XMMpd_MEMpd:
            OP_TYPE(OP_OR); INPUT_OP(IEEE_Double, REG_OP(0), 0); INPUT_OP(IEEE_Double, MEMORY_OP, 0);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 0);
                             INPUT_OP(IEEE_Double, REG_OP(0), 2); INPUT_OP(IEEE_Double, MEMORY_OP, 2);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 2);
                             mi++; break;
        case XED_IFORM_ORPD_XMMpd_XMMpd:
            OP_TYPE(OP_OR); INPUT_OP(IEEE_Double, REG_OP(0), 0); INPUT_OP(IEEE_Double, REG_OP(1), 0);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 0);
                             INPUT_OP(IEEE_Double, REG_OP(0), 2); INPUT_OP(IEEE_Double, REG_OP(1), 2);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 2);
                             break;
        case XED_IFORM_ORPS_XMMps_MEMps:
            OP_TYPE(OP_OR); INPUT_OP(IEEE_Single, REG_OP(0), 0); INPUT_OP(IEEE_Single, MEMORY_OP, 0);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 0);
                             INPUT_OP(IEEE_Single, REG_OP(0), 1); INPUT_OP(IEEE_Single, MEMORY_OP, 1);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 1);
                             INPUT_OP(IEEE_Single, REG_OP(0), 2); INPUT_OP(IEEE_Single, MEMORY_OP, 2);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 2);
                             INPUT_OP(IEEE_Single, REG_OP(0), 3); INPUT_OP(IEEE_Single, MEMORY_OP, 3);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 3);
                             mi++; break;
        case XED_IFORM_ORPS_XMMps_XMMps:
            OP_TYPE(OP_OR); INPUT_OP(IEEE_Single, REG_OP(0), 0); INPUT_OP(IEEE_Single, REG_OP(1), 0);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 0);
                             INPUT_OP(IEEE_Single, REG_OP(0), 1); INPUT_OP(IEEE_Single, REG_OP(1), 1);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 1);
                             INPUT_OP(IEEE_Single, REG_OP(0), 2); INPUT_OP(IEEE_Single, REG_OP(1), 2);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 2);
                             INPUT_OP(IEEE_Single, REG_OP(0), 3); INPUT_OP(IEEE_Single, REG_OP(1), 3);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 3);
                             break;
        case XED_IFORM_POR_XMMdq_MEMdq:
            OP_TYPE(OP_OR); INPUT_OP(SSE_Quad, REG_OP(0), 0); INPUT_OP(SSE_Quad, MEMORY_OP, 0);
                             OUTPUT_OP(SSE_Quad, REG_OP(0), 0);
                             mi++; break;
        case XED_IFORM_POR_XMMdq_XMMdq:
            OP_TYPE(OP_OR); INPUT_OP(SSE_Quad, REG_OP(0), 0); INPUT_OP(SSE_Quad, REG_OP(1), 0);
                             OUTPUT_OP(SSE_Quad, REG_OP(0), 0);
                             break;
        case XED_IFORM_XORPD_XMMpd_MEMpd:
            OP_TYPE(OP_XOR); INPUT_OP(IEEE_Double, REG_OP(0), 0); INPUT_OP(IEEE_Double, MEMORY_OP, 0);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 0);
                             INPUT_OP(IEEE_Double, REG_OP(0), 2); INPUT_OP(IEEE_Double, MEMORY_OP, 2);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 2);
                             mi++; break;
        case XED_IFORM_XORPD_XMMpd_XMMpd:
            OP_TYPE(OP_XOR); INPUT_OP(IEEE_Double, REG_OP(0), 0); INPUT_OP(IEEE_Double, REG_OP(1), 0);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 0);
                             INPUT_OP(IEEE_Double, REG_OP(0), 2); INPUT_OP(IEEE_Double, REG_OP(1), 2);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 2);
                             break;
        case XED_IFORM_XORPS_XMMps_MEMps:
            OP_TYPE(OP_XOR); INPUT_OP(IEEE_Single, REG_OP(0), 0); INPUT_OP(IEEE_Single, MEMORY_OP, 0);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 0);
                             INPUT_OP(IEEE_Single, REG_OP(0), 1); INPUT_OP(IEEE_Single, MEMORY_OP, 1);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 1);
                             INPUT_OP(IEEE_Single, REG_OP(0), 2); INPUT_OP(IEEE_Single, MEMORY_OP, 2);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 2);
                             INPUT_OP(IEEE_Single, REG_OP(0), 3); INPUT_OP(IEEE_Single, MEMORY_OP, 3);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 3);
                             mi++; break;
        case XED_IFORM_XORPS_XMMps_XMMps:
            OP_TYPE(OP_XOR); INPUT_OP(IEEE_Single, REG_OP(0), 0); INPUT_OP(IEEE_Single, REG_OP(1), 0);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 0);
                             INPUT_OP(IEEE_Single, REG_OP(0), 1); INPUT_OP(IEEE_Single, REG_OP(1), 1);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 1);
                             INPUT_OP(IEEE_Single, REG_OP(0), 2); INPUT_OP(IEEE_Single, REG_OP(1), 2);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 2);
                             INPUT_OP(IEEE_Single, REG_OP(0), 3); INPUT_OP(IEEE_Single, REG_OP(1), 3);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 3);
                             break;
        case XED_IFORM_PXOR_XMMdq_MEMdq:
            OP_TYPE(OP_XOR); INPUT_OP(SSE_Quad, REG_OP(0), 0); INPUT_OP(SSE_Quad, MEMORY_OP, 0);
                             OUTPUT_OP(SSE_Quad, REG_OP(0), 0);
                             mi++; break;
        case XED_IFORM_PXOR_XMMdq_XMMdq:
            OP_TYPE(OP_XOR); INPUT_OP(SSE_Quad, REG_OP(0), 0); INPUT_OP(SSE_Quad, REG_OP(1), 0);
                             OUTPUT_OP(SSE_Quad, REG_OP(0), 0);
                             break;
        // }}}

        // {{{ SSE misc operations
        case XED_IFORM_SQRTSD_XMMsd_MEMsd:
            OP_TYPE(OP_SQRT); INPUT_OP(IEEE_Double, MEMORY_OP, 0); OUTPUT_OP(IEEE_Double, REG_OP(0), 0);
                              mi++; break;
        case XED_IFORM_SQRTSD_XMMsd_XMMsd:
            OP_TYPE(OP_SQRT); INPUT_OP(IEEE_Double, REG_OP(1), 0); OUTPUT_OP(IEEE_Double, REG_OP(0), 0);
                              break;
        case XED_IFORM_SQRTPD_XMMpd_MEMpd:
            OP_TYPE(OP_SQRT); INPUT_OP(IEEE_Double, MEMORY_OP, 0); OUTPUT_OP(IEEE_Double, REG_OP(0), 0);
                              INPUT_OP(IEEE_Double, MEMORY_OP, 2); OUTPUT_OP(IEEE_Double, REG_OP(0), 2);
                              mi++; break;
        case XED_IFORM_SQRTPD_XMMpd_XMMpd:
            OP_TYPE(OP_SQRT); INPUT_OP(IEEE_Double, REG_OP(1), 0); OUTPUT_OP(IEEE_Double, REG_OP(0), 0);
                              INPUT_OP(IEEE_Double, REG_OP(1), 2); OUTPUT_OP(IEEE_Double, REG_OP(0), 2);
                              break;
        case XED_IFORM_SQRTSS_XMMss_MEMss:
            OP_TYPE(OP_SQRT); INPUT_OP(IEEE_Single, MEMORY_OP, 0); OUTPUT_OP(IEEE_Single, REG_OP(0), 0);
                              mi++; break;
        case XED_IFORM_SQRTSS_XMMss_XMMss:
            OP_TYPE(OP_SQRT); INPUT_OP(IEEE_Single, REG_OP(1), 0); OUTPUT_OP(IEEE_Single, REG_OP(0), 0);
                              break;
        case XED_IFORM_SQRTPS_XMMps_MEMps:
            OP_TYPE(OP_SQRT); INPUT_OP(IEEE_Single, MEMORY_OP, 0); OUTPUT_OP(IEEE_Single, REG_OP(0), 0);
                              INPUT_OP(IEEE_Single, MEMORY_OP, 1); OUTPUT_OP(IEEE_Single, REG_OP(0), 1);
                              INPUT_OP(IEEE_Single, MEMORY_OP, 2); OUTPUT_OP(IEEE_Single, REG_OP(0), 2);
                              INPUT_OP(IEEE_Single, MEMORY_OP, 3); OUTPUT_OP(IEEE_Single, REG_OP(0), 3);
                              mi++; break;
        case XED_IFORM_SQRTPS_XMMps_XMMps:
            OP_TYPE(OP_SQRT); INPUT_OP(IEEE_Single, REG_OP(1), 0); OUTPUT_OP(IEEE_Single, REG_OP(0), 0);
                              INPUT_OP(IEEE_Single, REG_OP(1), 1); OUTPUT_OP(IEEE_Single, REG_OP(0), 1);
                              INPUT_OP(IEEE_Single, REG_OP(1), 2); OUTPUT_OP(IEEE_Single, REG_OP(0), 2);
                              INPUT_OP(IEEE_Single, REG_OP(1), 3); OUTPUT_OP(IEEE_Single, REG_OP(0), 3);
                              break;
        case XED_IFORM_MAXSD_XMMsd_MEMsd:
            OP_TYPE(OP_MAX); INPUT_OP (IEEE_Double, REG_OP(0), 0);  INPUT_OP(IEEE_Double, MEMORY_OP, 0);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 0); mi++; break;
        case XED_IFORM_MAXSD_XMMsd_XMMsd:
            OP_TYPE(OP_MAX); INPUT_OP (IEEE_Double, REG_OP(0), 0);  INPUT_OP(IEEE_Double, REG_OP(1), 0);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 0); break;
        case XED_IFORM_MAXPD_XMMpd_MEMpd:
            OP_TYPE(OP_MAX); INPUT_OP (IEEE_Double, REG_OP(0), 0);  INPUT_OP(IEEE_Double, MEMORY_OP, 0);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 0);
                             INPUT_OP (IEEE_Double, REG_OP(0), 2);  INPUT_OP(IEEE_Double, MEMORY_OP, 2);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 2); mi++; break;
        case XED_IFORM_MAXPD_XMMpd_XMMpd:
            OP_TYPE(OP_MAX); INPUT_OP (IEEE_Double, REG_OP(0), 0);  INPUT_OP(IEEE_Double, REG_OP(1), 0);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 0);
                             INPUT_OP (IEEE_Double, REG_OP(0), 2);  INPUT_OP(IEEE_Double, REG_OP(1), 2);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 2); break;
        case XED_IFORM_MAXSS_XMMss_MEMss:
            OP_TYPE(OP_MAX); INPUT_OP (IEEE_Single, REG_OP(0), 0);  INPUT_OP(IEEE_Single, MEMORY_OP, 0);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 0); mi++; break;
        case XED_IFORM_MAXSS_XMMss_XMMss:
            OP_TYPE(OP_MAX); INPUT_OP (IEEE_Single, REG_OP(0), 0);  INPUT_OP(IEEE_Single, REG_OP(1), 0);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 0); break;
        case XED_IFORM_MAXPS_XMMps_MEMps:
            OP_TYPE(OP_MAX); INPUT_OP (IEEE_Single, REG_OP(0), 0);  INPUT_OP(IEEE_Single, MEMORY_OP, 0);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 0);
                             INPUT_OP (IEEE_Single, REG_OP(0), 1);  INPUT_OP(IEEE_Single, MEMORY_OP, 1);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 1);
                             INPUT_OP (IEEE_Single, REG_OP(0), 2);  INPUT_OP(IEEE_Single, MEMORY_OP, 2);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 2);
                             INPUT_OP (IEEE_Single, REG_OP(0), 3);  INPUT_OP(IEEE_Single, MEMORY_OP, 3);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 3); mi++; break;
        case XED_IFORM_MAXPS_XMMps_XMMps:
            OP_TYPE(OP_MAX); INPUT_OP (IEEE_Single, REG_OP(0), 0);  INPUT_OP(IEEE_Single, REG_OP(1), 0);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 0);
                             INPUT_OP (IEEE_Single, REG_OP(0), 1);  INPUT_OP(IEEE_Single, REG_OP(1), 1);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 1);
                             INPUT_OP (IEEE_Single, REG_OP(0), 2);  INPUT_OP(IEEE_Single, REG_OP(1), 2);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 2);
                             INPUT_OP (IEEE_Single, REG_OP(0), 3);  INPUT_OP(IEEE_Single, REG_OP(1), 3);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 3); break;
        case XED_IFORM_MINSD_XMMsd_MEMsd:
            OP_TYPE(OP_MIN); INPUT_OP (IEEE_Double, REG_OP(0), 0);  INPUT_OP(IEEE_Double, MEMORY_OP, 0);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 0); mi++; break;
        case XED_IFORM_MINSD_XMMsd_XMMsd:
            OP_TYPE(OP_MIN); INPUT_OP (IEEE_Double, REG_OP(0), 0);  INPUT_OP(IEEE_Double, REG_OP(1), 0);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 0); break;
        case XED_IFORM_MINPD_XMMpd_MEMpd:
            OP_TYPE(OP_MIN); INPUT_OP (IEEE_Double, REG_OP(0), 0);  INPUT_OP(IEEE_Double, MEMORY_OP, 0);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 0);
                             INPUT_OP (IEEE_Double, REG_OP(0), 2);  INPUT_OP(IEEE_Double, MEMORY_OP, 2);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 2); mi++; break;
        case XED_IFORM_MINPD_XMMpd_XMMpd:
            OP_TYPE(OP_MIN); INPUT_OP (IEEE_Double, REG_OP(0), 0);  INPUT_OP(IEEE_Double, REG_OP(1), 0);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 0);
                             INPUT_OP (IEEE_Double, REG_OP(0), 2);  INPUT_OP(IEEE_Double, REG_OP(1), 2);
                             OUTPUT_OP(IEEE_Double, REG_OP(0), 2); break;
        case XED_IFORM_MINSS_XMMss_MEMss:
            OP_TYPE(OP_MIN); INPUT_OP (IEEE_Single, REG_OP(0), 0);  INPUT_OP(IEEE_Single, MEMORY_OP, 0);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 0); mi++; break;
        case XED_IFORM_MINSS_XMMss_XMMss:
            OP_TYPE(OP_MIN); INPUT_OP (IEEE_Single, REG_OP(0), 0);  INPUT_OP(IEEE_Single, REG_OP(1), 0);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 0); break;
        case XED_IFORM_MINPS_XMMps_MEMps:
            OP_TYPE(OP_MIN); INPUT_OP (IEEE_Single, REG_OP(0), 0);  INPUT_OP(IEEE_Single, MEMORY_OP, 0);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 0);
                             INPUT_OP (IEEE_Single, REG_OP(0), 1);  INPUT_OP(IEEE_Single, MEMORY_OP, 1);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 1);
                             INPUT_OP (IEEE_Single, REG_OP(0), 2);  INPUT_OP(IEEE_Single, MEMORY_OP, 2);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 2);
                             INPUT_OP (IEEE_Single, REG_OP(0), 3);  INPUT_OP(IEEE_Single, MEMORY_OP, 3);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 3); mi++; break;
        case XED_IFORM_MINPS_XMMps_XMMps:
            OP_TYPE(OP_MIN); INPUT_OP (IEEE_Single, REG_OP(0), 0);  INPUT_OP(IEEE_Single, REG_OP(1), 0);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 0);
                             INPUT_OP (IEEE_Single, REG_OP(0), 1);  INPUT_OP(IEEE_Single, REG_OP(1), 1);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 1);
                             INPUT_OP (IEEE_Single, REG_OP(0), 2);  INPUT_OP(IEEE_Single, REG_OP(1), 2);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 2);
                             INPUT_OP (IEEE_Single, REG_OP(0), 3);  INPUT_OP(IEEE_Single, REG_OP(1), 3);
                             OUTPUT_OP(IEEE_Single, REG_OP(0), 3); break;
        // }}}

        // {{{ SSE conversions (ignore MMX instructors for now...)
        case XED_IFORM_CVTDQ2PD_XMMpd_MEMq:
            OP_TYPE(OP_CVT); INPUT_OP(SignedInt32, MEMORY_OP, 0); OUTPUT_OP(IEEE_Double, REG_OP(0), 0);
                             INPUT_OP(SignedInt32, MEMORY_OP, 1); OUTPUT_OP(IEEE_Double, REG_OP(0), 2);
                             mi++; break;
        case XED_IFORM_CVTDQ2PD_XMMpd_XMMq:
            OP_TYPE(OP_CVT); INPUT_OP(SignedInt32, REG_OP(1), 0); OUTPUT_OP(IEEE_Double, REG_OP(0), 0);
                             INPUT_OP(SignedInt32, REG_OP(1), 1); OUTPUT_OP(IEEE_Double, REG_OP(0), 2);
                             break;
        case XED_IFORM_CVTDQ2PS_XMMps_MEMdq:
            OP_TYPE(OP_CVT); INPUT_OP(SignedInt32, MEMORY_OP, 0); OUTPUT_OP(IEEE_Single, REG_OP(0), 0);
                             INPUT_OP(SignedInt32, MEMORY_OP, 1); OUTPUT_OP(IEEE_Single, REG_OP(0), 1);
                             INPUT_OP(SignedInt32, MEMORY_OP, 2); OUTPUT_OP(IEEE_Single, REG_OP(0), 2);
                             INPUT_OP(SignedInt32, MEMORY_OP, 3); OUTPUT_OP(IEEE_Single, REG_OP(0), 3);
                             mi++; break;
        case XED_IFORM_CVTDQ2PS_XMMps_XMMdq:
            OP_TYPE(OP_CVT); INPUT_OP(SignedInt32, REG_OP(1), 0); OUTPUT_OP(IEEE_Single, REG_OP(0), 0);
                             INPUT_OP(SignedInt32, REG_OP(1), 1); OUTPUT_OP(IEEE_Single, REG_OP(0), 1);
                             INPUT_OP(SignedInt32, REG_OP(1), 2); OUTPUT_OP(IEEE_Single, REG_OP(0), 2);
                             INPUT_OP(SignedInt32, REG_OP(1), 3); OUTPUT_OP(IEEE_Single, REG_OP(0), 3);
                             break;
        case XED_IFORM_CVTPD2DQ_XMMdq_MEMpd:
        case XED_IFORM_CVTTPD2DQ_XMMdq_MEMpd:
            OP_TYPE(OP_CVT); ZERO_OP(SignedInt64, REG_OP(0), 2);
                             INPUT_OP(IEEE_Double, MEMORY_OP, 0); OUTPUT_OP(SignedInt32, REG_OP(0), 0);
                             INPUT_OP(IEEE_Double, MEMORY_OP, 2); OUTPUT_OP(SignedInt32, REG_OP(0), 1);
                             mi++; break;
        case XED_IFORM_CVTPD2DQ_XMMdq_XMMpd:
        case XED_IFORM_CVTTPD2DQ_XMMdq_XMMpd:
            OP_TYPE(OP_CVT); ZERO_OP(SignedInt64, REG_OP(0), 2);
                             INPUT_OP(IEEE_Double, REG_OP(1), 0); OUTPUT_OP(SignedInt32, REG_OP(0), 0);
                             INPUT_OP(IEEE_Double, REG_OP(1), 2); OUTPUT_OP(SignedInt32, REG_OP(0), 1);
                             break;
        //case XED_IFORM_CVTPD2PI_MMXq_MEMpd:
        //case XED_IFORM_CVTTPD2PI_MMXq_MEMpd:
        //case XED_IFORM_CVTPD2PI_MMXq_XMMpd:
        //case XED_IFORM_CVTTPD2PI_MMXq_XMMpd:
        case XED_IFORM_CVTPD2PS_XMMps_MEMpd:
            OP_TYPE(OP_CVT); ZERO_OP(SignedInt64, REG_OP(0), 2);
                             INPUT_OP(IEEE_Double, MEMORY_OP, 0); OUTPUT_OP(IEEE_Single, REG_OP(0), 0);
                             INPUT_OP(IEEE_Double, MEMORY_OP, 2); OUTPUT_OP(IEEE_Single, REG_OP(0), 1);
                             mi++; break;
        case XED_IFORM_CVTPD2PS_XMMps_XMMpd:
            OP_TYPE(OP_CVT); ZERO_OP(SignedInt64, REG_OP(0), 2);
                             INPUT_OP(IEEE_Double, REG_OP(1), 0); OUTPUT_OP(IEEE_Single, REG_OP(0), 0);
                             INPUT_OP(IEEE_Double, REG_OP(1), 2); OUTPUT_OP(IEEE_Single, REG_OP(0), 1);
                             break;
        case XED_IFORM_CVTPI2PD_XMMpd_MEMq:
            OP_TYPE(OP_CVT); INPUT_OP(SignedInt32, MEMORY_OP, 0); OUTPUT_OP(IEEE_Double, REG_OP(0), 0);
                             INPUT_OP(SignedInt32, MEMORY_OP, 1); OUTPUT_OP(IEEE_Double, REG_OP(0), 2);
                             mi++; break;
        //case XED_IFORM_CVTPI2PD_XMMpd_MMXq:
        case XED_IFORM_CVTPI2PS_XMMq_MEMq:
            OP_TYPE(OP_CVT); INPUT_OP(SignedInt32, MEMORY_OP, 0); OUTPUT_OP(IEEE_Single, REG_OP(0), 0);
                             INPUT_OP(SignedInt32, MEMORY_OP, 1); OUTPUT_OP(IEEE_Single, REG_OP(0), 1);
                             mi++; break;
        //case XED_IFORM_CVTPI2PS_XMMq_MMXq:
        case XED_IFORM_CVTPS2DQ_XMMdq_MEMps:
        case XED_IFORM_CVTTPS2DQ_XMMdq_MEMps:
            OP_TYPE(OP_CVT); INPUT_OP(IEEE_Single, MEMORY_OP, 0); OUTPUT_OP(SignedInt32, REG_OP(0), 0);
                             INPUT_OP(IEEE_Single, MEMORY_OP, 1); OUTPUT_OP(SignedInt32, REG_OP(0), 1);
                             INPUT_OP(IEEE_Single, MEMORY_OP, 2); OUTPUT_OP(SignedInt32, REG_OP(0), 2);
                             INPUT_OP(IEEE_Single, MEMORY_OP, 3); OUTPUT_OP(SignedInt32, REG_OP(0), 3);
                             mi++; break;
        case XED_IFORM_CVTPS2DQ_XMMdq_XMMps:
        case XED_IFORM_CVTTPS2DQ_XMMdq_XMMps:
            OP_TYPE(OP_CVT); INPUT_OP(IEEE_Single, REG_OP(1), 0); OUTPUT_OP(SignedInt32, REG_OP(0), 0);
                             INPUT_OP(IEEE_Single, REG_OP(1), 1); OUTPUT_OP(SignedInt32, REG_OP(0), 1);
                             INPUT_OP(IEEE_Single, REG_OP(1), 2); OUTPUT_OP(SignedInt32, REG_OP(0), 2);
                             INPUT_OP(IEEE_Single, REG_OP(1), 3); OUTPUT_OP(SignedInt32, REG_OP(0), 3);
                             break;
        case XED_IFORM_CVTPS2PD_XMMpd_MEMq:
            OP_TYPE(OP_CVT); INPUT_OP(IEEE_Single, MEMORY_OP, 0); OUTPUT_OP(IEEE_Double, REG_OP(0), 0);
                             INPUT_OP(IEEE_Single, MEMORY_OP, 1); OUTPUT_OP(IEEE_Double, REG_OP(0), 2);
                             mi++; break;
        case XED_IFORM_CVTPS2PD_XMMpd_XMMq:
            OP_TYPE(OP_CVT); INPUT_OP(IEEE_Single, REG_OP(1), 0); OUTPUT_OP(IEEE_Double, REG_OP(0), 0);
                             INPUT_OP(IEEE_Single, REG_OP(1), 1); OUTPUT_OP(IEEE_Double, REG_OP(0), 2);
                             break;
        //case XED_IFORM_CVTPS2PI_MMXq_MEMq:
        //case XED_IFORM_CVTTPS2PI_MMXq_MEMq:
        //case XED_IFORM_CVTPS2PI_MMXq_XMMq:
        //case XED_IFORM_CVTTPS2PI_MMXq_XMMq:
        case XED_IFORM_CVTSD2SI_GPR32d_MEMsd:
        case XED_IFORM_CVTTSD2SI_GPR32d_MEMsd:
            OP_TYPE(OP_CVT); INPUT_OP(IEEE_Double, MEMORY_OP, 0); OUTPUT_OP(SignedInt32, REG_OP(0), 0); mi++; break;
        case XED_IFORM_CVTSD2SI_GPR32d_XMMsd:
        case XED_IFORM_CVTTSD2SI_GPR32d_XMMsd:
            OP_TYPE(OP_CVT); INPUT_OP(IEEE_Double, REG_OP(1), 0); OUTPUT_OP(SignedInt32, REG_OP(0), 0); break;
        case XED_IFORM_CVTSD2SI_GPR64q_MEMsd:
        case XED_IFORM_CVTTSD2SI_GPR64q_MEMsd:
            OP_TYPE(OP_CVT); INPUT_OP(IEEE_Double, MEMORY_OP, 0); OUTPUT_OP(SignedInt64, REG_OP(0), 0); mi++; break;
        case XED_IFORM_CVTSD2SI_GPR64q_XMMsd:
        case XED_IFORM_CVTTSD2SI_GPR64q_XMMsd:
            OP_TYPE(OP_CVT); INPUT_OP(IEEE_Double, REG_OP(1), 0); OUTPUT_OP(SignedInt64, REG_OP(0), 0); break;
        case XED_IFORM_CVTSD2SS_XMMss_MEMsd:
            OP_TYPE(OP_CVT); INPUT_OP(IEEE_Double, MEMORY_OP, 0); OUTPUT_OP(IEEE_Single, REG_OP(0), 0); mi++; break;
        case XED_IFORM_CVTSD2SS_XMMss_XMMsd:
            OP_TYPE(OP_CVT); INPUT_OP(IEEE_Double, REG_OP(1), 0); OUTPUT_OP(IEEE_Single, REG_OP(0), 0); break;
        case XED_IFORM_CVTSI2SD_XMMsd_GPR32d:
            OP_TYPE(OP_CVT); INPUT_OP(SignedInt32, REG_OP(1), 0); OUTPUT_OP(IEEE_Double, REG_OP(0), 0); break;
        case XED_IFORM_CVTSI2SD_XMMsd_GPR64q:
            OP_TYPE(OP_CVT); INPUT_OP(SignedInt64, REG_OP(1), 0); OUTPUT_OP(IEEE_Double, REG_OP(0), 0); break;
        case XED_IFORM_CVTSI2SD_XMMsd_MEMd:
            OP_TYPE(OP_CVT); INPUT_OP(SignedInt32, MEMORY_OP, 0); OUTPUT_OP(IEEE_Double, REG_OP(0), 0); mi++; break;
        case XED_IFORM_CVTSI2SD_XMMsd_MEMq:
            OP_TYPE(OP_CVT); INPUT_OP(SignedInt64, MEMORY_OP, 0); OUTPUT_OP(IEEE_Double, REG_OP(0), 0); mi++; break;
        case XED_IFORM_CVTSI2SS_XMMss_GPR32d:
            OP_TYPE(OP_CVT); INPUT_OP(SignedInt32, REG_OP(1), 0); OUTPUT_OP(IEEE_Single, REG_OP(0), 0); break;
        case XED_IFORM_CVTSI2SS_XMMss_GPR64q:
            OP_TYPE(OP_CVT); INPUT_OP(SignedInt64, REG_OP(1), 0); OUTPUT_OP(IEEE_Single, REG_OP(0), 0); break;
        case XED_IFORM_CVTSI2SS_XMMss_MEMd:
            OP_TYPE(OP_CVT); INPUT_OP(SignedInt32, MEMORY_OP, 0); OUTPUT_OP(IEEE_Single, REG_OP(0), 0); mi++; break;
        case XED_IFORM_CVTSI2SS_XMMss_MEMq:
            OP_TYPE(OP_CVT); INPUT_OP(SignedInt64, MEMORY_OP, 0); OUTPUT_OP(IEEE_Single, REG_OP(0), 0); mi++; break;
        case XED_IFORM_CVTSS2SD_XMMsd_MEMss:
            OP_TYPE(OP_CVT); INPUT_OP(IEEE_Single, MEMORY_OP, 0); OUTPUT_OP(IEEE_Double, REG_OP(0), 0); mi++; break;
        case XED_IFORM_CVTSS2SD_XMMsd_XMMss:
            OP_TYPE(OP_CVT); INPUT_OP(IEEE_Single, REG_OP(1), 0); OUTPUT_OP(IEEE_Double, REG_OP(0), 0); break;
        case XED_IFORM_CVTSS2SI_GPR32d_MEMss:
        case XED_IFORM_CVTTSS2SI_GPR32d_MEMss:
            OP_TYPE(OP_CVT); INPUT_OP(IEEE_Single, MEMORY_OP, 0); OUTPUT_OP(SignedInt32, REG_OP(0), 0); mi++; break;
        case XED_IFORM_CVTSS2SI_GPR32d_XMMss:
        case XED_IFORM_CVTTSS2SI_GPR32d_XMMss:
            OP_TYPE(OP_CVT); INPUT_OP(IEEE_Single, REG_OP(1), 0); OUTPUT_OP(SignedInt32, REG_OP(0), 0); break;
        case XED_IFORM_CVTSS2SI_GPR64q_MEMss:
        case XED_IFORM_CVTTSS2SI_GPR64q_MEMss:
            OP_TYPE(OP_CVT); INPUT_OP(IEEE_Single, MEMORY_OP, 0); OUTPUT_OP(SignedInt64, REG_OP(0), 0); mi++; break;
        case XED_IFORM_CVTSS2SI_GPR64q_XMMss:
        case XED_IFORM_CVTTSS2SI_GPR64q_XMMss:
            OP_TYPE(OP_CVT); INPUT_OP(IEEE_Single, REG_OP(1), 0); OUTPUT_OP(SignedInt64, REG_OP(0), 0); break;
        // TODO: need special handling for "truncated" conversions?
        // }}}

        // {{{ SSE comparisons
        // TODO: check these
        case XED_IFORM_CMPSS_XMMss_MEMss_IMMb:
            OP_TYPE(OP_CMP); INPUT_OP(IEEE_Single, REG_OP(0), 0); INPUT_OP(IEEE_Single, MEMORY_OP, 0);
                             operation->addInputOperand(new FPOperand((uint8_t)xed_decoded_inst_get_unsigned_immediate(&xedd)));
                             OUTPUT_OP(UnsignedInt32, REG_OP(0), 0); mi++; break;
        case XED_IFORM_CMPSS_XMMss_XMMss_IMMb:
            OP_TYPE(OP_CMP); INPUT_OP(IEEE_Single, REG_OP(0), 0); INPUT_OP(IEEE_Single, REG_OP(1), 0);
                             operation->addInputOperand(new FPOperand((uint8_t)xed_decoded_inst_get_unsigned_immediate(&xedd)));
                             OUTPUT_OP(UnsignedInt32, REG_OP(0), 0); break;
        case XED_IFORM_CMPSD_XMM_XMMsd_MEMsd_IMMb:
            OP_TYPE(OP_CMP); INPUT_OP(IEEE_Double, REG_OP(0), 0); INPUT_OP(IEEE_Double, MEMORY_OP, 0);
                             operation->addInputOperand(new FPOperand((uint8_t)xed_decoded_inst_get_unsigned_immediate(&xedd)));
                             OUTPUT_OP(UnsignedInt64, REG_OP(0), 0); mi++; break;
        case XED_IFORM_CMPSD_XMM_XMMsd_XMMsd_IMMb:
            OP_TYPE(OP_CMP); INPUT_OP(IEEE_Double, REG_OP(0), 0); INPUT_OP(IEEE_Double, REG_OP(1), 0);
                             operation->addInputOperand(new FPOperand((uint8_t)xed_decoded_inst_get_unsigned_immediate(&xedd)));
                             OUTPUT_OP(UnsignedInt64, REG_OP(0), 0); break;
        case XED_IFORM_CMPPS_XMMps_MEMps_IMMb:
            OP_TYPE(OP_CMP); INPUT_OP(IEEE_Single, REG_OP(0), 0); INPUT_OP(IEEE_Single, MEMORY_OP, 0);
                             operation->addInputOperand(new FPOperand((uint8_t)xed_decoded_inst_get_unsigned_immediate(&xedd)));
                             OUTPUT_OP(UnsignedInt32, REG_OP(0), 0);
                             INPUT_OP(IEEE_Single, REG_OP(0), 1); INPUT_OP(IEEE_Single, MEMORY_OP, 1);
                             operation->addInputOperand(new FPOperand((uint8_t)xed_decoded_inst_get_unsigned_immediate(&xedd)));
                             OUTPUT_OP(UnsignedInt32, REG_OP(0), 1);
                             INPUT_OP(IEEE_Single, REG_OP(0), 2); INPUT_OP(IEEE_Single, MEMORY_OP, 2);
                             operation->addInputOperand(new FPOperand((uint8_t)xed_decoded_inst_get_unsigned_immediate(&xedd)));
                             OUTPUT_OP(UnsignedInt32, REG_OP(0), 2);
                             INPUT_OP(IEEE_Single, REG_OP(0), 3); INPUT_OP(IEEE_Single, MEMORY_OP, 3);
                             operation->addInputOperand(new FPOperand((uint8_t)xed_decoded_inst_get_unsigned_immediate(&xedd)));
                             OUTPUT_OP(UnsignedInt32, REG_OP(0), 3); mi++; break;
        case XED_IFORM_CMPPS_XMMps_XMMps_IMMb:
            OP_TYPE(OP_CMP); INPUT_OP(IEEE_Single, REG_OP(0), 0); INPUT_OP(IEEE_Single, REG_OP(1), 0);
                             operation->addInputOperand(new FPOperand((uint8_t)xed_decoded_inst_get_unsigned_immediate(&xedd)));
                             OUTPUT_OP(UnsignedInt32, REG_OP(0), 0);
                             INPUT_OP(IEEE_Single, REG_OP(0), 1); INPUT_OP(IEEE_Single, REG_OP(1), 1);
                             operation->addInputOperand(new FPOperand((uint8_t)xed_decoded_inst_get_unsigned_immediate(&xedd)));
                             OUTPUT_OP(UnsignedInt32, REG_OP(0), 1);
                             INPUT_OP(IEEE_Single, REG_OP(0), 2); INPUT_OP(IEEE_Single, REG_OP(1), 2);
                             operation->addInputOperand(new FPOperand((uint8_t)xed_decoded_inst_get_unsigned_immediate(&xedd)));
                             OUTPUT_OP(UnsignedInt32, REG_OP(0), 2);
                             INPUT_OP(IEEE_Single, REG_OP(0), 3); INPUT_OP(IEEE_Single, REG_OP(1), 3);
                             operation->addInputOperand(new FPOperand((uint8_t)xed_decoded_inst_get_unsigned_immediate(&xedd)));
                             OUTPUT_OP(UnsignedInt32, REG_OP(0), 3); break;
        case XED_IFORM_CMPPD_XMMpd_MEMpd_IMMb:
            OP_TYPE(OP_CMP); INPUT_OP(IEEE_Double, REG_OP(0), 0); INPUT_OP(IEEE_Double, MEMORY_OP, 0);
                             operation->addInputOperand(new FPOperand((uint8_t)xed_decoded_inst_get_unsigned_immediate(&xedd)));
                             OUTPUT_OP(UnsignedInt64, REG_OP(0), 0);
                             INPUT_OP(IEEE_Double, REG_OP(0), 2); INPUT_OP(IEEE_Double, MEMORY_OP, 2);
                             operation->addInputOperand(new FPOperand((uint8_t)xed_decoded_inst_get_unsigned_immediate(&xedd)));
                             OUTPUT_OP(UnsignedInt64, REG_OP(0), 2); mi++; break;
        case XED_IFORM_CMPPD_XMMpd_XMMpd_IMMb:
            OP_TYPE(OP_CMP); INPUT_OP(IEEE_Double, REG_OP(0), 0); INPUT_OP(IEEE_Double, REG_OP(1), 0);
                             operation->addInputOperand(new FPOperand((uint8_t)xed_decoded_inst_get_unsigned_immediate(&xedd)));
                             OUTPUT_OP(UnsignedInt64, REG_OP(0), 0);
                             INPUT_OP(IEEE_Double, REG_OP(0), 2); INPUT_OP(IEEE_Double, REG_OP(1), 2);
                             operation->addInputOperand(new FPOperand((uint8_t)xed_decoded_inst_get_unsigned_immediate(&xedd)));
                             OUTPUT_OP(UnsignedInt64, REG_OP(0), 2); break;

        case XED_IFORM_COMISD_XMMsd_MEMsd:
            OP_TYPE(OP_COMI);  INPUT_OP(IEEE_Double, REG_OP(0), 0); INPUT_OP(IEEE_Double, MEMORY_OP, 0); mi++;
                               operation->addOutputOperand(new FPOperand(UnsignedInt32, REG_EFLAGS)); break;
        case XED_IFORM_UCOMISD_XMMsd_MEMsd:
            OP_TYPE(OP_UCOMI); INPUT_OP(IEEE_Double, REG_OP(0), 0); INPUT_OP(IEEE_Double, MEMORY_OP, 0); mi++;
                               operation->addOutputOperand(new FPOperand(UnsignedInt32, REG_EFLAGS)); break;
        case XED_IFORM_COMISD_XMMsd_XMMsd:
            OP_TYPE(OP_COMI);  INPUT_OP(IEEE_Double, REG_OP(0), 0); INPUT_OP(IEEE_Double, REG_OP(1), 0);
                               operation->addOutputOperand(new FPOperand(UnsignedInt32, REG_EFLAGS)); break;
        case XED_IFORM_UCOMISD_XMMsd_XMMsd:
            OP_TYPE(OP_UCOMI); INPUT_OP(IEEE_Double, REG_OP(0), 0); INPUT_OP(IEEE_Double, REG_OP(1), 0);
                               operation->addOutputOperand(new FPOperand(UnsignedInt32, REG_EFLAGS)); break;
        case XED_IFORM_COMISS_XMMss_MEMss:
            OP_TYPE(OP_COMI);  INPUT_OP(IEEE_Single, REG_OP(0), 0); INPUT_OP(IEEE_Single, MEMORY_OP, 0); mi++;
                               operation->addOutputOperand(new FPOperand(UnsignedInt32, REG_EFLAGS)); break;
        case XED_IFORM_UCOMISS_XMMss_MEMss:
            OP_TYPE(OP_UCOMI); INPUT_OP(IEEE_Single, REG_OP(0), 0); INPUT_OP(IEEE_Single, MEMORY_OP, 0); mi++;
                               operation->addOutputOperand(new FPOperand(UnsignedInt32, REG_EFLAGS)); break;
        case XED_IFORM_COMISS_XMMss_XMMss:
            OP_TYPE(OP_COMI);  INPUT_OP(IEEE_Single, REG_OP(0), 0); INPUT_OP(IEEE_Single, REG_OP(1), 0);
                               operation->addOutputOperand(new FPOperand(UnsignedInt32, REG_EFLAGS)); break;
        case XED_IFORM_UCOMISS_XMMss_XMMss:
            OP_TYPE(OP_UCOMI); INPUT_OP(IEEE_Single, REG_OP(0), 0); INPUT_OP(IEEE_Single, REG_OP(1), 0);
                               operation->addOutputOperand(new FPOperand(UnsignedInt32, REG_EFLAGS)); break;

        //case XED_IFORM_PCMPEQD_XMMdq_MEMdq:
            //OP_TYPE(OP_CMP); INPUT_OP(IEEE_Single, REG_OP(0), 0); INPUT_OP(IEEE_Single, MEMORY_OP, 0);
                             //OUTPUT_OP(UnsignedInt64, REG_OP(0), 0); mi++; break;
        //case XED_IFORM_PCMPEQD_XMMdq_XMMdq:
            //OP_TYPE(OP_CMP); INPUT_OP(IEEE_Single, REG_OP(0), 0); INPUT_OP(IEEE_Single, REG_OP(1), 0);
                             //OUTPUT_OP(UnsignedInt64, REG_OP(0), 0); break;
        //case XED_IFORM_PCMPEQQ_XMMdq_MEMdq:
            //OP_TYPE(OP_CMP); INPUT_OP(IEEE_Double, REG_OP(0), 0); INPUT_OP(IEEE_Double, MEMORY_OP, 0);
                             //OUTPUT_OP(UnsignedInt64, REG_OP(0), 0); mi++; break;
        //case XED_IFORM_PCMPEQQ_XMMdq_XMMdq:
            //OP_TYPE(OP_CMP); INPUT_OP(IEEE_Double, REG_OP(0), 0); INPUT_OP(IEEE_Double, REG_OP(1), 0);
                             //OUTPUT_OP(UnsignedInt64, REG_OP(0), 0); break;
        // }}}

        // {{{ weird misc crap

        // bit test and complement; these are only the common FP semantics
        case XED_IFORM_BTC_GPRv_GPRv:
        case XED_IFORM_BTC_GPRv_IMMb:
            if ((uint8_t)xed_decoded_inst_get_unsigned_immediate(&xedd) == 0x3f) {
                OP_TYPE(OP_NEG); INPUT_OP(IEEE_Double, REG_OP(0), 0); OUTPUT_OP(IEEE_Double, REG_OP(0), 0); break;
            } else if ((uint8_t)xed_decoded_inst_get_unsigned_immediate(&xedd) == 0x1f) {
                OP_TYPE(OP_NEG); INPUT_OP(IEEE_Single, REG_OP(0), 0); OUTPUT_OP(IEEE_Single, REG_OP(0), 0); break;
            }
        case XED_IFORM_BTC_MEMv_GPRv:
        case XED_IFORM_BTC_MEMv_IMMb:
            if ((uint8_t)xed_decoded_inst_get_unsigned_immediate(&xedd) == 0x3f) {
                OP_TYPE(OP_NEG); INPUT_OP(IEEE_Double, MEMORY_OP, 0); OUTPUT_OP(IEEE_Double, MEMORY_OP, 0);
            } else if ((uint8_t)xed_decoded_inst_get_unsigned_immediate(&xedd) == 0x1f) {
                OP_TYPE(OP_NEG); INPUT_OP(IEEE_Single, MEMORY_OP, 0); OUTPUT_OP(IEEE_Single, MEMORY_OP, 0);
            }
                             mi++; break;
        // }}}

        // {{{ stuff to ignore
        case XED_IFORM_FXRSTOR_MEMmfpxenv:              // fp state save/restore
        case XED_IFORM_FXRSTOR64_MEMmfpxenv:
        case XED_IFORM_FXSAVE_MEMmfpxenv:
        case XED_IFORM_FXSAVE64_MEMmfpxenv:
        case XED_IFORM_LDMXCSR_MEMd:                    // status/flags load/store
        case XED_IFORM_STMXCSR_MEMd:
        case XED_IFORM_FLDCW_MEMmem16:
        case XED_IFORM_FNSTCW_MEMmem16:
        case XED_IFORM_FNSTSW_AX:
        case XED_IFORM_FNSTSW_MEMmem16:
        case XED_IFORM_PAND_MMXq_MEMq:                  // packed MMX bitwise operations
        case XED_IFORM_PAND_MMXq_MMXq:
        case XED_IFORM_PANDN_MMXq_MEMq:
        case XED_IFORM_PANDN_MMXq_MMXq:
        case XED_IFORM_POR_MMXq_MEMq:
        case XED_IFORM_POR_MMXq_MMXq:
        case XED_IFORM_PXOR_MMXq_MEMq:
        case XED_IFORM_PXOR_MMXq_MMXq:
        case XED_IFORM_PADDB_MMXq_MEMq:                 // packed integer arithmetic
        case XED_IFORM_PADDB_MMXq_MMXq:
        case XED_IFORM_PADDB_XMMdq_MEMdq:
        case XED_IFORM_PADDB_XMMdq_XMMdq:
        case XED_IFORM_PADDD_MMXq_MEMq:
        case XED_IFORM_PADDD_MMXq_MMXq:
        case XED_IFORM_PADDD_XMMdq_MEMdq:
        case XED_IFORM_PADDD_XMMdq_XMMdq:
        case XED_IFORM_PADDQ_MMXq_MEMq:
        case XED_IFORM_PADDQ_MMXq_MMXq:
        case XED_IFORM_PADDQ_XMMdq_MEMdq:
        case XED_IFORM_PADDQ_XMMdq_XMMdq:
        case XED_IFORM_PADDSB_MMXq_MEMq:
        case XED_IFORM_PADDSB_MMXq_MMXq:
        case XED_IFORM_PADDSB_XMMdq_MEMdq:
        case XED_IFORM_PADDSB_XMMdq_XMMdq:
        case XED_IFORM_PADDSW_MMXq_MEMq:
        case XED_IFORM_PADDSW_MMXq_MMXq:
        case XED_IFORM_PADDSW_XMMdq_MEMdq:
        case XED_IFORM_PADDSW_XMMdq_XMMdq:
        case XED_IFORM_PADDUSB_MMXq_MEMq:
        case XED_IFORM_PADDUSB_MMXq_MMXq:
        case XED_IFORM_PADDUSB_XMMdq_MEMdq:
        case XED_IFORM_PADDUSB_XMMdq_XMMdq:
        case XED_IFORM_PADDUSW_MMXq_MEMq:
        case XED_IFORM_PADDUSW_MMXq_MMXq:
        case XED_IFORM_PADDUSW_XMMdq_MEMdq:
        case XED_IFORM_PADDUSW_XMMdq_XMMdq:
        case XED_IFORM_PADDW_MMXq_MEMq:
        case XED_IFORM_PADDW_MMXq_MMXq:
        case XED_IFORM_PADDW_XMMdq_MEMdq:
        case XED_IFORM_PADDW_XMMdq_XMMdq:
        case XED_IFORM_PMULDQ_XMMdq_MEMdq:
        case XED_IFORM_PMULDQ_XMMdq_XMMdq:
        case XED_IFORM_PMULHRSW_MMXq_MEMq:
        case XED_IFORM_PMULHRSW_MMXq_MMXq:
        case XED_IFORM_PMULHRSW_XMMdq_MEMdq:
        case XED_IFORM_PMULHRSW_XMMdq_XMMdq:
        case XED_IFORM_PMULHRW_MMXq_MEMq:
        case XED_IFORM_PMULHRW_MMXq_MMXq:
        case XED_IFORM_PMULHUW_MMXq_MEMq:
        case XED_IFORM_PMULHUW_MMXq_MMXq:
        case XED_IFORM_PMULHUW_XMMdq_MEMdq:
        case XED_IFORM_PMULHUW_XMMdq_XMMdq:
        case XED_IFORM_PMULHW_MMXq_MEMq:
        case XED_IFORM_PMULHW_MMXq_MMXq:
        case XED_IFORM_PMULHW_XMMdq_MEMdq:
        case XED_IFORM_PMULHW_XMMdq_XMMdq:
        case XED_IFORM_PMULLD_XMMdq_MEMdq:
        case XED_IFORM_PMULLD_XMMdq_XMMdq:
        case XED_IFORM_PMULLW_MMXq_MEMq:
        case XED_IFORM_PMULLW_MMXq_MMXq:
        case XED_IFORM_PMULLW_XMMdq_MEMdq:
        case XED_IFORM_PMULLW_XMMdq_XMMdq:
        case XED_IFORM_PMULUDQ_MMXq_MEMq:
        case XED_IFORM_PMULUDQ_MMXq_MMXq:
        case XED_IFORM_PMULUDQ_XMMdq_MEMdq:
        case XED_IFORM_PMULUDQ_XMMdq_XMMdq:
        case XED_IFORM_PCMPEQB_XMMdq_MEMdq:             // packed integer comparisons
        case XED_IFORM_PCMPEQB_XMMdq_XMMdq:
        case XED_IFORM_PCMPEQD_XMMdq_MEMdq:
        case XED_IFORM_PCMPEQD_XMMdq_XMMdq:
        case XED_IFORM_PCMPEQQ_XMMdq_MEMdq:
        case XED_IFORM_PCMPEQQ_XMMdq_XMMdq:
        case XED_IFORM_PCMPEQW_XMMdq_MEMdq:
        case XED_IFORM_PCMPEQW_XMMdq_XMMdq:
        case XED_IFORM_PCMPESTRI_XMMdq_MEMdq_IMMb:
        case XED_IFORM_PCMPESTRI_XMMdq_XMMdq_IMMb:
        case XED_IFORM_PCMPESTRM_XMMdq_MEMdq_IMMb:
        case XED_IFORM_PCMPESTRM_XMMdq_XMMdq_IMMb:
        case XED_IFORM_PCMPGTB_XMMdq_MEMdq:
        case XED_IFORM_PCMPGTB_XMMdq_XMMdq:
        case XED_IFORM_PCMPGTD_XMMdq_MEMdq:
        case XED_IFORM_PCMPGTD_XMMdq_XMMdq:
        case XED_IFORM_PCMPGTQ_XMMdq_MEMdq:
        case XED_IFORM_PCMPGTQ_XMMdq_XMMdq:
        case XED_IFORM_PCMPGTW_XMMdq_MEMdq:
        case XED_IFORM_PCMPGTW_XMMdq_XMMdq:
        case XED_IFORM_PCMPISTRI_XMMdq_MEMdq_IMMb:
        case XED_IFORM_PCMPISTRI_XMMdq_XMMdq_IMMb:
        case XED_IFORM_PCMPISTRM_XMMdq_MEMdq_IMMb:
        case XED_IFORM_PCMPISTRM_XMMdq_XMMdq_IMMb:
        case XED_IFORM_PSLLD_XMMdq_IMMb:                // packed integer bit shifts
        case XED_IFORM_PSLLD_XMMdq_MEMdq:
        case XED_IFORM_PSLLD_XMMdq_XMMdq:
        case XED_IFORM_PSLLDQ_XMMdq_IMMb:
        case XED_IFORM_PSLLQ_XMMdq_IMMb:
        case XED_IFORM_PSLLQ_XMMdq_MEMdq:
        case XED_IFORM_PSLLQ_XMMdq_XMMdq:
        case XED_IFORM_PSLLW_XMMdq_IMMb:
        case XED_IFORM_PSLLW_XMMdq_MEMdq:
        case XED_IFORM_PSLLW_XMMdq_XMMdq:
        case XED_IFORM_PSRAD_XMMdq_IMMb:
        case XED_IFORM_PSRAD_XMMdq_MEMdq:
        case XED_IFORM_PSRAD_XMMdq_XMMdq:
        case XED_IFORM_PSRAW_XMMdq_IMMb:
        case XED_IFORM_PSRAW_XMMdq_MEMdq:
        case XED_IFORM_PSRAW_XMMdq_XMMdq:
        case XED_IFORM_PSRLD_XMMdq_IMMb:
        case XED_IFORM_PSRLD_XMMdq_MEMdq:
        case XED_IFORM_PSRLD_XMMdq_XMMdq:
        case XED_IFORM_PSRLDQ_XMMdq_IMMb:
        case XED_IFORM_PSRLQ_XMMdq_IMMb:
        case XED_IFORM_PSRLQ_XMMdq_MEMdq:
        case XED_IFORM_PSRLQ_XMMdq_XMMdq:
        case XED_IFORM_PSRLW_XMMdq_IMMb:
        case XED_IFORM_PSRLW_XMMdq_MEMdq:
        case XED_IFORM_PSRLW_XMMdq_XMMdq:
        case XED_IFORM_PUNPCKHBW_XMMdq_MEMdq:           // packed integer movement
        case XED_IFORM_PUNPCKHBW_XMMdq_XMMq:
        case XED_IFORM_PUNPCKHWD_XMMdq_MEMdq:
        case XED_IFORM_PUNPCKHWD_XMMdq_XMMq:
        case XED_IFORM_PUNPCKLBW_XMMdq_MEMdq:
        case XED_IFORM_PUNPCKLBW_XMMdq_XMMq:
        case XED_IFORM_PUNPCKLWD_XMMdq_MEMdq:
        case XED_IFORM_PUNPCKLWD_XMMdq_XMMq:
        case XED_IFORM_MOVQ_GPR64_XMMq:
        case XED_IFORM_MOVQ_XMMdq_GPR64:
        case XED_IFORM_MOVD_GPR32_MMXd:
        case XED_IFORM_MOVD_MMXq_GPR32:
        case XED_IFORM_PACKSSDW_MMXq_MEMq:
        case XED_IFORM_PACKSSDW_MMXq_MMXq:
        case XED_IFORM_PACKSSDW_XMMdq_MEMdq:
        case XED_IFORM_PACKSSDW_XMMdq_XMMdq:
        case XED_IFORM_PACKSSWB_MMXq_MEMq:
        case XED_IFORM_PACKSSWB_MMXq_MMXq:
        case XED_IFORM_PACKSSWB_XMMdq_MEMdq:
        case XED_IFORM_PACKSSWB_XMMdq_XMMdq:
        case XED_IFORM_PACKUSDW_XMMdq_MEMdq:
        case XED_IFORM_PACKUSDW_XMMdq_XMMdq:
        case XED_IFORM_PACKUSWB_MMXq_MEMq:
        case XED_IFORM_PACKUSWB_MMXq_MMXq:
        case XED_IFORM_PACKUSWB_XMMdq_MEMdq:
        case XED_IFORM_PACKUSWB_XMMdq_XMMdq:
        case XED_IFORM_PSHUFB_MMXq_MEMq:
        case XED_IFORM_PSHUFB_MMXq_MMXq:
        case XED_IFORM_PSHUFB_XMMdq_MEMdq:
        case XED_IFORM_PSHUFB_XMMdq_XMMdq:
        case XED_IFORM_PSHUFD_XMMdq_MEMdq_IMMb:
        case XED_IFORM_PSHUFD_XMMdq_XMMdq_IMMb:
        case XED_IFORM_PSHUFHW_XMMdq_MEMdq_IMMb:
        case XED_IFORM_PSHUFHW_XMMdq_XMMdq_IMMb:
        case XED_IFORM_PSHUFLW_XMMdq_MEMdq_IMMb:
        case XED_IFORM_PSHUFLW_XMMdq_XMMdq_IMMb:
        case XED_IFORM_PSHUFW_MMXq_MEMq_IMMb:
        case XED_IFORM_PSHUFW_MMXq_MMXq_IMMb:
        case XED_IFORM_PEXTRB_GPR32d_XMMdq_IMMb:        // SSE4 integer movement (insert/extract)
        case XED_IFORM_PEXTRB_MEMb_XMMdq_IMMb:
        case XED_IFORM_PEXTRD_GPR32d_XMMdq_IMMb:
        case XED_IFORM_PEXTRD_MEMd_XMMdq_IMMb:
        case XED_IFORM_PEXTRQ_GPR64q_XMMdq_IMMb:
        case XED_IFORM_PEXTRQ_MEMq_XMMdq_IMMb:
        case XED_IFORM_PEXTRW_GPRy_MMXq_IMMb:
        case XED_IFORM_PEXTRW_GPRy_XMMdq_IMMb:
        case XED_IFORM_PEXTRW_SSE4_GPRy_XMMdq_IMMb:
        case XED_IFORM_PEXTRW_SSE4_MEMw_XMMdq_IMMb:
        case XED_IFORM_PINSRB_XMMdq_GPR32d_IMMb:
        case XED_IFORM_PINSRB_XMMdq_MEMb_IMMb:
        case XED_IFORM_PINSRD_XMMdq_GPR32d_IMMb:
        case XED_IFORM_PINSRD_XMMdq_MEMd_IMMb:
        case XED_IFORM_PINSRQ_XMMdq_GPR64q_IMMb:
        case XED_IFORM_PINSRQ_XMMdq_MEMq_IMMb:
        case XED_IFORM_PINSRW_MMXq_GPRy_IMMb:
        case XED_IFORM_PINSRW_MMXq_MEMw_IMMb:
        case XED_IFORM_PINSRW_XMMdq_GPRy_IMMb:
        case XED_IFORM_PINSRW_XMMdq_MEMw_IMMb:
        case XED_IFORM_PMOVMSKB_GPR32_MMXq:             // integer movement with masking
        case XED_IFORM_PMOVMSKB_GPR32_XMMdq:

            OP_TYPE(OP_NONE); break;
        // }}}

        default: OP_TYPE(OP_INVALID); break;
    }
    if (operation->getType() == OP_INVALID) {
        fprintf(stderr, "WARNING: invalid operation from XED: \"%s\" iform=%s addr=%p\n",
               buffer, xed_iform_enum_t2str(iform), addr);
        semantics->add(operation);
        return semantics;
    }

    // finalize main operation
    semantics->add(operation);

    // add extra semantics to handle a x87 stack pop
    for (i = 0; i < xed_inst_noperands(inst); i++) {
        c_operand = (xed_operand_t*)xed_inst_operand(inst, i);
        c_op_name = xed_operand_name(c_operand);
        c_reg_enum = xed_decoded_inst_get_reg(&xedd, c_op_name);
        switch (c_reg_enum) {
            case XED_REG_X87POP2:
                temp_op = new FPOperation();
                temp_op->setType(OP_POP_X87);
                semantics->add(temp_op);
                // allow fallthrough
            case XED_REG_X87POP:
                temp_op = new FPOperation();
                temp_op->setType(OP_POP_X87);
                semantics->add(temp_op);
                break;
            default:
                break;
        }
    }

    debugData << "decoded semantics:" << endl;
    debugData << semantics->toString() << endl;
    debugData << endl;
    semantics->setDecoderDebugData(debugData.str());

    return semantics;
}

FPRegister FPDecoderXED::getX86Reg(FPSemantics *inst)
{
    unsigned char bytes[24];
    size_t nbytes;
    xed_decoded_inst_t xedd;
    xed_error_enum_t xed_error;

    xed_decoded_inst_zero_set_mode(&xedd, &dstate);
    xed_decoded_inst_set_input_chip(&xedd, XED_CHIP_INVALID);

    nbytes = inst->getNumBytes();
    assert(nbytes < 24);
    inst->getBytes(bytes);
    xed_error = xed_decode(&xedd, (const xed_uint8_t*) bytes, nbytes);
    if (xed_error != XED_ERROR_NONE) { 
        return REG_NONE;
    }
    xed_reg_enum_t reg = xed_decoded_inst_get_reg(&xedd, XED_OPERAND_REG0);
    printf("getX86Reg() = %s\n", FPContext::FPReg2Str(xedReg2FPReg(reg)).c_str());
    printf("%s\n", inst->getDecoderDebugData().c_str());
    return xedReg2FPReg(reg);
}

FPRegister FPDecoderXED::getX86RM(FPSemantics *inst)
{
    unsigned char bytes[24];
    size_t nbytes;
    xed_decoded_inst_t xedd;
    xed_error_enum_t xed_error;

    xed_decoded_inst_zero_set_mode(&xedd, &dstate);
    xed_decoded_inst_set_input_chip(&xedd, XED_CHIP_INVALID);

    nbytes = inst->getNumBytes();
    assert(nbytes < 24);
    inst->getBytes(bytes);
    xed_error = xed_decode(&xedd, (const xed_uint8_t*) bytes, nbytes);
    if (xed_error != XED_ERROR_NONE) { 
        return REG_NONE;
    }
    xed_reg_enum_t reg = xed_decoded_inst_get_reg(&xedd, XED_OPERAND_MEM0);
    if (reg == XED_REG_CR0) {
        return REG_NONE;
    } else if (reg == XED_REG_INVALID) {
        reg = xed_decoded_inst_get_reg(&xedd, XED_OPERAND_REG1);
    }
    printf("getX86RM() = %s\n", FPContext::FPReg2Str(xedReg2FPReg(reg)).c_str());
    return xedReg2FPReg(reg);
}

bool FPDecoderXED::readsFlags(FPSemantics *inst)
{
    unsigned char bytes[24];
    size_t nbytes;
    xed_decoded_inst_t xedd;
    xed_error_enum_t xed_error;

    xed_decoded_inst_zero_set_mode(&xedd, &dstate);
    xed_decoded_inst_set_input_chip(&xedd, XED_CHIP_INVALID);

    nbytes = inst->getNumBytes();
    assert(nbytes < 24);
    inst->getBytes(bytes);
    xed_error = xed_decode(&xedd, (const xed_uint8_t*) bytes, nbytes);
    if (xed_error != XED_ERROR_NONE) { 
        return REG_NONE;
    }

    if (xed_decoded_inst_uses_rflags(&xedd)) {
        const xed_simple_flag_t *flags = xed_decoded_inst_get_rflags_info(&xedd);
        const xed_flag_set_t *flagset = xed_simple_flag_get_read_flag_set(flags);
        if (flagset->s.of || flagset->s.sf || flagset->s.zf ||
            flagset->s.af || flagset->s.pf || flagset->s.cf) {
            return true;
        }
    }

    return false;
}

bool FPDecoderXED::writesFlags(FPSemantics *inst)
{
    unsigned char bytes[24];
    size_t nbytes;
    xed_decoded_inst_t xedd;
    xed_error_enum_t xed_error;

    xed_decoded_inst_zero_set_mode(&xedd, &dstate);
    xed_decoded_inst_set_input_chip(&xedd, XED_CHIP_INVALID);

    nbytes = inst->getNumBytes();
    assert(nbytes < 24);
    inst->getBytes(bytes);
    xed_error = xed_decode(&xedd, (const xed_uint8_t*) bytes, nbytes);
    if (xed_error != XED_ERROR_NONE) { 
        return REG_NONE;
    }

    if (xed_decoded_inst_uses_rflags(&xedd)) {
        const xed_simple_flag_t *flags = xed_decoded_inst_get_rflags_info(&xedd);
        const xed_flag_set_t *flagset = xed_simple_flag_get_written_flag_set(flags);
        if (flagset->s.of || flagset->s.sf || flagset->s.zf ||
            flagset->s.af || flagset->s.pf || flagset->s.cf) {
            return true;
        }
    }

    return false;
}

string FPDecoderXED::disassemble(unsigned char *bytes, size_t nbytes)
{
    char buffer[1048];

    xed_decoded_inst_t xedd;
    xed_error_enum_t xed_error;
    xed_decoded_inst_zero_set_mode(&xedd, &dstate);
    xed_decoded_inst_set_input_chip(&xedd, XED_CHIP_INVALID);

    xed_error = xed_decode(&xedd, (const xed_uint8_t*) bytes, nbytes);
    if (xed_error != XED_ERROR_NONE) { 
        return string("(invalid)");
    }

    xed_format_intel(&xedd, buffer, 1048, (xed_uint64_t)(long)bytes);

    return string(buffer);
}

}

