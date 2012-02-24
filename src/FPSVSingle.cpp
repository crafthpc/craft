#include "FPSVSingle.h"

namespace FPInst {

FPSVSingle::FPSVSingle(FPOperandAddress addr) : 
    FPSV(SVT_IEEE_Single, addr)
{
    value = 0.0f;
}

FPSVSingle::~FPSVSingle()
{
}

void FPSVSingle::setSeedData()
{
    seed_size = 1;
    *(uint32_t*)seed_data = *(uint32_t*)(&value);
}

void FPSVSingle::setValue(FPOperand *op)
{
    setValue(op->getCurrentValue());
}

#define SH_SET_VALUE(TYPE,MEMBER) this->value = (float)val.data.MEMBER; \
                                  this->seed_size = sizeof(TYPE)/4; \
                                  *(TYPE*)(this->seed_data) = val.data.MEMBER

// set the shadow value to the current value of the given operand,
// coercing the value into the type of the shadow value
void FPSVSingle::setValue(FPOperandValue val)
{
#ifdef INCLUDE_DEBUG
    if (FPAnalysisPointer::debugPrint) {
        cout << "FPSVSingle initializing shadow value: " 
             << "[" << this << "] := " << FPOperand::FPOpVal2Str(&val) << endl;
    }
#endif
    switch (val.type) {
        case SignedInt8:     SH_SET_VALUE(  int8_t,  sint8);  break;
        case UnsignedInt8:   SH_SET_VALUE( uint8_t,  uint8);  break;
        case SignedInt16:    SH_SET_VALUE( int16_t, sint16);  break;
        case UnsignedInt16:  SH_SET_VALUE(uint16_t, uint16);  break;
        case SignedInt32:    SH_SET_VALUE( int32_t, sint32);  break;
        case UnsignedInt32:  SH_SET_VALUE(uint32_t, uint32);  break;
        case SignedInt64:    SH_SET_VALUE( int64_t, sint64);  break;
        case UnsignedInt64:  SH_SET_VALUE(uint64_t, uint64);  break;
        case IEEE_Single:    SH_SET_VALUE(float, flt);        break;
        case IEEE_Double:    SH_SET_VALUE(double, dbl);       break;
        case C99_LongDouble: SH_SET_VALUE(long double, ldbl); break;

        // TODO: what to do with quads?
        case SSE_Quad:       SH_SET_VALUE(uint32_t, sse_quad[0]); break;
    }
}

void FPSVSingle::setToZero()
{
    value = 0.0f;
}

void FPSVSingle::setToPositiveBitFlag()
{
    *(uint32_t*)(&value) = 0x0;
    value = 0.0f;
}

void FPSVSingle::setToNegativeBitFlag()
{
    *(uint32_t*)(&value) = 0x80000000;
}

void FPSVSingle::setToCMPTrue()
{
    *(uint32_t*)(&value) = 0xFFFFFFFF;
}

void FPSVSingle::setToCMPFalse()
{
    *(uint32_t*)(&value) = 0x0;
}

int FPSVSingle::compare(FPSV *val2)
{
    FPSVSingle *val2d = static_cast<FPSVSingle*>(val2);
    int rval = 0;
    if (value < val2d->value) {
        rval = -1;
    } else if (value > val2d->value) {
        rval = 1;
    }
    return rval;
}

void FPSVSingle::doUnaryOp(FPOperationType type, FPSV *op)
{
    FPSVSingle *opd = static_cast<FPSVSingle*>(op);
    switch (type) {
        case OP_NEG:    value = -(opd->value);       break;
        case OP_ABS:    value = fabsf(opd->value);   break;
        case OP_RCP:    value = 1.0f / opd->value;   break;
        case OP_SQRT:   value = sqrtf(opd->value);   break;
        case OP_ZERO:   value = 0.0f;                break;
        case OP_SIN:    value = sinf(opd->value);    break;
        case OP_COS:    value = cosf(opd->value);    break;
        case OP_TAN:    value = tanf(opd->value);    break;
        case OP_ASIN:   value = asinf(opd->value);   break;
        case OP_ACOS:   value = acosf(opd->value);   break;
        case OP_ATAN:   value = atanf(opd->value);   break;
        case OP_SINH:   value = sinhf(opd->value);   break;
        case OP_COSH:   value = coshf(opd->value);   break;
        case OP_TANH:   value = tanhf(opd->value);   break;
        case OP_ASINH:  value = asinhf(opd->value);  break;
        case OP_ACOSH:  value = acoshf(opd->value);  break;
        case OP_ATANH:  value = atanhf(opd->value);  break;
        case OP_LOG:    value = logf(opd->value);    break;
        case OP_LOGB:   value = logbf(opd->value);   break;
        case OP_LOG10:  value = log10f(opd->value);  break;
        case OP_EXP:    value = expf(opd->value);    break;
        case OP_EXP2:   value = exp2f(opd->value);   break;
        case OP_CEIL:   value = ceilf(opd->value);   break;
        case OP_FLOOR:  value = floorf(opd->value);  break;
        case OP_TRUNC:  value = truncf(opd->value);  break;
        case OP_ROUND:  value = roundf(opd->value);  break;
        case OP_ERF:    value = erff(opd->value);    break;
        case OP_ERFC:   value = erfcf(opd->value);   break;

        default:

            cout << "ERROR: unhandled FPSVSingle operation (" << FPOperation::FPOpType2Str(type) 
                << " " << opd->value << ")" << endl;
            break;
    }
#ifdef INCLUDE_DEBUG
    /*
     *if (FPAnalysisPointer::debugPrint) {
     *    cout << "FPSVSingle finished unary operation: " 
     *         << FPOperation::FPOpType2Str(type) << " " << opd->value <<  " = " << value << endl
     *         << FPOperation::FPOpType2Str(type) << "  " << opd->toDetailedString() << " = " << endl
     *         << "  " << toDetailedString() << endl;
     *}
     */
#endif
}

void FPSVSingle::doBinaryOp(FPOperationType type, FPSV *op1, FPSV *op2)
{
    FPSVSingle *op1d = static_cast<FPSVSingle*>(op1);
    FPSVSingle *op2d = static_cast<FPSVSingle*>(op2);
    switch (type) {
        case OP_ADD:
            value = op1d->value + op2d->value; break;
        case OP_SUB:
            value = op1d->value - op2d->value; break;
        case OP_MUL:
            value = op1d->value * op2d->value; break;
        case OP_DIV:
            value = op1d->value / op2d->value; break;
        case OP_FMOD:
            value = fmodf(op1d->value, op2d->value); break;
        case OP_MIN:
            value = (op1d->value <= op2d->value ? op1d->value : op2d->value); break;
        case OP_MAX:
            value = (op1d->value >= op2d->value ? op1d->value : op2d->value); break;
        case OP_ATAN2:
            value = atan2f(op1d->value, op2d->value); break;
        case OP_COPYSIGN:
            value = copysignf(op1d->value, op2d->value); break;
        case OP_POW:
            value = powf(op1d->value, op2d->value); break;
            
        default:
            cout << "ERROR: unhandled FPSVSingle operation (" << op1d->value << " " 
                 << FPOperation::FPOpType2Str(type) << " " << op2d->value << ")" << endl;
            break;
    }
#ifdef INCLUDE_DEBUG
    /*
     *if (FPAnalysisPointer::debugPrint) {
     *    cout << "FPSVSingle finished binary operation: " 
     *         << op1d->value << " " << FPOperation::FPOpType2Str(type) << " " << op2d->value <<  " = " << value << endl
     *         << "  " << op1d->toDetailedString() << " " << FPOperation::FPOpType2Str(type) << endl 
     *         << "  " << op2d->toDetailedString() << " = " << endl
     *         << "  " << toDetailedString() << endl;
     *}
     */
#endif
}

void FPSVSingle::doModF(FPSV *op, FPSV *int_part)
{
    FPSVSingle *opd = static_cast<FPSVSingle*>(op);
    FPSVSingle *opid = static_cast<FPSVSingle*>(int_part);
    value = modff(opd->value, &(opid->value));
}

void FPSVSingle::doFrExp(FPSV *op, int *exp)
{
    FPSVSingle *opd = static_cast<FPSVSingle*>(op);
    value = frexpf(opd->value, exp);
}

void FPSVSingle::doLdExp(FPSV *op, int exp)
{
    FPSVSingle *opd = static_cast<FPSVSingle*>(op);
    value = ldexpf(opd->value, exp);
}

bool FPSVSingle::isBitwiseZero()
{
    return *(uint32_t*)(&value) == 0x0;
}

bool FPSVSingle::isZero()
{
    return (value == 0.0f || value == -0.0f);
}

bool FPSVSingle::isNegative()
{
    return (value < 0.0f);
}

bool FPSVSingle::isPositive()
{
    return (value > 0.0f);
}

bool FPSVSingle::isNormal()
{
    return isnormal(value);
}

bool FPSVSingle::isNaN()
{
    return isnanf(value);
}

bool FPSVSingle::isInf()
{
    return isinff(value);
}

// get the current shadow value in the operand type most appropriate 
// to this shadow value type
FPOperandValue FPSVSingle::getValue()
{
    FPOperandValue val;
    val.type = IEEE_Single;
    val.data.flt = value;
#ifdef INCLUDE_DEBUG
    /*
     *if (FPAnalysisPointer::debugPrint) {
     *    cout << "FPSVSingle retrieving value: " 
     *         << " [" << this << "] => " << FPOperand::FPOpVal2Str(&val) << endl;
     *}
     */
#endif
    return val;
}

#define SV_GET_VALUE(MEMBER, TYPE) val.data.MEMBER = (TYPE)value

// get the current shadow value coerced into the given operand type
FPOperandValue FPSVSingle::getValue(FPOperandType type)
{
    FPOperandValue val;
    val.type = type;
    switch (type) {
        case SignedInt8:     SV_GET_VALUE(sint8,    int8_t);  break;
        case UnsignedInt8:   SV_GET_VALUE(uint8,   uint8_t);  break;
        case SignedInt16:    SV_GET_VALUE(sint16,  int16_t);  break;
        case UnsignedInt16:  SV_GET_VALUE(uint16, uint16_t);  break;
        case SignedInt32:    SV_GET_VALUE(sint32,  int32_t);  break;
        case UnsignedInt32:  SV_GET_VALUE(uint32, uint32_t);  break;
        case SignedInt64:    SV_GET_VALUE(sint64,  int64_t);  break;
        case UnsignedInt64:  SV_GET_VALUE(uint64, uint64_t);  break;
        case IEEE_Single:    SV_GET_VALUE(flt,  float);       break;
        case IEEE_Double:    SV_GET_VALUE(dbl,  double);      break;
        case C99_LongDouble: SV_GET_VALUE(ldbl, long double); break;

        // TODO: what to do about quads?
        case SSE_Quad:       SV_GET_VALUE(sse_quad[0], uint32_t); break;
    }
#ifdef INCLUDE_DEBUG
    /*
     *if (FPAnalysisPointer::debugPrint) {
     *    cout << "FPSVSingle retrieving value: " 
     *         << " [" << this << "] => " << FPOperand::FPOpVal2Str(&val) << endl;
     *}
     */
#endif
    return val;
}

string FPSVSingle::toString()
{
    stringstream ss("");
    ss << value;
    return ss.str();
}

string FPSVSingle::toDetailedString()
{
    stringstream ss("");
    ss << "IEEE_Single:" << this << ":" << value << ":" << addr;
    if (seed_size) {
        ss << "[" << hex;
        for (int i=seed_size-1; i>=0; i--) {
            ss << seed_data[i];
            if (i>0) {
                ss << " ";
            }
        }
        ss << dec << "]";
    }
    return ss.str();
}

string FPSVSingle::getTypeString()
{
    return string("32-bit IEEE floating-point");
}

}

