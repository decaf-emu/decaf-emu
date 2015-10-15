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

// Floating Add
static void
fadd(ThreadState *state, Instruction instr)
{
   double a, b, d;
   a = state->fpr[instr.frA].paired0;
   b = state->fpr[instr.frB].paired0;

   state->fpscr.vxisi = is_infinity(a) && is_infinity(b);
   state->fpscr.vxsnan = is_signalling_nan(a) || is_signalling_nan(b);

   d = a + b;
   updateFPSCR(state);
   updateFPRF(state, d);
   state->fpr[instr.frD].paired0 = d;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Floating Add Single
static void
fadds(ThreadState *state, Instruction instr)
{
   return fadd(state, instr);
}

// Floating Divide
static void
fdiv(ThreadState *state, Instruction instr)
{
   double a, b, d;
   a = state->fpr[instr.frA].paired0;
   b = state->fpr[instr.frB].paired0;

   state->fpscr.vxzdz = is_zero(a) && is_zero(b);
   state->fpscr.vxidi = is_infinity(a) && is_infinity(b);
   state->fpscr.vxsnan = is_signalling_nan(a) || is_signalling_nan(b);

   d = a / b;
   updateFPSCR(state);
   updateFPRF(state, d);
   state->fpr[instr.frD].paired0 = d;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Floating Divide Single
static void
fdivs(ThreadState *state, Instruction instr)
{
   return fdiv(state, instr);
}

// Floating Multiply
static void
fmul(ThreadState *state, Instruction instr)
{
   double a, c, d;
   a = state->fpr[instr.frA].paired0;
   c = state->fpr[instr.frC].paired0;

   state->fpscr.vximz = is_infinity(a) && is_zero(c);
   state->fpscr.vxsnan = is_signalling_nan(a) || is_signalling_nan(c);

   d = a * c;
   updateFPSCR(state);
   updateFPRF(state, d);
   state->fpr[instr.frD].paired0 = d;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Floating Multiply Single
static void
fmuls(ThreadState *state, Instruction instr)
{
   return fmul(state, instr);
}

// Floating Subtract
static void
fsub(ThreadState *state, Instruction instr)
{
   double a, b, d;
   a = state->fpr[instr.frA].paired0;
   b = state->fpr[instr.frB].paired0;

   state->fpscr.vxisi = is_infinity(a) && is_infinity(b);
   state->fpscr.vxsnan = is_signalling_nan(a) || is_signalling_nan(b);

   d = a - b;
   updateFPSCR(state);
   updateFPRF(state, d);
   state->fpr[instr.frD].paired0 = d;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Floating Subtract Single
static void
fsubs(ThreadState *state, Instruction instr)
{
   return fsub(state, instr);
}

// Floating Reciprocal Estimate Single
static void
fres(ThreadState *state, Instruction instr)
{
   double b, d;
   b = state->fpr[instr.frB].paired0;
   d = 1.0 / b;

   state->fpscr.vxsnan |= is_signalling_nan(b);
   updateFPSCR(state);
   updateFPRF(state, d);
   state->fpr[instr.frD].paired0 = d;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Floating Reciprocal Square Root Estimate
static void
frsqrte(ThreadState *state, Instruction instr)
{
   double b, d;
   b = state->fpr[instr.frB].paired0;
   d = 1.0 / std::sqrt(b);

   auto vxsnan = is_signalling_nan(b);
   state->fpscr.vxsnan |= vxsnan;
   state->fpscr.vxsqrt |= vxsnan;

   updateFPSCR(state);
   updateFPRF(state, d);
   state->fpr[instr.frD].paired0 = d;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

static void
fsel(ThreadState *state, Instruction instr)
{
   double a, b, c, d;
   a = state->fpr[instr.frA].paired0;
   b = state->fpr[instr.frB].paired0;
   c = state->fpr[instr.frC].paired0;

   if (a >= 0.0) {
      d = c;
   } else {
      d = b;
   }

   state->fpr[instr.frD].paired0 = d;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Floating Multiply-Add
static void
fmadd(ThreadState *state, Instruction instr)
{
   double a, b, c, d;
   a = state->fpr[instr.frA].paired0;
   b = state->fpr[instr.frB].paired0;
   c = state->fpr[instr.frC].paired0;

   state->fpscr.vxsnan = is_signalling_nan(a) || is_signalling_nan(b) || is_signalling_nan(c);
   state->fpscr.vxisi = is_infinity(a * c) || is_infinity(c);
   state->fpscr.vximz = is_infinity(a * c) && is_zero(c);

   d = (a * c) + b;
   updateFPSCR(state);
   updateFPRF(state, d);
   state->fpr[instr.frD].paired0 = d;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Floating Multiply-Add Single
static void
fmadds(ThreadState *state, Instruction instr)
{
   return fmadd(state, instr);
}

// Floating Multiply-Sub
static void
fmsub(ThreadState *state, Instruction instr)
{
   double a, b, c, d;
   a = state->fpr[instr.frA].paired0;
   b = state->fpr[instr.frB].paired0;
   c = state->fpr[instr.frC].paired0;

   state->fpscr.vximz = is_infinity(a * c) && is_zero(c);
   state->fpscr.vxisi = is_infinity(a * c) || is_infinity(c);
   state->fpscr.vxsnan = is_signalling_nan(a) || is_signalling_nan(b) || is_signalling_nan(c);

   d = (a * c) - b;
   updateFPSCR(state);
   updateFPRF(state, d);
   state->fpr[instr.frD].paired0 = d;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Floating Multiply-Sub Single
static void
fmsubs(ThreadState *state, Instruction instr)
{
   return fmsub(state, instr);
}

// Floating Negative Multiply-Add
static void
fnmadd(ThreadState *state, Instruction instr)
{
   double a, b, c, d;
   a = state->fpr[instr.frA].paired0;
   b = state->fpr[instr.frB].paired0;
   c = state->fpr[instr.frC].paired0;

   state->fpscr.vximz = is_infinity(a * c) && is_zero(c);
   state->fpscr.vxisi = is_infinity(a * c) || is_infinity(c);
   state->fpscr.vxsnan = is_signalling_nan(a) || is_signalling_nan(b) || is_signalling_nan(c);

   d = -((a * c) + b);
   updateFPSCR(state);
   updateFPRF(state, d);
   state->fpr[instr.frD].paired0 = d;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Floating Negative Multiply-Add Single
static void
fnmadds(ThreadState *state, Instruction instr)
{
   return fnmadd(state, instr);
}

// Floating Negative Multiply-Sub
static void
fnmsub(ThreadState *state, Instruction instr)
{
   double a, b, c, d;
   a = state->fpr[instr.frA].paired0;
   b = state->fpr[instr.frB].paired0;
   c = state->fpr[instr.frC].paired0;

   state->fpscr.vximz = is_infinity(a * c) && is_zero(c);
   state->fpscr.vxisi = is_infinity(a * c) || is_infinity(c);
   state->fpscr.vxsnan = is_signalling_nan(a) || is_signalling_nan(b) || is_signalling_nan(c);

   d = -((a * c) - b);
   updateFPSCR(state);
   updateFPRF(state, d);
   state->fpr[instr.frD].paired0 = d;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Floating Negative Multiply-Sub Single
static void
fnmsubs(ThreadState *state, Instruction instr)
{
   return fnmsub(state, instr);
}

// Floating Convert to Integer Word
static void
fctiw(ThreadState *state, Instruction instr)
{
   double b;
   int32_t bi;
   b = state->fpr[instr.frB].paired0;

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

// Floating Convert to Integer Word with Round toward Zero
static void
fctiwz(ThreadState *state, Instruction instr)
{
   double b;
   int32_t bi;
   b = state->fpr[instr.frB].paired0;

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

// Floating Round to Single
static void
frsp(ThreadState *state, Instruction instr)
{
   auto b = state->fpr[instr.frB].paired0;
   state->fpscr.vxsnan |= is_signalling_nan(b);

   auto d = static_cast<float>(b);
   updateFPSCR(state);
   updateFPRF(state, d);
   state->fpr[instr.frD].paired0 = static_cast<double>(d);

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Floating Absolute Value
static void
fabs(ThreadState *state, Instruction instr)
{
   double b, d;

   b = state->fpr[instr.frB].paired0;
   d = std::fabs(b);
   state->fpr[instr.frD].paired0 = d;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Floating Negative Absolute Value
static void
fnabs(ThreadState *state, Instruction instr)
{
   double b, d;

   b = state->fpr[instr.frB].paired0;
   d = -std::fabs(b);
   state->fpr[instr.frD].paired0 = d;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Floating Move Register
static void
fmr(ThreadState *state, Instruction instr)
{
   state->fpr[instr.frD].paired0 = state->fpr[instr.frB].paired0;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Floating Negate
static void
fneg(ThreadState *state, Instruction instr)
{
   state->fpr[instr.frD].paired0 = -state->fpr[instr.frB].paired0;

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
