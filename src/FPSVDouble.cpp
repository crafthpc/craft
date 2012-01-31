#include "FPSVDouble.h"

namespace FPInst {

FPSVDouble::FPSVDouble(FPOperandAddress addr) : 
    FPSV(SVT_IEEE_Double, addr)
{
    value = 0.0;
}

FPSVDouble::~FPSVDouble()
{
}

void FPSVDouble::setSeedData()
{
    seed_size = 2;
    *(uint64_t*)seed_data = *(uint64_t*)(&value);
}

void FPSVDouble::setValue(FPOperand *op)
{
    setValue(op->getCurrentValue());
}

#define SH_SET_VALUE(TYPE,MEMBER) this->value = (double)val.data.MEMBER; \
                                  this->seed_size = sizeof(TYPE)/4; \
                                  *(TYPE*)(this->seed_data) = val.data.MEMBER

// set the shadow value to the current value of the given operand,
// coercing the value into the type of the shadow value
void FPSVDouble::setValue(FPOperandValue val)
{
#ifdef INCLUDE_DEBUG
    if (FPAnalysisPointer::debugPrint) {
        cout << "FPSVDouble initializing shadow value: " 
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

void FPSVDouble::setToZero()
{
    value = 0.0;
}

void FPSVDouble::setToPositiveBitFlag()
{
    *(uint64_t*)(&value) = 0x0;
    value = 0.0f;
}

void FPSVDouble::setToNegativeBitFlag()
{
    *(uint64_t*)(&value) = 0x8000000000000000;
}

void FPSVDouble::setToCMPTrue()
{
    *(uint64_t*)(&value) = 0xFFFFFFFFFFFFFFFF;
}

void FPSVDouble::setToCMPFalse()
{
    *(uint64_t*)(&value) = 0x0;
}

int FPSVDouble::compare(FPSV *val2)
{
    FPSVDouble *val2d = static_cast<FPSVDouble*>(val2);
    int rval = 0;
    if (value < val2d->value) {
        rval = -1;
    } else if (value > val2d->value) {
        rval = 1;
    }
    return rval;
}

void FPSVDouble::doUnaryOp(FPOperationType type, FPSV *op)
{
    FPSVDouble *opd = static_cast<FPSVDouble*>(op);
    switch (type) {
        case OP_NEG:    value = -(opd->value);      break;
        case OP_ABS:    value = fabs(opd->value);   break;
        case OP_SQRT:   value = sqrt(opd->value);   break;
        case OP_ZERO:   value = 0.0;                break;
        case OP_SIN:    value = sin(opd->value);    break;
        case OP_COS:    value = cos(opd->value);    break;
        case OP_TAN:    value = tan(opd->value);    break;
        case OP_ASIN:   value = asin(opd->value);   break;
        case OP_ACOS:   value = acos(opd->value);   break;
        case OP_ATAN:   value = atan(opd->value);   break;
        case OP_SINH:   value = sinh(opd->value);   break;
        case OP_COSH:   value = cosh(opd->value);   break;
        case OP_TANH:   value = tanh(opd->value);   break;
        case OP_ASINH:  value = asinh(opd->value);  break;
        case OP_ACOSH:  value = acosh(opd->value);  break;
        case OP_ATANH:  value = atanh(opd->value);  break;
        case OP_LOG:    value = log(opd->value);    break;
        case OP_LOGB:   value = logb(opd->value);   break;
        case OP_LOG10:  value = log10(opd->value);  break;
        case OP_EXP:    value = exp(opd->value);    break;
        case OP_EXP2:   value = exp2(opd->value);   break;
        case OP_CEIL:   value = ceil(opd->value);   break;
        case OP_FLOOR:  value = floor(opd->value);  break;
        case OP_TRUNC:  value = trunc(opd->value);  break;
        case OP_ROUND:  value = round(opd->value);  break;
        case OP_ERF:    value = erf(opd->value);    break;
        case OP_ERFC:   value = erfc(opd->value);   break;

        default:
            cout << "ERROR: unhandled FPSVDouble operation (" << FPOperation::FPOpType2Str(type) 
                << " " << opd->value << ")" << endl;
            break;
    }
#ifdef INCLUDE_DEBUG
    if (FPAnalysisPointer::debugPrint) {
       cout << "FPSVDouble finished unary operation: " 
            << FPOperation::FPOpType2Str(type) << " " << opd->value <<  " = " << value << endl;
            //<< FPOperation::FPOpType2Str(type) << "  " << opd->toDetailedString() << " = " << endl
            //<< "  " << toDetailedString() << endl;
    }
#endif
}

void FPSVDouble::doBinaryOp(FPOperationType type, FPSV *op1, FPSV *op2)
{
    FPSVDouble *op1d = static_cast<FPSVDouble*>(op1);
    FPSVDouble *op2d = static_cast<FPSVDouble*>(op2);
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
            value = fmod(op1d->value, op2d->value); break;
        case OP_MIN:
            value = (op1d->value <= op2d->value ? op1d->value : op2d->value); break;
        case OP_MAX:
            value = (op1d->value >= op2d->value ? op1d->value : op2d->value); break;
        case OP_ATAN2:
            value = atan2(op1d->value, op2d->value); break;
        case OP_COPYSIGN:
            value = copysign(op1d->value, op2d->value); break;
        case OP_POW:
            value = pow(op1d->value, op2d->value); break;

        default:
            cout << "ERROR: unhandled FPSVDouble operation (" << op1d->value << " " 
                 << FPOperation::FPOpType2Str(type) << " " << op2d->value << ")" << endl;
            break;
    }
#ifdef INCLUDE_DEBUG
    if (FPAnalysisPointer::debugPrint) {
       cout << "FPSVDouble finished binary operation: " 
            << op1d->value << " " << FPOperation::FPOpType2Str(type) << " " << op2d->value <<  " = " << value << endl;
            //<< "  " << op1d->toDetailedString() << " " << FPOperation::FPOpType2Str(type) << endl 
            //<< "  " << op2d->toDetailedString() << " = " << endl
            //<< "  " << toDetailedString() << endl;
    }
#endif
}

void FPSVDouble::doModF(FPSV *op, FPSV *int_part)
{
    FPSVDouble *opd = static_cast<FPSVDouble*>(op);
    FPSVDouble *opid = static_cast<FPSVDouble*>(int_part);
    value = modf(opd->value, &(opid->value));
}

void FPSVDouble::doFrExp(FPSV *op, int *exp)
{
    FPSVDouble *opd = static_cast<FPSVDouble*>(op);
    value = frexp(opd->value, exp);
}

void FPSVDouble::doLdExp(FPSV *op, int exp)
{
    FPSVDouble *opd = static_cast<FPSVDouble*>(op);
    value = ldexp(opd->value, exp);
}

bool FPSVDouble::isBitwiseZero()
{
    return *(uint64_t*)(&value) == 0x0;
}

bool FPSVDouble::isZero()
{
    return (value == 0.0 || value == -0.0);
}

bool FPSVDouble::isNegative()
{
    return (value < 0.0);
}

bool FPSVDouble::isPositive()
{
    return (value > 0.0);
}

bool FPSVDouble::isNormal()
{
    return isnormal(value);
}

bool FPSVDouble::isNaN()
{
    return isnan(value);
}

bool FPSVDouble::isInf()
{
    return isinf(value);
}

// get the current shadow value in the operand type most appropriate 
// to this shadow value type
FPOperandValue FPSVDouble::getValue()
{
    FPOperandValue val;
    val.type = IEEE_Double;
    val.data.dbl = value;
#ifdef INCLUDE_DEBUG
    /*
     *if (FPAnalysisPointer::debugPrint) {
     *    cout << "FPSVDouble retrieving value: " 
     *         << " [" << this << "] => " << FPOperand::FPOpVal2Str(&val) << endl;
     *}
     */
#endif
    return val;
}

#define SV_GET_VALUE(MEMBER, TYPE) val.data.MEMBER = (TYPE)value

// get the current shadow value coerced into the given operand type
FPOperandValue FPSVDouble::getValue(FPOperandType type)
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
     *    cout << "FPSVDouble retrieving value: " 
     *         << " [" << this << "] => " << FPOperand::FPOpVal2Str(&val) << endl;
     *}
     */
#endif
    return val;
}

string FPSVDouble::toString()
{
    stringstream ss("");
    ss << value;
    return ss.str();
}

string FPSVDouble::toDetailedString()
{
    stringstream ss("");
    ss << "IEEE_Double:" << this << ":" << value << ":" << addr;
    if (seed_size) {
        ss << "[" << hex;
        for (size_t i=0; i<seed_size; i++) {
            if (i>0) {
                ss << " ";
            }
            ss << seed_data[i];
        }
        ss << dec << "]";
    }
    return ss.str();
}

string FPSVDouble::getTypeString()
{
    return string("64-bit IEEE floating-point");
}

}

