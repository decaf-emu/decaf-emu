#include <cfenv>
#include <numeric>
#include "bitutils.h"
#include "floatutils.h"
#include "interpreter.h"
#include "interpreter_float.h"

void
updateFPSCR(ThreadState *state)
{
   auto except = std::fetestexcept(FE_ALL_EXCEPT);
   auto round = std::fegetround();
   auto &fpscr = state->fpscr;

   // Underflow
   fpscr.ux |= !!(except & FE_UNDERFLOW);

   // Overflow
   fpscr.ox |= !!(except & FE_OVERFLOW);

   // Zerodivide
   fpscr.zx |= !!(except & FE_DIVBYZERO);

   // Inexact
   fpscr.fi = !!(except & FE_INEXACT);
   fpscr.xx |= fpscr.fi;

   // Fraction Rounded
   fpscr.fr = !!(round & FE_UPWARD);

   // FP Enabled Exception Summary
   fpscr.fex =
        (fpscr.vx & fpscr.ve)
      | (fpscr.ox & fpscr.oe)
      | (fpscr.ux & fpscr.ue)
      | (fpscr.zx & fpscr.ze)
      | (fpscr.xx & fpscr.xe);

   // Invalid Operation Summary
   fpscr.vx =
        fpscr.vxsnan
      | fpscr.vxisi
      | fpscr.vxidi
      | fpscr.vxzdz
      | fpscr.vximz
      | fpscr.vxvc
      | fpscr.vxsqrt
      | fpscr.vxsoft
      | fpscr.vxcvi;

   // FP Exception Summary
   fpscr.fx =
        fpscr.vx
      | fpscr.ox
      | fpscr.ux
      | fpscr.zx
      | fpscr.xx;
}

template<typename Type>
void
updateFPRF(ThreadState *state, Type value)
{
   auto cls = std::fpclassify(value);
   auto neg = std::signbit(value);
   auto flags = 0u;

   switch (cls) {
   case FP_NAN:
      flags |= FloatingPointResultFlags::ClassDescriptor;
      flags |= FloatingPointResultFlags::Unordered;
      break;
   case FP_INFINITE:
      flags |= FloatingPointResultFlags::Unordered;
      break;
   case FP_SUBNORMAL:
      flags |= FloatingPointResultFlags::ClassDescriptor;
      break;
   case FP_ZERO:
      flags |= FloatingPointResultFlags::Equal;
      break;
   }

   if (cls != FP_NAN) {
      if (neg) {
         flags |= FloatingPointResultFlags::Negative;
      } else {
         flags |= FloatingPointResultFlags::Positive;
      }
   }

   state->fpscr.fprf = flags;
}

void
updateFloatConditionRegister(ThreadState *state)
{
   state->cr.cr1 = state->fpscr.cr1;
}

template<>
float
getFpr<float>(ThreadState *state, unsigned fr)
{
   return (float)(state->fpr[fr].value);
}

template<>
double
getFpr<double>(ThreadState *state, unsigned fr)
{
   return state->fpr[fr].value;
}

void
setFpr(ThreadState *state, unsigned fr, float value)
{
   state->fpr[fr].value = (double)value;
}

void
setFpr(ThreadState *state, unsigned fr, double value)
{
   state->fpr[fr].value = value;
}

// Floating Add
template<typename Type>
static void
floatAdd(ThreadState *state, Instruction instr)
{
   Type a, b, d;
   a = getFpr<Type>(state, instr.frA);
   b = getFpr<Type>(state, instr.frB);

   state->fpscr.vxisi = is_infinity(a) && is_infinity(b);
   state->fpscr.vxsnan = is_signalling_nan(a) || is_signalling_nan(b);

   d = a + b;
   updateFPSCR(state);
   updateFPRF(state, d);
   setFpr(state, instr.frD, d);

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

static void
fadd(ThreadState *state, Instruction instr)
{
   return floatAdd<double>(state, instr);
}

static void
fadds(ThreadState *state, Instruction instr)
{
   return floatAdd<float>(state, instr);
}

// Floating Divide
template<typename Type>
static void
floatDiv(ThreadState *state, Instruction instr)
{
   Type a, b, d;
   a = getFpr<Type>(state, instr.frA);
   b = getFpr<Type>(state, instr.frB);

   state->fpscr.vxzdz = is_zero(a) && is_zero(b);
   state->fpscr.vxidi = is_infinity(a) && is_infinity(b);
   state->fpscr.vxsnan = is_signalling_nan(a) || is_signalling_nan(b);

   d = a / b;
   updateFPSCR(state);
   updateFPRF(state, d);
   setFpr(state, instr.frD, d);

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

static void
fdiv(ThreadState *state, Instruction instr)
{
   return floatDiv<double>(state, instr);
}

static void
fdivs(ThreadState *state, Instruction instr)
{
   return floatDiv<float>(state, instr);
}

// Floating Multiply
template<typename Type>
static void
floatMul(ThreadState *state, Instruction instr)
{
   Type a, b, d;
   a = getFpr<Type>(state, instr.frA);
   b = getFpr<Type>(state, instr.frB);

   state->fpscr.vximz = is_infinity(a) && is_zero(b);
   state->fpscr.vxsnan = is_signalling_nan(a) || is_signalling_nan(b);

   d = a / b;
   updateFPSCR(state);
   updateFPRF(state, d);
   setFpr(state, instr.frD, d);

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

static void
fmul(ThreadState *state, Instruction instr)
{
   return floatMul<double>(state, instr);
}

static void
fmuls(ThreadState *state, Instruction instr)
{
   return floatMul<float>(state, instr);
}

// Floating Subtract
template<typename Type>
static void
floatSub(ThreadState *state, Instruction instr)
{
   Type a, b, d;
   a = getFpr<Type>(state, instr.frA);
   b = getFpr<Type>(state, instr.frB);

   state->fpscr.vxisi = is_infinity(a) && is_infinity(b);
   state->fpscr.vxsnan = is_signalling_nan(a) || is_signalling_nan(b);

   d = a - b;
   updateFPSCR(state);
   updateFPRF(state, d);
   setFpr(state, instr.frD, d);

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

static void
fsub(ThreadState *state, Instruction instr)
{
   return floatSub<double>(state, instr);
}

static void
fsubs(ThreadState *state, Instruction instr)
{
   return floatSub<float>(state, instr);
}

static void
fres(ThreadState *state, Instruction instr)
{
   float b, d;
   b = getFpr<float>(state, instr.frB);
   d = 1.0f / b;

   state->fpscr.vxsnan |= is_signalling_nan(b);
   updateFPSCR(state);
   updateFPRF(state, d);
   setFpr(state, instr.frD, d);

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

static void
frsqrte(ThreadState *state, Instruction instr)
{
   double b, d;
   b = getFpr<double>(state, instr.frB);
   d = 1.0f / std::sqrt(b);

   auto vxsnan = is_signalling_nan(b);
   state->fpscr.vxsnan |= vxsnan;
   state->fpscr.vxsqrt |= vxsnan;

   updateFPSCR(state);
   updateFPRF(state, d);
   setFpr(state, instr.frD, d);

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

static void
fsel(ThreadState *state, Instruction instr)
{
   double a, b, c, d;
   a = getFpr<double>(state, instr.frA);
   b = getFpr<double>(state, instr.frB);
   c = getFpr<double>(state, instr.frC);

   state->fpscr.vxsnan = is_signalling_nan(a) || is_signalling_nan(b) || is_signalling_nan(c);

   if (a >= 0.0) {
      d = c;
   } else {
      d = b;
   }

   updateFPRF(state, d);
   setFpr(state, instr.frD, d);

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Floating Multiply-Add
template<typename Type>
static void
floatMulAdd(ThreadState *state, Instruction instr)
{
   Type a, b, c, d;
   a = getFpr<Type>(state, instr.frA);
   b = getFpr<Type>(state, instr.frB);
   c = getFpr<Type>(state, instr.frC);

   state->fpscr.vxsnan = is_signalling_nan(a) || is_signalling_nan(b) || is_signalling_nan(c);
   state->fpscr.vxisi = is_infinity(a * c) || is_infinity(c);
   state->fpscr.vximz = is_infinity(a * c) && is_zero(c);

   d = (a * c) + b;
   updateFPSCR(state);
   updateFPRF(state, d);
   setFpr(state, instr.frD, d);

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

static void
fmadd(ThreadState *state, Instruction instr)
{
   return floatMulAdd<double>(state, instr);
}

static void
fmadds(ThreadState *state, Instruction instr)
{
   return floatMulAdd<float>(state, instr);
}

// Floating Multiply-Sub
template<typename Type>
static void
floatMulSub(ThreadState *state, Instruction instr)
{
   Type a, b, c, d;
   a = getFpr<Type>(state, instr.frA);
   b = getFpr<Type>(state, instr.frB);
   c = getFpr<Type>(state, instr.frC);

   state->fpscr.vximz = is_infinity(a * c) && is_zero(c);
   state->fpscr.vxisi = is_infinity(a * c) || is_infinity(c);
   state->fpscr.vxsnan = is_signalling_nan(a) || is_signalling_nan(b) || is_signalling_nan(c);

   d = (a * c) - b;
   updateFPSCR(state);
   updateFPRF(state, d);
   setFpr(state, instr.frD, d);

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

static void
fmsub(ThreadState *state, Instruction instr)
{
   return floatMulSub<double>(state, instr);
}

static void
fmsubs(ThreadState *state, Instruction instr)
{
   return floatMulSub<float>(state, instr);
}

// Floating Negative Multiply-Add
template<typename Type>
static void
floatNegMulAdd(ThreadState *state, Instruction instr)
{
   Type a, b, c, d;
   a = getFpr<Type>(state, instr.frA);
   b = getFpr<Type>(state, instr.frB);
   c = getFpr<Type>(state, instr.frC);

   state->fpscr.vximz = is_infinity(a * c) && is_zero(c);
   state->fpscr.vxisi = is_infinity(a * c) || is_infinity(c);
   state->fpscr.vxsnan = is_signalling_nan(a) || is_signalling_nan(b) || is_signalling_nan(c);

   d = -((a * c) + b);
   updateFPSCR(state);
   updateFPRF(state, d);
   setFpr(state, instr.frD, d);

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

static void
fnmadd(ThreadState *state, Instruction instr)
{
   return floatNegMulAdd<double>(state, instr);
}

static void
fnmadds(ThreadState *state, Instruction instr)
{
   return floatNegMulAdd<float>(state, instr);
}

// Floating Negative Multiply-Sub
template<typename Type>
static void
floatNegMulSub(ThreadState *state, Instruction instr)
{
   Type a, b, c, d;
   a = getFpr<Type>(state, instr.frA);
   b = getFpr<Type>(state, instr.frB);
   c = getFpr<Type>(state, instr.frC);

   state->fpscr.vximz = is_infinity(a * c) && is_zero(c);
   state->fpscr.vxisi = is_infinity(a * c) || is_infinity(c);
   state->fpscr.vxsnan = is_signalling_nan(a) || is_signalling_nan(b) || is_signalling_nan(c);

   d = -((a * c) - b);
   updateFPSCR(state);
   updateFPRF(state, d);
   setFpr(state, instr.frD, d);

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

static void
fnmsub(ThreadState *state, Instruction instr)
{
   return floatNegMulSub<double>(state, instr);
}

static void
fnmsubs(ThreadState *state, Instruction instr)
{
   return floatNegMulSub<float>(state, instr);
}

static void
fctiw(ThreadState *state, Instruction instr)
{
   double b;
   int32_t bi;
   b = state->fpr[instr.frB].value;

   if (b > static_cast<double>(0x7FFFFFFF)) {
      bi = 0x7FFFFFFF;
      state->fpscr.vxcvi = 1;
   } else if (b < static_cast<double>(0x80000000)) {
      bi = 0x80000000;
      state->fpscr.vxcvi = 1;
   } else {
      switch (state->fpscr.rn) {
      case FloatingPointRoundMode::Nearest:
         bi = static_cast<int32_t>(std::round(b));
         break;
      case FloatingPointRoundMode::Positive:
         bi = static_cast<int32_t>(std::ceil(b));
         break;
      case FloatingPointRoundMode::Negative:
         bi = static_cast<int32_t>(std::floor(b));
         break;
      case FloatingPointRoundMode::Zero:
         bi = static_cast<int32_t>(std::trunc(b));
         break;
      }
   }

   auto vxsnan = is_signalling_nan(b);
   state->fpscr.vxsnan |= vxsnan;
   state->fpscr.vxcvi |= vxsnan;

   updateFPSCR(state);
   state->fpr[instr.frD].iw0 = bi;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

static void
fctiwz(ThreadState *state, Instruction instr)
{
   double b;
   int32_t bi;
   b = state->fpr[instr.frB].value;

   if (b > static_cast<double>(0x7FFFFFFF)) {
      bi = 0x7FFFFFFF;
      state->fpscr.vxcvi = 1;
   } else if (b < static_cast<double>(0x80000000)) {
      bi = 0x80000000;
      state->fpscr.vxcvi = 1;
   } else {
      bi = static_cast<int32_t>(std::trunc(b));
   }

   auto vxsnan = is_signalling_nan(b);
   state->fpscr.vxsnan |= vxsnan;
   state->fpscr.vxcvi |= vxsnan;

   updateFPSCR(state);
   state->fpr[instr.frD].iw0 = bi;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

static void
frsp(ThreadState *state, Instruction instr)
{
   auto b = state->fpr[instr.frB].value;
   state->fpscr.vxsnan |= is_signalling_nan(b);

   auto d = static_cast<float>(b);
   updateFPSCR(state);
   updateFPRF(state, d);
   setFpr(state, instr.frD, d);

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Floating Absolute Value
static void
fabs(ThreadState *state, Instruction instr)
{
   double b, d;

   b = state->fpr[instr.frB].value;
   d = std::fabs(b);
   setFpr(state, instr.frD, d);

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Floating Negative Absolute Value
static void
fnabs(ThreadState *state, Instruction instr)
{
   double b, d;

   b = state->fpr[instr.frB].value;
   d = -std::fabs(b);
   setFpr(state, instr.frD, d);

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Floating Move Register
static void
fmr(ThreadState *state, Instruction instr)
{
   double b;

   b = state->fpr[instr.frB].value;
   setFpr(state, instr.frD, b);

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Floating Negate
static void
fneg(ThreadState *state, Instruction instr)
{
   double b;

   b = -state->fpr[instr.frB].value;
   setFpr(state, instr.frD, b);

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

void
Interpreter::registerFloatInstructions()
{
   RegisterInstruction(fadd);
   RegisterInstruction(fadds);
   RegisterInstruction(fdiv);
   RegisterInstruction(fdivs);
   RegisterInstruction(fmul);
   RegisterInstruction(fmuls);
   RegisterInstruction(fsub);
   RegisterInstruction(fsubs);
   RegisterInstruction(fres);
   RegisterInstruction(frsqrte);
   RegisterInstruction(fsel);
   RegisterInstruction(fmadd);
   RegisterInstruction(fmadds);
   RegisterInstruction(fmsub);
   RegisterInstruction(fmsubs);
   RegisterInstruction(fnmadd);
   RegisterInstruction(fnmadds);
   RegisterInstruction(fnmsub);
   RegisterInstruction(fnmsubs);
   RegisterInstruction(fctiw);
   RegisterInstruction(fctiwz);
   RegisterInstruction(frsp);
   RegisterInstruction(fabs);
   RegisterInstruction(fnabs);
   RegisterInstruction(fmr);
   RegisterInstruction(fabs);
   RegisterInstruction(fneg);
}
