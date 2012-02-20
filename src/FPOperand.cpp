#include "FPOperand.h"

namespace FPInst {

FPOperandAddress FPOperand::FPRegTag2Addr(FPRegister reg, long tag)
{
    // {{{ hard-coded "address" values for registers; this is so that register
    // operands will still have unique keys in shadow value tables
    switch (reg) {
        case REG_EAX: return (FPOperandAddress)0x10;
        case REG_EBX: return (FPOperandAddress)0x11;
        case REG_ECX: return (FPOperandAddress)0x12;
        case REG_EDX: return (FPOperandAddress)0x13;
        case REG_ESP: return (FPOperandAddress)0x14;
        case REG_EBP: return (FPOperandAddress)0x15;
        case REG_ESI: return (FPOperandAddress)0x16;
        case REG_EDI: return (FPOperandAddress)0x17;
        case REG_E8:  return (FPOperandAddress)0x20;
        case REG_E9:  return (FPOperandAddress)0x21;
        case REG_E10: return (FPOperandAddress)0x22;
        case REG_E11: return (FPOperandAddress)0x23;
        case REG_E12: return (FPOperandAddress)0x24;
        case REG_E13: return (FPOperandAddress)0x25;
        case REG_E14: return (FPOperandAddress)0x26;
        case REG_E15: return (FPOperandAddress)0x27;
        case REG_ST0: return (FPOperandAddress)0x40;
        case REG_ST1: return (FPOperandAddress)0x41;
        case REG_ST2: return (FPOperandAddress)0x42;
        case REG_ST3: return (FPOperandAddress)0x43;
        case REG_ST4: return (FPOperandAddress)0x44;
        case REG_ST5: return (FPOperandAddress)0x45;
        case REG_ST6: return (FPOperandAddress)0x46;
        case REG_ST7: return (FPOperandAddress)0x47;
        case REG_XMM0:  return (FPOperandAddress)(0x100+tag);
        case REG_XMM1:  return (FPOperandAddress)(0x110+tag);
        case REG_XMM2:  return (FPOperandAddress)(0x120+tag);
        case REG_XMM3:  return (FPOperandAddress)(0x130+tag);
        case REG_XMM4:  return (FPOperandAddress)(0x140+tag);
        case REG_XMM5:  return (FPOperandAddress)(0x150+tag);
        case REG_XMM6:  return (FPOperandAddress)(0x160+tag);
        case REG_XMM7:  return (FPOperandAddress)(0x170+tag);
        case REG_XMM8:  return (FPOperandAddress)(0x180+tag);
        case REG_XMM9:  return (FPOperandAddress)(0x190+tag);
        case REG_XMM10: return (FPOperandAddress)(0x1A0+tag);
        case REG_XMM11: return (FPOperandAddress)(0x1B0+tag);
        case REG_XMM12: return (FPOperandAddress)(0x1C0+tag);
        case REG_XMM13: return (FPOperandAddress)(0x1D0+tag);
        case REG_XMM14: return (FPOperandAddress)(0x1E0+tag);
        case REG_XMM15: return (FPOperandAddress)(0x1F0+tag);
        default: return (FPOperandAddress)0x0;
    }
    // }}}
}

string FPOperand::FPOpType2Str(FPOperandType type)
{
    string ret("");
    switch (type) {
        case IEEE_Single:      ret = "32-bit IEEE Single";      break;
        case IEEE_Double:      ret = "64-bit IEEE Double";      break;
        case C99_LongDouble:   ret = "80-bit C99 Long Double";  break;
        case SignedInt8:       ret = "8-bit Signed Int";        break;
        case UnsignedInt8:     ret = "8-bit Unsigned Int";      break;
        case SignedInt16:      ret = "16-bit Signed Int";       break;
        case UnsignedInt16:    ret = "16-bit Unsigned Int";     break;
        case SignedInt32:      ret = "32-bit Signed Int";       break;
        case UnsignedInt32:    ret = "32-bit Unsigned Int";     break;
        case SignedInt64:      ret = "64-bit Signed Int";       break;
        case UnsignedInt64:    ret = "64-bit Unsigned Int";     break;
        case SSE_Quad:         ret = "128-bit SSE Register";    break;
    }
    return ret;
}

string FPOperand::FPOpVal2Str(FPOperandValue *val)
{
    stringstream ss("");
    ss << "{" << FPOpType2Str(val->type) << ": ";
    switch (val->type) {
        case IEEE_Single:      ss << val->data.flt;             break;
        case IEEE_Double:      ss << val->data.dbl;             break;
        case C99_LongDouble:   ss << val->data.ldbl;            break;
        case SignedInt8:       ss << val->data.sint8;           break;
        case UnsignedInt8:     ss << val->data.uint8;           break;
        case SignedInt16:      ss << val->data.sint16;          break;
        case UnsignedInt16:    ss << val->data.uint16;          break;
        case SignedInt32:      ss << val->data.sint32;          break;
        case UnsignedInt32:    ss << val->data.uint32;          break;
        case SignedInt64:      ss << val->data.sint64;          break;
        case UnsignedInt64:    ss << val->data.uint64;          break;
        case SSE_Quad:         ss << val->data.sse_quad[3] << " " << val->data.sse_quad[2] << " "
                                  << val->data.sse_quad[1] << " " << val->data.sse_quad[0];     break;
    }
    ss << "}";
    return ss.str();
}

void* FPOperand::FPOpValPtr(FPOperandValue *val)
{
    return val->data.ptr;
}

long FPOperand::FPOpValExp(FPOperandValue *val)
{
    int exp = 0;
    switch (val->type) {
        case IEEE_Single: frexp(val->data.flt, &exp); break;
        case IEEE_Double: frexp(val->data.dbl, &exp); break;
        case C99_LongDouble: frexp(val->data.ldbl, &exp); break;
        default: break;
    }
    return (long)exp;
}

float FPOperand::FPOpValF(FPOperandValue *val)
{
    float num = 0.0f;
    switch (val->type) {
        case IEEE_Single:    num = val->data.flt; break;
        case IEEE_Double:    num = (float)(val->data.dbl); break;
        case C99_LongDouble: num = (float)(val->data.ldbl); break;
        case SignedInt8:     num = (float)(val->data.sint8); break;
        case SignedInt16:    num = (float)(val->data.sint16); break;
        case SignedInt32:    num = (float)(val->data.sint32); break;
        case SignedInt64:    num = (float)(val->data.sint64); break;
        case UnsignedInt8:   num = (float)(val->data.uint8); break;
        case UnsignedInt16:  num = (float)(val->data.uint16); break;
        case UnsignedInt32:  num = (float)(val->data.uint32); break;
        case UnsignedInt64:  num = (float)(val->data.uint64); break;
        case SSE_Quad:       num = *(float*)(val->data.sse_quad); break;
    }
    return num;
}

double FPOperand::FPOpValD(FPOperandValue *val)
{
    double num = 0.0f;
    switch (val->type) {
        case IEEE_Single:    num = (double)(val->data.flt); break;
        case IEEE_Double:    num = val->data.dbl; break;
        case C99_LongDouble: num = (double)(val->data.ldbl); break;
        case SignedInt8:    num = (double)(val->data.sint8); break;
        case SignedInt16:    num = (double)(val->data.sint16); break;
        case SignedInt32:    num = (double)(val->data.sint32); break;
        case SignedInt64:    num = (double)(val->data.sint64); break;
        case UnsignedInt8:   num = (double)(val->data.uint8); break;
        case UnsignedInt16:  num = (double)(val->data.uint16); break;
        case UnsignedInt32:  num = (double)(val->data.uint32); break;
        case UnsignedInt64:  num = (double)(val->data.uint64); break;
        case SSE_Quad:       num = *(double*)(val->data.sse_quad); break;
    }
    return num;
}

long double FPOperand::FPOpValLD(FPOperandValue *val)
{
    long double num = 0.0f;
    switch (val->type) {
        case IEEE_Single:    num = (long double)(val->data.flt); break;
        case IEEE_Double:    num = (long double)(val->data.dbl); break;
        case C99_LongDouble: num = val->data.ldbl; break;
        case SignedInt8:     num = (long double)(val->data.sint8); break;
        case SignedInt16:    num = (long double)(val->data.sint16); break;
        case SignedInt32:    num = (long double)(val->data.sint32); break;
        case SignedInt64:    num = (long double)(val->data.sint64); break;
        case UnsignedInt8:   num = (long double)(val->data.uint8); break;
        case UnsignedInt16:  num = (long double)(val->data.uint16); break;
        case UnsignedInt32:  num = (long double)(val->data.uint32); break;
        case UnsignedInt64:  num = (long double)(val->data.uint64); break;
        case SSE_Quad:       num = (long double)(*(double*)(val->data.sse_quad)); break;
    }
    return num;
}

int8_t FPOperand::FPOpValSInt8(FPOperandValue *val)
{
    int8_t num = 0;
    switch (val->type) {
        case IEEE_Single:    num = (int8_t)(val->data.flt); break;
        case IEEE_Double:    num = (int8_t)(val->data.dbl); break;
        case C99_LongDouble: num = (int8_t)(val->data.ldbl); break;
        case SignedInt8:     num = (int8_t)(val->data.sint8); break;
        case SignedInt16:    num = (int8_t)(val->data.sint16); break;
        case SignedInt32:    num = (int8_t)(val->data.sint32); break;
        case SignedInt64:    num = (int8_t)(val->data.sint64); break;
        case UnsignedInt8:   num = (int8_t)(val->data.uint8); break;
        case UnsignedInt16:  num = (int8_t)(val->data.uint16); break;
        case UnsignedInt32:  num = (int8_t)(val->data.uint32); break;
        case UnsignedInt64:  num = (int8_t)(val->data.uint64); break;
        case SSE_Quad:       num = *(int8_t*)(val->data.sse_quad); break;
    }
    return num;
}

uint8_t FPOperand::FPOpValUInt8(FPOperandValue *val)
{
    uint8_t num = 0;
    switch (val->type) {
        case IEEE_Single:    num = (uint8_t)(val->data.flt); break;
        case IEEE_Double:    num = (uint8_t)(val->data.dbl); break;
        case C99_LongDouble: num = (uint8_t)(val->data.ldbl); break;
        case SignedInt8:     num = (uint8_t)(val->data.sint8); break;
        case SignedInt16:    num = (uint8_t)(val->data.sint16); break;
        case SignedInt32:    num = (uint8_t)(val->data.sint32); break;
        case SignedInt64:    num = (uint8_t)(val->data.sint64); break;
        case UnsignedInt8:   num = (uint8_t)(val->data.uint8); break;
        case UnsignedInt16:  num = (uint8_t)(val->data.uint16); break;
        case UnsignedInt32:  num = (uint8_t)(val->data.uint32); break;
        case UnsignedInt64:  num = (uint8_t)(val->data.uint64); break;
        case SSE_Quad:       num = *(uint8_t*)(val->data.sse_quad); break;
    }
    return num;
}

int16_t FPOperand::FPOpValSInt16(FPOperandValue *val)
{
    int16_t num = 0;
    switch (val->type) {
        case IEEE_Single:    num = (int16_t)(val->data.flt); break;
        case IEEE_Double:    num = (int16_t)(val->data.dbl); break;
        case C99_LongDouble: num = (int16_t)(val->data.ldbl); break;
        case SignedInt8:     num = (int16_t)(val->data.sint8); break;
        case SignedInt16:    num = (int16_t)(val->data.sint16); break;
        case SignedInt32:    num = (int16_t)(val->data.sint32); break;
        case SignedInt64:    num = (int16_t)(val->data.sint64); break;
        case UnsignedInt8:   num = (int16_t)(val->data.uint8); break;
        case UnsignedInt16:  num = (int16_t)(val->data.uint16); break;
        case UnsignedInt32:  num = (int16_t)(val->data.uint32); break;
        case UnsignedInt64:  num = (int16_t)(val->data.uint64); break;
        case SSE_Quad:       num = *(int16_t*)(val->data.sse_quad); break;
    }
    return num;
}

uint16_t FPOperand::FPOpValUInt16(FPOperandValue *val)
{
    uint16_t num = 0;
    switch (val->type) {
        case IEEE_Single:    num = (uint16_t)(val->data.flt); break;
        case IEEE_Double:    num = (uint16_t)(val->data.dbl); break;
        case C99_LongDouble: num = (uint16_t)(val->data.ldbl); break;
        case SignedInt8:     num = (uint16_t)(val->data.sint8); break;
        case SignedInt16:    num = (uint16_t)(val->data.sint16); break;
        case SignedInt32:    num = (uint16_t)(val->data.sint32); break;
        case SignedInt64:    num = (uint16_t)(val->data.sint64); break;
        case UnsignedInt8:   num = (uint16_t)(val->data.uint8); break;
        case UnsignedInt16:  num = (uint16_t)(val->data.uint16); break;
        case UnsignedInt32:  num = (uint16_t)(val->data.uint32); break;
        case UnsignedInt64:  num = (uint16_t)(val->data.uint64); break;
        case SSE_Quad:       num = *(uint16_t*)(val->data.sse_quad); break;
    }
    return num;
}

int32_t FPOperand::FPOpValSInt32(FPOperandValue *val)
{
    int32_t num = 0;
    switch (val->type) {
        case IEEE_Single:    num = (int32_t)(val->data.flt); break;
        case IEEE_Double:    num = (int32_t)(val->data.dbl); break;
        case C99_LongDouble: num = (int32_t)(val->data.ldbl); break;
        case SignedInt8:     num = (int32_t)(val->data.sint8); break;
        case SignedInt16:    num = (int32_t)(val->data.sint16); break;
        case SignedInt32:    num = (int32_t)(val->data.sint32); break;
        case SignedInt64:    num = (int32_t)(val->data.sint64); break;
        case UnsignedInt8:   num = (int32_t)(val->data.uint8); break;
        case UnsignedInt16:  num = (int32_t)(val->data.uint16); break;
        case UnsignedInt32:  num = (int32_t)(val->data.uint32); break;
        case UnsignedInt64:  num = (int32_t)(val->data.uint64); break;
        case SSE_Quad:       num = *(int32_t*)(val->data.sse_quad); break;
    }
    return num;
}

uint32_t FPOperand::FPOpValUInt32(FPOperandValue *val)
{
    uint32_t num = 0;
    switch (val->type) {
        case IEEE_Single:    num = (uint32_t)(val->data.flt); break;
        case IEEE_Double:    num = (uint32_t)(val->data.dbl); break;
        case C99_LongDouble: num = (uint32_t)(val->data.ldbl); break;
        case SignedInt8:     num = (uint32_t)(val->data.sint8); break;
        case SignedInt16:    num = (uint32_t)(val->data.sint16); break;
        case SignedInt32:    num = (uint32_t)(val->data.sint32); break;
        case SignedInt64:    num = (uint32_t)(val->data.sint64); break;
        case UnsignedInt8:   num = (uint32_t)(val->data.uint8); break;
        case UnsignedInt16:  num = (uint32_t)(val->data.uint16); break;
        case UnsignedInt32:  num = (uint32_t)(val->data.uint32); break;
        case UnsignedInt64:  num = (uint32_t)(val->data.uint64); break;
        case SSE_Quad:       num = *(uint32_t*)(val->data.sse_quad); break;
    }
    return num;
}

int64_t FPOperand::FPOpValSInt64(FPOperandValue *val)
{
    int64_t num = 0;
    switch (val->type) {
        case IEEE_Single:    num = (int64_t)(val->data.flt); break;
        case IEEE_Double:    num = (int64_t)(val->data.dbl); break;
        case C99_LongDouble: num = (int64_t)(val->data.ldbl); break;
        case SignedInt8:     num = (int64_t)(val->data.sint8); break;
        case SignedInt16:    num = (int64_t)(val->data.sint16); break;
        case SignedInt32:    num = (int64_t)(val->data.sint32); break;
        case SignedInt64:    num = (int64_t)(val->data.sint64); break;
        case UnsignedInt8:   num = (int64_t)(val->data.uint8); break;
        case UnsignedInt16:  num = (int64_t)(val->data.uint16); break;
        case UnsignedInt32:  num = (int64_t)(val->data.uint32); break;
        case UnsignedInt64:  num = (int64_t)(val->data.uint64); break;
        case SSE_Quad:       num = *(int64_t*)(val->data.sse_quad); break;
    }
    return num;
}

uint64_t FPOperand::FPOpValUInt64(FPOperandValue *val)
{
    uint64_t num = 0;
    switch (val->type) {
        case IEEE_Single:    num = (uint64_t)(val->data.flt); break;
        case IEEE_Double:    num = (uint64_t)(val->data.dbl); break;
        case C99_LongDouble: num = (uint64_t)(val->data.ldbl); break;
        case SignedInt8:     num = (uint64_t)(val->data.sint8); break;
        case SignedInt16:    num = (uint64_t)(val->data.sint16); break;
        case SignedInt32:    num = (uint64_t)(val->data.sint32); break;
        case SignedInt64:    num = (uint64_t)(val->data.sint64); break;
        case UnsignedInt8:   num = (uint64_t)(val->data.uint8); break;
        case UnsignedInt16:  num = (uint64_t)(val->data.uint16); break;
        case UnsignedInt32:  num = (uint64_t)(val->data.uint32); break;
        case UnsignedInt64:  num = (uint64_t)(val->data.uint64); break;
        case SSE_Quad:       num = *(uint64_t*)(val->data.sse_quad); break;
    }
    return num;
}

FPOperand::FPOperand()
    : type(IEEE_Single), reg(REG_NONE), tag(0), base(REG_NONE), index(REG_NONE), 
      disp(0), scale(1), segment(REG_NONE), immediate(true), inverted(false)
{
    currentAddress = 0;
    currentValue.type = IEEE_Single;
    currentValue.data.flt = 0.0f;
    updateAttributes();
}

FPOperand::FPOperand(FPOperand *baseOp, long tag)
    : type(baseOp->type), reg(baseOp->reg), tag(tag), base(baseOp->base), 
      index(baseOp->index), disp(baseOp->disp), scale(baseOp->scale), 
      segment(REG_NONE), immediate(baseOp->immediate), inverted(baseOp->inverted)
{
    currentAddress = baseOp->currentAddress;
    currentValue = baseOp->currentValue;
    updateAttributes();
}


FPOperand::FPOperand(FPOperandValue value)
    : type(value.type), reg(REG_NONE), tag(0), base(REG_NONE), 
      index(REG_NONE), disp(0), scale(1), 
      segment(REG_NONE), immediate(true), inverted(false)
{
    currentAddress = 0;
    switch (value.type) {
        case IEEE_Single: currentValue.data.flt = value.data.flt; break;
        case IEEE_Double: currentValue.data.dbl = value.data.dbl; break;
        case C99_LongDouble: currentValue.data.ldbl = value.data.ldbl; break;
        case SignedInt8: currentValue.data.sint8 = value.data.sint8; break;
        case SignedInt16: currentValue.data.sint16 = value.data.sint16; break;
        case SignedInt32: currentValue.data.sint32 = value.data.sint32; break;
        case SignedInt64: currentValue.data.sint64 = value.data.sint64; break;
        case UnsignedInt8: currentValue.data.uint8 = value.data.uint8; break;
        case UnsignedInt16: currentValue.data.uint16 = value.data.uint16; break;
        case UnsignedInt32: currentValue.data.uint32 = value.data.uint32; break;
        case UnsignedInt64: currentValue.data.uint64 = value.data.uint64; break;
        case SSE_Quad: currentValue.data.sse_quad[0] = value.data.sse_quad[0];
                       currentValue.data.sse_quad[1] = value.data.sse_quad[1];
                       currentValue.data.sse_quad[2] = value.data.sse_quad[2];
                       currentValue.data.sse_quad[3] = value.data.sse_quad[3];
                       break;
    }
    updateAttributes();
}

FPOperand::FPOperand(float value)
    : type(IEEE_Single), reg(REG_NONE), tag(0), base(REG_NONE), 
      index(REG_NONE), disp(0), scale(1), 
      segment(REG_NONE), immediate(true), inverted(false)
{
    currentAddress = 0;
    currentValue.type = type;
    currentValue.data.flt = value;
    updateAttributes();
}

FPOperand::FPOperand(double value)
    : type(IEEE_Double), reg(REG_NONE), tag(0), base(REG_NONE), 
      index(REG_NONE), disp(0), scale(1), 
      segment(REG_NONE), immediate(true), inverted(false)
{
    currentAddress = 0;
    currentValue.type = type;
    currentValue.data.dbl = value;
    updateAttributes();
}

FPOperand::FPOperand(long double value)
    : type(C99_LongDouble), reg(REG_NONE), tag(0), base(REG_NONE), 
      index(REG_NONE), disp(0), scale(1), 
      segment(REG_NONE), immediate(true), inverted(false)
{
    currentAddress = 0;
    currentValue.type = type;
    currentValue.data.ldbl = value;
    updateAttributes();
}

FPOperand::FPOperand(int8_t value)
    : type(SignedInt32), reg(REG_NONE), tag(0), base(REG_NONE), 
      index(REG_NONE), disp(0), scale(1), 
      segment(REG_NONE), immediate(true), inverted(false)
{
    currentAddress = 0;
    currentValue.type = type;
    currentValue.data.sint8 = value;
    updateAttributes();
}

FPOperand::FPOperand(int16_t value)
    : type(SignedInt32), reg(REG_NONE), tag(0), base(REG_NONE), 
      index(REG_NONE), disp(0), scale(1), 
      segment(REG_NONE), immediate(true), inverted(false)
{
    currentAddress = 0;
    currentValue.type = type;
    currentValue.data.sint16 = value;
    updateAttributes();
}

FPOperand::FPOperand(int32_t value)
    : type(SignedInt32), reg(REG_NONE), tag(0), base(REG_NONE), 
      index(REG_NONE), disp(0), scale(1), 
      segment(REG_NONE), immediate(true), inverted(false)
{
    currentAddress = 0;
    currentValue.type = type;
    currentValue.data.sint32 = value;
    updateAttributes();
}

FPOperand::FPOperand(int64_t value)
    : type(SignedInt64), reg(REG_NONE), tag(0), base(REG_NONE), 
      index(REG_NONE), disp(0), scale(1), 
      segment(REG_NONE), immediate(true), inverted(false)
{
    currentAddress = 0;
    currentValue.type = type;
    currentValue.data.sint64 = value;
    updateAttributes();
}

FPOperand::FPOperand(uint8_t value)
    : type(UnsignedInt32), reg(REG_NONE), tag(0), base(REG_NONE), 
      index(REG_NONE), disp(0), scale(1), 
      segment(REG_NONE), immediate(true), inverted(false)
{
    currentAddress = 0;
    currentValue.type = type;
    currentValue.data.uint8 = value;
    updateAttributes();
}

FPOperand::FPOperand(uint16_t value)
    : type(UnsignedInt32), reg(REG_NONE), tag(0), base(REG_NONE), 
      index(REG_NONE), disp(0), scale(1), 
      segment(REG_NONE), immediate(true), inverted(false)
{
    currentAddress = 0;
    currentValue.type = type;
    currentValue.data.uint16 = value;
    updateAttributes();
}

FPOperand::FPOperand(uint32_t value)
    : type(UnsignedInt32), reg(REG_NONE), tag(0), base(REG_NONE), 
      index(REG_NONE), disp(0), scale(1), 
      segment(REG_NONE), immediate(true), inverted(false)
{
    currentAddress = 0;
    currentValue.type = type;
    currentValue.data.uint32 = value;
    updateAttributes();
}

FPOperand::FPOperand(uint64_t value)
    : type(UnsignedInt64), reg(REG_NONE), tag(0), base(REG_NONE), 
      index(REG_NONE), disp(0), scale(1), 
      segment(REG_NONE), immediate(true), inverted(false)
{
    currentAddress = 0;
    currentValue.type = type;
    currentValue.data.uint64 = value;
    updateAttributes();
}

FPOperand::FPOperand(FPOperandType type)
    : type(type), reg(REG_NONE), tag(0), base(REG_NONE), 
      index(REG_NONE), disp(0), scale(1), 
      segment(REG_NONE), immediate(false), inverted(false)
{
    currentAddress = 0;
    currentValue.type = type;
    updateAttributes();
}

FPOperand::FPOperand(FPOperandType type, FPRegister reg)
    : type(type), reg(reg), tag(0), base(REG_NONE), 
      index(REG_NONE), disp(0), scale(1), 
      segment(REG_NONE), immediate(false), inverted(false)
{
    currentAddress = FPRegTag2Addr(reg, 0);
    currentValue.type = type;
    updateAttributes();
}

FPOperand::FPOperand(FPOperandType type, FPRegister reg, long tag)
    : type(type), reg(reg), tag(tag), base(REG_NONE), 
      index(REG_NONE), disp(0), scale(1), 
      segment(REG_NONE), immediate(false), inverted(false)
{
    currentAddress = FPRegTag2Addr(reg, tag);
    currentValue.type = type;
    updateAttributes();
}

FPOperand::FPOperand(FPOperandType type, FPRegister base, FPRegister index, long disp, long scale)
   : type(type), reg(REG_NONE), tag(0), base(base), 
     index(index), disp(disp), scale(scale), 
     segment(REG_NONE), immediate(false), inverted(false)
{
    currentAddress = 0;
    currentValue.type = type;
    updateAttributes();
}

FPOperand::FPOperand(FPOperandType type, FPRegister base, FPRegister index, long disp, long scale, long tag)
   : type(type), reg(REG_NONE), tag(tag), base(base), 
     index(index), disp(disp), scale(scale), 
     segment(REG_NONE), immediate(false), inverted(false)
{
    currentAddress = 0;
    currentValue.type = type;
    updateAttributes();
}

FPOperand::FPOperand(FPOperandType type, FPRegister base, FPRegister index, long disp, long scale, FPRegister segment, long tag)
   : type(type), reg(REG_NONE), tag(tag), base(base), 
     index(index), disp(disp), scale(scale), 
     segment(segment), immediate(false), inverted(false)
{
    currentAddress = 0;
    currentValue.type = type;
    updateAttributes();
}

void FPOperand::updateAttributes()
{
    attrIsMemory = isMemory();
    attrIsRegister = isRegister();
    attrIsImmediate = isImmediate();
    attrIsStack = isStack();
    attrIsLocalVar = isLocalVar();
    attrIsRegisterSSE = isRegisterSSE();
    attrIsRegisterST = isRegisterST();
    attrIsRegisterGPR = isRegisterGPR();
    attrIsGlobalVar = isGlobalVar();
}

void FPOperand::FPInvBytes(void *op, void *ret, long size)
{
    long i;
    for (i=0; i<size; i++) {
        *((unsigned char*)ret+i) = ~ *((unsigned char*)op+i);
    }
}

void FPOperand::FPAndBytes(void *op1, void *op2, void *ret, long size)
{
    long i;
    for (i=0; i<size; i++) {
        *((unsigned char*)ret+i) = *((unsigned char*)op1+i) & *((unsigned char*)op2+i);
    }
}

void FPOperand::FPOrBytes(void *op1, void *op2, void *ret, long size)
{
    long i;
    for (i=0; i<size; i++) {
        *((unsigned char*)ret+i) = *((unsigned char*)op1+i) | *((unsigned char*)op2+i);
    }
}

void FPOperand::FPXorBytes(void *op1, void *op2, void *ret, long size)
{
    long i;
    for (i=0; i<size; i++) {
        *((unsigned char*)ret+i) = *((unsigned char*)op1+i) ^ *((unsigned char*)op2+i);
    }
}

void FPOperand::refresh(FPContext *context)
{
    if (immediate) return;
    if (reg != REG_NONE) { // && base == REG_NONE && index == REG_NONE && disp == 0) {
        switch (currentValue.type) {
            case IEEE_Single:    context->getRegisterValue((void*)&currentValue.data, reg, tag,  4); break;
            case IEEE_Double:    context->getRegisterValue((void*)&currentValue.data, reg, tag,  8); break;
            case C99_LongDouble: context->getRegisterValue((void*)&currentValue.data, reg, tag, 12); break;
            case SignedInt8:     context->getRegisterValue((void*)&currentValue.data, reg, tag,  1); break;
            case SignedInt16:    context->getRegisterValue((void*)&currentValue.data, reg, tag,  2); break;
            case SignedInt32:    context->getRegisterValue((void*)&currentValue.data, reg, tag,  4); break;
            case SignedInt64:    context->getRegisterValue((void*)&currentValue.data, reg, tag,  8); break;
            case UnsignedInt8:   context->getRegisterValue((void*)&currentValue.data, reg, tag,  1); break;
            case UnsignedInt16:  context->getRegisterValue((void*)&currentValue.data, reg, tag,  2); break;
            case UnsignedInt32:  context->getRegisterValue((void*)&currentValue.data, reg, tag,  4); break;
            case UnsignedInt64:  context->getRegisterValue((void*)&currentValue.data, reg, tag,  8); break;
            case SSE_Quad:       context->getRegisterValue((void*)&currentValue.data, reg, tag, 16); break;
        }
    } else {
        assert(segment == REG_NONE);
        switch (currentValue.type) {
            case IEEE_Single:    context->getMemoryValue((void*)&currentValue.data, &currentAddress, base, index, disp+tag*4, scale,  4); break;
            case IEEE_Double:    context->getMemoryValue((void*)&currentValue.data, &currentAddress, base, index, disp+tag*4, scale,  8); break;
            case C99_LongDouble: context->getMemoryValue((void*)&currentValue.data, &currentAddress, base, index, disp+tag*4, scale, 12); break;
            case SignedInt8:     context->getMemoryValue((void*)&currentValue.data, &currentAddress, base, index, disp+tag*4, scale,  1); break;
            case SignedInt16:    context->getMemoryValue((void*)&currentValue.data, &currentAddress, base, index, disp+tag*4, scale,  2); break;
            case SignedInt32:    context->getMemoryValue((void*)&currentValue.data, &currentAddress, base, index, disp+tag*4, scale,  4); break;
            case SignedInt64:    context->getMemoryValue((void*)&currentValue.data, &currentAddress, base, index, disp+tag*4, scale,  8); break;
            case UnsignedInt8:   context->getMemoryValue((void*)&currentValue.data, &currentAddress, base, index, disp+tag*4, scale,  1); break;
            case UnsignedInt16:  context->getMemoryValue((void*)&currentValue.data, &currentAddress, base, index, disp+tag*4, scale,  2); break;
            case UnsignedInt32:  context->getMemoryValue((void*)&currentValue.data, &currentAddress, base, index, disp+tag*4, scale,  4); break;
            case UnsignedInt64:  context->getMemoryValue((void*)&currentValue.data, &currentAddress, base, index, disp+tag*4, scale,  8); break;
            case SSE_Quad:       context->getMemoryValue((void*)&currentValue.data, &currentAddress, base, index, disp+tag*4, scale, 16); break;
        }
    }
    if (inverted) {
        switch (currentValue.type) {
            case IEEE_Single:    FPInvBytes(&currentValue.data.flt,    &currentValue.data.flt,     4);   break;
            case IEEE_Double:    FPInvBytes(&currentValue.data.dbl,    &currentValue.data.dbl,     8);   break;
            case C99_LongDouble: FPInvBytes(&currentValue.data.ldbl,   &currentValue.data.ldbl,   12);   break;
            case SignedInt8:     FPInvBytes(&currentValue.data.sint8,  &currentValue.data.sint8,   1);   break;
            case SignedInt16:    FPInvBytes(&currentValue.data.sint16, &currentValue.data.sint16,  2);   break;
            case SignedInt32:    FPInvBytes(&currentValue.data.sint32, &currentValue.data.sint32,  4);   break;
            case SignedInt64:    FPInvBytes(&currentValue.data.sint64, &currentValue.data.sint64,  8);   break;
            case UnsignedInt8:   FPInvBytes(&currentValue.data.uint8,  &currentValue.data.uint8,   1);   break;
            case UnsignedInt16:  FPInvBytes(&currentValue.data.uint16, &currentValue.data.uint16,  2);   break;
            case UnsignedInt32:  FPInvBytes(&currentValue.data.uint32, &currentValue.data.uint32,  4);   break;
            case UnsignedInt64:  FPInvBytes(&currentValue.data.uint64, &currentValue.data.uint64,  8);   break;
            case SSE_Quad:       FPInvBytes(currentValue.data.sse_quad,currentValue.data.sse_quad,16);   break;
        }
    }
    //printf("refreshed: %s\n", toStringV().c_str());
}

void FPOperand::refreshAddress(FPContext *context)
{
    if (immediate) return;
    // register "addresses" are hard-coded, so this is only necessary if it's a
    // memory operand
    if (reg == REG_NONE) {
        assert(segment == REG_NONE);
        switch (currentValue.type) {
            case IEEE_Single:    context->getMemoryAddress((void**)&currentAddress, base, index, disp+tag*4, scale,  4); break;
            case IEEE_Double:    context->getMemoryAddress((void**)&currentAddress, base, index, disp+tag*4, scale,  8); break;
            case C99_LongDouble: context->getMemoryAddress((void**)&currentAddress, base, index, disp+tag*4, scale, 12); break;
            case SignedInt8:     context->getMemoryAddress((void**)&currentAddress, base, index, disp+tag*4, scale,  1); break;
            case SignedInt16:    context->getMemoryAddress((void**)&currentAddress, base, index, disp+tag*4, scale,  2); break;
            case SignedInt32:    context->getMemoryAddress((void**)&currentAddress, base, index, disp+tag*4, scale,  4); break;
            case SignedInt64:    context->getMemoryAddress((void**)&currentAddress, base, index, disp+tag*4, scale,  8); break;
            case UnsignedInt8:   context->getMemoryAddress((void**)&currentAddress, base, index, disp+tag*4, scale,  1); break;
            case UnsignedInt16:  context->getMemoryAddress((void**)&currentAddress, base, index, disp+tag*4, scale,  2); break;
            case UnsignedInt32:  context->getMemoryAddress((void**)&currentAddress, base, index, disp+tag*4, scale,  4); break;
            case UnsignedInt64:  context->getMemoryAddress((void**)&currentAddress, base, index, disp+tag*4, scale,  8); break;
            case SSE_Quad:       context->getMemoryAddress((void**)&currentAddress, base, index, disp+tag*4, scale, 16); break;
        }
    }
    //printf("refreshed address only: %s\n", toString().c_str());
}

void FPOperand::refreshValue(FPContext *context)
{
    if (immediate) return;
    if (reg != REG_NONE) { // && base == REG_NONE && index == REG_NONE && disp == 0) {
        switch (currentValue.type) {
            case IEEE_Single:    context->getRegisterValue((void*)&currentValue.data, reg, tag,  4); break;
            case IEEE_Double:    context->getRegisterValue((void*)&currentValue.data, reg, tag,  8); break;
            case C99_LongDouble: context->getRegisterValue((void*)&currentValue.data, reg, tag, 12); break;
            case SignedInt8:     context->getRegisterValue((void*)&currentValue.data, reg, tag,  1); break;
            case SignedInt16:    context->getRegisterValue((void*)&currentValue.data, reg, tag,  2); break;
            case SignedInt32:    context->getRegisterValue((void*)&currentValue.data, reg, tag,  4); break;
            case SignedInt64:    context->getRegisterValue((void*)&currentValue.data, reg, tag,  8); break;
            case UnsignedInt8:   context->getRegisterValue((void*)&currentValue.data, reg, tag,  1); break;
            case UnsignedInt16:  context->getRegisterValue((void*)&currentValue.data, reg, tag,  2); break;
            case UnsignedInt32:  context->getRegisterValue((void*)&currentValue.data, reg, tag,  4); break;
            case UnsignedInt64:  context->getRegisterValue((void*)&currentValue.data, reg, tag,  8); break;
            case SSE_Quad:       context->getRegisterValue((void*)&currentValue.data, reg, tag, 16); break;
        }
    } else {
        assert(segment == REG_NONE);
        switch (currentValue.type) {
            case IEEE_Single:    context->getMemoryValue((void*)&currentValue.data, currentAddress,  4); break;
            case IEEE_Double:    context->getMemoryValue((void*)&currentValue.data, currentAddress,  8); break;
            case C99_LongDouble: context->getMemoryValue((void*)&currentValue.data, currentAddress, 12); break;
            case SignedInt8:     context->getMemoryValue((void*)&currentValue.data, currentAddress,  1); break;
            case SignedInt16:    context->getMemoryValue((void*)&currentValue.data, currentAddress,  2); break;
            case SignedInt32:    context->getMemoryValue((void*)&currentValue.data, currentAddress,  4); break;
            case SignedInt64:    context->getMemoryValue((void*)&currentValue.data, currentAddress,  8); break;
            case UnsignedInt8:   context->getMemoryValue((void*)&currentValue.data, currentAddress,  1); break;
            case UnsignedInt16:  context->getMemoryValue((void*)&currentValue.data, currentAddress,  2); break;
            case UnsignedInt32:  context->getMemoryValue((void*)&currentValue.data, currentAddress,  4); break;
            case UnsignedInt64:  context->getMemoryValue((void*)&currentValue.data, currentAddress,  8); break;
            case SSE_Quad:       context->getMemoryValue((void*)&currentValue.data, currentAddress, 16); break;
        }
    }
    if (inverted) {
        switch (currentValue.type) {
            case IEEE_Single:    FPInvBytes(&currentValue.data.flt,    &currentValue.data.flt,     4);   break;
            case IEEE_Double:    FPInvBytes(&currentValue.data.dbl,    &currentValue.data.dbl,     8);   break;
            case C99_LongDouble: FPInvBytes(&currentValue.data.ldbl,   &currentValue.data.ldbl,   12);   break;
            case SignedInt8:     FPInvBytes(&currentValue.data.sint8,  &currentValue.data.sint8,   1);   break;
            case SignedInt16:    FPInvBytes(&currentValue.data.sint16, &currentValue.data.sint16,  2);   break;
            case SignedInt32:    FPInvBytes(&currentValue.data.sint32, &currentValue.data.sint32,  4);   break;
            case SignedInt64:    FPInvBytes(&currentValue.data.sint64, &currentValue.data.sint64,  8);   break;
            case UnsignedInt8:   FPInvBytes(&currentValue.data.uint8,  &currentValue.data.uint8,   1);   break;
            case UnsignedInt16:  FPInvBytes(&currentValue.data.uint16, &currentValue.data.uint16,  2);   break;
            case UnsignedInt32:  FPInvBytes(&currentValue.data.uint32, &currentValue.data.uint32,  4);   break;
            case UnsignedInt64:  FPInvBytes(&currentValue.data.uint64, &currentValue.data.uint64,  8);   break;
            case SSE_Quad:       FPInvBytes(currentValue.data.sse_quad,currentValue.data.sse_quad,16);   break;
        }
    }
    //printf("refreshed value only: %s\n", toStringV().c_str());
}

void FPOperand::setType(FPOperandType type)
{
    this->type = type;
}

FPOperandType FPOperand::getType()
{
    return type;
}

FPRegister FPOperand::getRegister()
{
    return reg;
}

FPRegister FPOperand::getBase()
{
    return base;
}

FPRegister FPOperand::getIndex()
{
    return index;
}

long FPOperand::getDisp()
{
    return disp;
}

long FPOperand::getScale()
{
    return scale;
}

FPRegister FPOperand::getSegment()
{
    return segment;
}

long FPOperand::getTag()
{
    return tag;
}

FPOperandAddress FPOperand::getCurrentAddress()
{
    return currentAddress;
}

FPOperandValue FPOperand::getCurrentValue()
{
    return currentValue;
}

void* FPOperand::getCurrentValuePtr()
{
    return FPOpValPtr(&currentValue);
}

long FPOperand::getCurrentValueExp()
{
    return FPOpValExp(&currentValue);
}

float FPOperand::getCurrentValueF()
{
    return FPOpValF(&currentValue);
}

double FPOperand::getCurrentValueD()
{
    return FPOpValD(&currentValue);
}

long double FPOperand::getCurrentValueLD()
{
    return FPOpValLD(&currentValue);
}

int8_t FPOperand::getCurrentValueSInt8()
{
    return FPOpValSInt8(&currentValue);
}

int16_t FPOperand::getCurrentValueSInt16()
{
    return FPOpValSInt16(&currentValue);
}

int32_t FPOperand::getCurrentValueSInt32()
{
    return FPOpValSInt32(&currentValue);
}

int64_t FPOperand::getCurrentValueSInt64()
{
    return FPOpValSInt64(&currentValue);
}

uint8_t FPOperand::getCurrentValueUInt8()
{
    return FPOpValUInt8(&currentValue);
}

uint16_t FPOperand::getCurrentValueUInt16()
{
    return FPOpValUInt16(&currentValue);
}

uint32_t FPOperand::getCurrentValueUInt32()
{
    return FPOpValUInt32(&currentValue);
}

uint64_t FPOperand::getCurrentValueUInt64()
{
    return FPOpValUInt64(&currentValue);
}

void FPOperand::setCurrentValueZero(FPContext *context, bool writeBack, bool queue)
{
    switch (type) {
        case IEEE_Single:    setCurrentValueF(0.0f, context, writeBack, queue); break;
        case IEEE_Double:    setCurrentValueD(0.0, context, writeBack, queue); break;
        case C99_LongDouble: setCurrentValueLD(0.0L, context, writeBack, queue); break;
        case SignedInt8:     setCurrentValueSInt8(0, context, writeBack, queue); break;
        case SignedInt16:    setCurrentValueSInt16(0, context, writeBack, queue); break;
        case SignedInt32:    setCurrentValueSInt32(0, context, writeBack, queue); break;
        case SignedInt64:    setCurrentValueSInt64(0L, context, writeBack, queue); break;
        case UnsignedInt8:   setCurrentValueUInt8(0, context, writeBack, queue); break;
        case UnsignedInt16:  setCurrentValueUInt16(0, context, writeBack, queue); break;
        case UnsignedInt32:  setCurrentValueUInt32(0, context, writeBack, queue); break;
        case UnsignedInt64:  setCurrentValueUInt64(0L, context, writeBack, queue); break;
        case SSE_Quad:
            if (attrIsMemory) {
                assert(segment == REG_NONE);
                if (writeBack) {
                    context->setMemoryValueUInt32((void*)((unsigned long)currentAddress+0), 0, queue);
                    context->setMemoryValueUInt32((void*)((unsigned long)currentAddress+1), 0, queue);
                    context->setMemoryValueUInt32((void*)((unsigned long)currentAddress+2), 0, queue);
                    context->setMemoryValueUInt32((void*)((unsigned long)currentAddress+3), 0, queue);
                }
                if (!queue) {
                    currentValue.data.sse_quad[0] = 0;
                    currentValue.data.sse_quad[1] = 0;
                    currentValue.data.sse_quad[2] = 0;
                    currentValue.data.sse_quad[3] = 0;
                }
            } else if (attrIsRegister) {
                if (writeBack) {
                    context->setRegisterValueUInt32(reg, 0, 0, queue);
                    context->setRegisterValueUInt32(reg, 1, 0, queue);
                    context->setRegisterValueUInt32(reg, 2, 0, queue);
                    context->setRegisterValueUInt32(reg, 3, 0, queue);
                }
                if (!queue) {
                    currentValue.data.sse_quad[0] = 0;
                    currentValue.data.sse_quad[1] = 0;
                    currentValue.data.sse_quad[2] = 0;
                    currentValue.data.sse_quad[3] = 0;
                }
            } else {
                printf("  trying to set UNHANDLED OPERAND: %s\n", toStringV().c_str());
            }
            break;
    }
}

void FPOperand::setCurrentValuePtr(void* val, FPContext *context, bool writeBack, bool queue)
{
    // check pointer size and redirect to appropriate 32- or 64-bit version
#ifdef __X86_64__
    setCurrentValueUInt64((uint64_t)val, context, writeBack, queue);
#else
    setCurrentValueUInt32((uint32_t)val, context, writeBack, queue);
#endif
}

void FPOperand::setCurrentValueF(float val, FPContext *context, bool writeBack, bool queue)
{
    setCurrentValueUInt32(*(uint32_t*)(&val), context, writeBack, queue);
}

void FPOperand::setCurrentValueD(double val, FPContext *context, bool writeBack, bool queue)
{
    setCurrentValueUInt64(*(uint64_t*)(&val), context, writeBack, queue);
}

void FPOperand::setCurrentValueLD(long double /*val*/, FPContext * /*context*/, bool /*writeBack*/, bool /*queue*/)
{
    fprintf(stderr, "ERROR: trying to save long double value (not implemented)\n");
    assert(0);
}

void FPOperand::setCurrentValueSInt8(int8_t val, FPContext *context, bool writeBack, bool queue)
{
    setCurrentValueUInt8(*(uint8_t*)(&val), context, writeBack, queue);
}

void FPOperand::setCurrentValueSInt16(int16_t val, FPContext *context, bool writeBack, bool queue)
{
    setCurrentValueUInt16(*(uint16_t*)(&val), context, writeBack, queue);
}

void FPOperand::setCurrentValueSInt32(int32_t val, FPContext *context, bool writeBack, bool queue)
{
    setCurrentValueUInt32(*(uint32_t*)(&val), context, writeBack, queue);
}

void FPOperand::setCurrentValueSInt64(int64_t val, FPContext *context, bool writeBack, bool queue)
{
    setCurrentValueUInt64(*(uint64_t*)(&val), context, writeBack, queue);
}

void FPOperand::setCurrentValueUInt8(uint8_t val, FPContext *context, bool writeBack, bool queue)
{
    if (attrIsMemory) {
        assert(segment == REG_NONE);
#if INCLUDE_DEBUG
        //printf("  setting operand value at 0x%p = %x %s %s\n", 
              //(void*)currentAddress, val,
              //(writeBack ? "[writeBack]" : ""),
              //(queue ? "[queue]" : ""));
#endif
        if (writeBack) {
            refreshAddress(context);
            context->setMemoryValueUInt8((void*)currentAddress, val, queue);
        }
        if (!queue) {
            currentValue.data.uint8 = val;
        }
    } else if (attrIsRegister) {
#if INCLUDE_DEBUG
        //printf("  setting operand value of %s = %x %s %s\n", 
              //FPContext::FPReg2Str(reg).c_str(), val,
              //(writeBack ? "[writeBack]" : ""),
              //(queue ? "[queue]" : ""));
#endif
        if (writeBack) {
            context->setRegisterValueUInt8(reg, tag, val, queue);
        }
        if (!queue) {
            currentValue.data.uint8 = val;
        }
    } else if (isImmediate()) {
        if (!queue) {
            currentValue.data.uint8 = val;
        }
    } else {
        printf("  trying to set UNHANDLED OPERAND: %s\n", toStringV().c_str());
    }
}

void FPOperand::setCurrentValueUInt16(uint16_t val, FPContext *context, bool writeBack, bool queue)
{
    if (attrIsMemory) {
        assert(segment == REG_NONE);
#if INCLUDE_DEBUG
        //printf("  setting operand value at 0x%p = 0x%lx %s %s\n", 
              //(void*)currentAddress, val,
              //(writeBack ? "[writeBack]" : ""),
              //(queue ? "[queue]" : ""));
#endif
        if (writeBack) {
            refreshAddress(context);
            context->setMemoryValueUInt16((void*)currentAddress, val, queue);
        }
        if (!queue) {
            currentValue.data.uint16 = val;
        }
    } else if (attrIsRegister) {
#if INCLUDE_DEBUG
        //printf("  setting operand value of %s = %lx %s %s\n", 
              //FPContext::FPReg2Str(reg).c_str(), val,
              //(writeBack ? "[writeBack]" : ""),
              //(queue ? "[queue]" : ""));
#endif
        if (writeBack) {
            context->setRegisterValueUInt16(reg, tag, val, queue);
        }
        if (!queue) {
            currentValue.data.uint16 = val;
        }
    } else if (isImmediate()) {
        if (!queue) {
            currentValue.data.uint16 = val;
        }
    } else {
        printf("  trying to set UNHANDLED OPERAND: %s\n", toStringV().c_str());
    }
}

void FPOperand::setCurrentValueUInt32(uint32_t val, FPContext *context, bool writeBack, bool queue)
{
    if (attrIsMemory) {
        assert(segment == REG_NONE);
#if INCLUDE_DEBUG
        //printf("  setting operand value at 0x%p = %x %s %s\n", 
              //(void*)currentAddress, val,
              //(writeBack ? "[writeBack]" : ""),
              //(queue ? "[queue]" : ""));
#endif
        if (writeBack) {
            refreshAddress(context);
            context->setMemoryValueUInt32((void*)currentAddress, val, queue);
        }
        if (!queue) {
            currentValue.data.uint32 = val;
        }
    } else if (attrIsRegister) {
#if INCLUDE_DEBUG
        //printf("  setting operand value of %s = %x %s %s\n", 
              //FPContext::FPReg2Str(reg).c_str(), val,
              //(writeBack ? "[writeBack]" : ""),
              //(queue ? "[queue]" : ""));
#endif
        if (writeBack) {
            context->setRegisterValueUInt32(reg, tag, val, queue);
        }
        if (!queue) {
            currentValue.data.uint32 = val;
        }
    } else if (isImmediate()) {
        if (!queue) {
            currentValue.data.uint32 = val;
        }
    } else {
        printf("  trying to set UNHANDLED OPERAND: %s\n", toStringV().c_str());
    }
}

void FPOperand::setCurrentValueUInt64(uint64_t val, FPContext *context, bool writeBack, bool queue)
{
    if (attrIsMemory) {
        assert(segment == REG_NONE);
#if INCLUDE_DEBUG
        //printf("  setting operand value at 0x%p = 0x%lx %s %s\n", 
              //(void*)currentAddress, val,
              //(writeBack ? "[writeBack]" : ""),
              //(queue ? "[queue]" : ""));
#endif
        if (writeBack) {
            refreshAddress(context);
            context->setMemoryValueUInt64((void*)currentAddress, val, queue);
        }
        if (!queue) {
            currentValue.data.uint64 = val;
        }
    } else if (attrIsRegister) {
#if INCLUDE_DEBUG
        //printf("  setting operand value of %s = %lx %s %s\n", 
              //FPContext::FPReg2Str(reg).c_str(), val,
              //(writeBack ? "[writeBack]" : ""),
              //(queue ? "[queue]" : ""));
#endif
        if (writeBack) {
            context->setRegisterValueUInt64(reg, tag, val, queue);
        }
        if (!queue) {
            currentValue.data.uint64 = val;
        }
    } else if (isImmediate()) {
        if (!queue) {
            currentValue.data.uint64 = val;
        }
    } else {
        printf("  trying to set UNHANDLED OPERAND: %s\n", toStringV().c_str());
    }
}

bool FPOperand::isMemory()
{
    return isStack() || isLocalVar() || isGlobalVar() ||
         (base >= REG_EAX && base <= REG_E15 &&
         (index == REG_NONE || (index >= REG_EAX && index <= REG_E15)));
}

bool FPOperand::isRegister()
{
    return isRegisterSSE() || isRegisterST() || isRegisterGPR();
}

bool FPOperand::isInverted()
{
    return inverted;
}

void FPOperand::setInverted(bool invert)
{
    inverted = invert;
}

bool FPOperand::isImmediate()
{
    return immediate;
}

bool FPOperand::isStack()
{
    return (reg == REG_NONE && base == REG_EBP) || (reg == REG_NONE && base == REG_ESP);
}

bool FPOperand::isLocalVar()
{
    return (reg == REG_NONE && base == REG_EBP && index == REG_NONE && disp < 0);
}

bool FPOperand::isRegisterSSE()
{
    return (reg >= REG_XMM0 && reg <= REG_XMM15);
}

bool FPOperand::isRegisterST()
{
    return (reg >= REG_ST0 && reg <= REG_ST7);
}

bool FPOperand::isRegisterGPR()
{
    return (reg >= REG_EAX && reg <= REG_E15);
}

bool FPOperand::isGlobalVar()
{
    return (!immediate && reg == REG_NONE && (base == REG_NONE || base == REG_EIP));
}

bool FPOperand::isGlobalVarInRange(FPOperandAddress lbound, FPOperandAddress ubound)
{
    return (!immediate && reg == REG_NONE && (base == REG_NONE || base == REG_EIP) && 
            disp >= (long)lbound && disp < (long)ubound);
}

long FPOperand::getDisplacement()
{
    return disp;
}

void FPOperand::getNeededRegisters(set<FPRegister> &regs)
{
    if (reg != REG_NONE)
        regs.insert(reg);
    if (base != REG_NONE)
        regs.insert(base);
    if (index != REG_NONE)
        regs.insert(index);
}

void FPOperand::getModifiedRegisters(set<FPRegister> &regs)
{
    if (reg != REG_NONE)
        regs.insert(reg);
}

inline
FPOperandType FPOperand::leastCommonPrecision(FPOperand* op)
{
    if (type == SignedInt8 || op->type == SignedInt8)
        return SignedInt8;
    else if (type == SignedInt16 || op->type == SignedInt16)
        return SignedInt16;
    else if (type == IEEE_Single || op->type == IEEE_Single)
        return IEEE_Single;
    else if (type == SignedInt32 || op->type == SignedInt32)
        return SignedInt32;
    else if (type == IEEE_Double || op->type == IEEE_Double)
        return IEEE_Double;
    else if (type == SignedInt64 || op->type == SignedInt64)
        return SignedInt64;
    else if (type == C99_LongDouble || op->type == C99_LongDouble)
        return C99_LongDouble;
    else
        return SSE_Quad;
}

void FPOperand::add(FPOperand* op, FPOperandValue *dest)
{
    dest->type = leastCommonPrecision(op);
    switch (dest->type) {
        case IEEE_Single: dest->data.flt = getCurrentValueF() + op->getCurrentValueF(); break;
        case IEEE_Double: dest->data.dbl = getCurrentValueD() + op->getCurrentValueD(); break;
        case C99_LongDouble: dest->data.ldbl = getCurrentValueLD() + op->getCurrentValueLD(); break;
        case SignedInt8: dest->data.sint8 = getCurrentValueSInt8() + op->getCurrentValueSInt8(); break;
        case SignedInt16: dest->data.sint16 = getCurrentValueSInt16() + op->getCurrentValueSInt16(); break;
        case SignedInt32: dest->data.sint32 = getCurrentValueSInt32() + op->getCurrentValueSInt32(); break;
        case SignedInt64: dest->data.sint64 = getCurrentValueSInt64() + op->getCurrentValueSInt64(); break;
        case UnsignedInt8: dest->data.uint8 = getCurrentValueUInt8() + op->getCurrentValueUInt8(); break;
        case UnsignedInt16: dest->data.uint16 = getCurrentValueUInt16() + op->getCurrentValueUInt16(); break;
        case UnsignedInt32: dest->data.uint32 = getCurrentValueUInt32() + op->getCurrentValueUInt32(); break;
        case UnsignedInt64: dest->data.uint64 = getCurrentValueUInt64() + op->getCurrentValueUInt64(); break;
        case SSE_Quad: break;  // no meaning
    }
}

void FPOperand::sub(FPOperand* op, FPOperandValue *dest)
{
    dest->type = leastCommonPrecision(op);
    switch (dest->type) {
        case IEEE_Single: dest->data.flt = getCurrentValueF() - op->getCurrentValueF(); break;
        case IEEE_Double: dest->data.dbl = getCurrentValueD() - op->getCurrentValueD(); break;
        case C99_LongDouble: dest->data.ldbl = getCurrentValueLD() - op->getCurrentValueLD(); break;
        case SignedInt8: dest->data.sint8 = getCurrentValueSInt8() - op->getCurrentValueSInt8(); break;
        case SignedInt16: dest->data.sint16 = getCurrentValueSInt16() - op->getCurrentValueSInt16(); break;
        case SignedInt32: dest->data.sint32 = getCurrentValueSInt32() - op->getCurrentValueSInt32(); break;
        case SignedInt64: dest->data.sint64 = getCurrentValueSInt64() - op->getCurrentValueSInt64(); break;
        case UnsignedInt8: dest->data.uint8 = getCurrentValueUInt8() - op->getCurrentValueUInt8(); break;
        case UnsignedInt16: dest->data.uint16 = getCurrentValueUInt16() - op->getCurrentValueUInt16(); break;
        case UnsignedInt32: dest->data.uint32 = getCurrentValueUInt32() - op->getCurrentValueUInt32(); break;
        case UnsignedInt64: dest->data.uint64 = getCurrentValueUInt64() - op->getCurrentValueUInt64(); break;
        case SSE_Quad: break;  // no meaning
    }
}

string FPOperand::toString()
{
    stringstream ss;
    ss.clear();
    ss.str("");
    switch (type) {
        case IEEE_Single: ss << "IEEE single"; break;
        case IEEE_Double: ss << "IEEE double"; break;
        case C99_LongDouble: ss << "C99 long double"; break;
        case SignedInt8: ss << "8-bit signed int"; break;
        case SignedInt16: ss << "16-bit signed int"; break;
        case SignedInt32: ss << "32-bit signed int"; break;
        case SignedInt64: ss << "64-bit signed int"; break;
        case UnsignedInt8: ss << "8-bit unsigned int"; break;
        case UnsignedInt16: ss << "16-bit unsigned int"; break;
        case UnsignedInt32: ss << "32-bit unsigned int"; break;
        case UnsignedInt64: ss << "64-bit unsigned int"; break;
        case SSE_Quad: ss << "128-bit SSE register"; break;
    }
    if (reg != REG_NONE) {
        ss << "  reg=" << FPContext::FPReg2Str(reg);
    }
    if (tag != 0) {
        ss << "  tag=" << tag;
    }
    if (base != REG_NONE) {
        ss << "  base=" << FPContext::FPReg2Str(base);
    }
    if (index != REG_NONE) {
        ss << "  idx=" << FPContext::FPReg2Str(index);
    }
    if (scale != 1) {
        ss << "  scale=" << scale;
    }
    if (disp != 0) {
        if (base == REG_NONE)
            ss << "  addr=0x" << hex << disp << dec;
        else
            ss << "  disp=0x" << hex << disp << dec;
    }
    if (segment != REG_NONE) {
        ss << "  segment=" << FPContext::FPReg2Str(segment);
    }
    if (immediate) {
        switch (currentValue.type) {
            case IEEE_Single: ss << "  imm_f=" << currentValue.data.flt; break;
            case IEEE_Double: ss << "  imm_d=" << currentValue.data.dbl; break;
            case C99_LongDouble: ss << "  imm_ld=" << currentValue.data.ldbl; break;
            case SignedInt8: ss << "  imm_si8=" << currentValue.data.sint8; break;
            case SignedInt16: ss << "  imm_si16=" << currentValue.data.sint16; break;
            case SignedInt32: ss << "  imm_si32=" << currentValue.data.sint32; break;
            case SignedInt64: ss << "  imm_si64=" << currentValue.data.sint64; break;
            case UnsignedInt8: ss << "  imm_ui8=" << currentValue.data.sint8; break;
            case UnsignedInt16: ss << "  imm_ui32=" << currentValue.data.sint32; break;
            case UnsignedInt32: ss << "  imm_ui32=" << currentValue.data.sint32; break;
            case UnsignedInt64: ss << "  imm_ui64=" << currentValue.data.sint64; break;
            case SSE_Quad: ss << "  imm_sse_quad=" << currentValue.data.sse_quad[3] << " "
                                                   << currentValue.data.sse_quad[2] << " "
                                                   << currentValue.data.sse_quad[1] << " "
                                                   << currentValue.data.sse_quad[0] << " ";
                           break;
        }
    }
    if (inverted) {
        ss << "  [inv]";
    }
    return ss.str();
}

string FPOperand::toStringV()
{
    stringstream ss;
    ss.clear();
    ss.str("");
    ss << toString();
    ss << "  id=" << currentAddress;
    ss << setprecision(30);
    switch (currentValue.type) {
        case IEEE_Single: ss << "  float=" << currentValue.data.flt; break;
        case IEEE_Double: ss << "  double=" << currentValue.data.dbl; break;
        case C99_LongDouble: ss << "  long double=" << currentValue.data.ldbl; break;
        case SignedInt8: ss << "  sint8=" << currentValue.data.sint8; break;
        case SignedInt16: ss << "  sint16=" << currentValue.data.sint16; break;
        case SignedInt32: ss << "  sint32=" << currentValue.data.sint32; break;
        case SignedInt64: ss << "  sint64=" << currentValue.data.sint64; break;
        case UnsignedInt8: ss << "  uint8=" << currentValue.data.uint8; break;
        case UnsignedInt16: ss << "  uint16=" << currentValue.data.uint16; break;
        case UnsignedInt32: ss << "  uint32=" << currentValue.data.uint32; break;
        case UnsignedInt64: ss << "  uint64=" << currentValue.data.uint64; break;
        case SSE_Quad: ss << "  sse_quad=" << currentValue.data.sse_quad[3] << " "
                                           << currentValue.data.sse_quad[2] << " "
                                           << currentValue.data.sse_quad[1] << " "
                                           << currentValue.data.sse_quad[0] << " ";
                       break;
    }
    ss << "  ptr=" << hex << currentValue.data.ptr << dec;
    return ss.str();
}

}

