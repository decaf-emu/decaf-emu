#include <cfenv>
#include <numeric>
#include "interpreter_insreg.h"
#include "interpreter.h"
#include "utils/bitutils.h"
#include "utils/floatutils.h"

const int fres_expected_base[] =
{
   0x7ff800, 0x783800, 0x70ea00, 0x6a0800,
   0x638800, 0x5d6200, 0x579000, 0x520800,
   0x4cc800, 0x47ca00, 0x430800, 0x3e8000,
   0x3a2c00, 0x360800, 0x321400, 0x2e4a00,
   0x2aa800, 0x272c00, 0x23d600, 0x209e00,
   0x1d8800, 0x1a9000, 0x17ae00, 0x14f800,
   0x124400, 0x0fbe00, 0x0d3800, 0x0ade00,
   0x088400, 0x065000, 0x041c00, 0x020c00,
};

const int fres_expected_dec[] =
{
   0x3e1, 0x3a7, 0x371, 0x340,
   0x313, 0x2ea, 0x2c4, 0x2a0,
   0x27f, 0x261, 0x245, 0x22a,
   0x212, 0x1fb, 0x1e5, 0x1d1,
   0x1be, 0x1ac, 0x19b, 0x18b,
   0x17c, 0x16e, 0x15b, 0x15b,
   0x143, 0x143, 0x12d, 0x12d,
   0x11a, 0x11a, 0x108, 0x106,
};

double
ppc_estimate_reciprocal(double v)
{
   auto bits = get_float_bits(v);

   if (bits.mantissa == 0 && bits.exponent == 0) {
      return std::copysign(std::numeric_limits<double>::infinity(), v);
   }

   if (bits.exponent == bits.exponent_max) {
      if (bits.mantissa == 0) {
         return std::copysign(0.0, v);
      }
      return static_cast<float>(v);
   }

   if (bits.exponent < 895) {
      return std::copysign(std::numeric_limits<float>::max(), v);
   }

   if (bits.exponent > 1149) {
      return std::copysign(0.0, v);
   }

   int idx = (int)(bits.mantissa >> 37);
   bits.exponent = 0x7FD - bits.exponent;
   bits.mantissa = (int64_t)(fres_expected_base[idx / 1024] - (fres_expected_dec[idx / 1024] * (idx % 1024) + 1) / 2) << 29;
   return bits.v;
}

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

   std::feclearexcept(FE_ALL_EXCEPT);
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
      } else if (cls != FP_ZERO) {
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

template<typename Type>
static double
checkNan(double d, double a, double b)
{
   if (is_nan(d)) {
      if (is_nan(a)) {
         return make_quiet(static_cast<Type>(a));
      } else if (is_nan(b)) {
         return make_quiet(static_cast<Type>(b));
      } else {
         return make_nan<double>();
      }
   } else {
      return d;
   }
}

// Floating Add
template<typename Type>
static void
faddGeneric(ThreadState *state, Instruction instr)
{
   double a, b, d;
   a = state->fpr[instr.frA].paired0;
   b = state->fpr[instr.frB].paired0;

   state->fpscr.vxisi = is_infinity(a) && is_infinity(b);
   state->fpscr.vxsnan = is_signalling_nan(a) || is_signalling_nan(b);

   d = static_cast<Type>(a + b);
   updateFPSCR(state);

   d = checkNan<Type>(d, a, b);
   updateFPRF(state, d);
   state->fpr[instr.frD].paired0 = d;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Floating Add Double
static void
fadd(ThreadState *state, Instruction instr)
{
   faddGeneric<double>(state, instr);
}

// Floating Add Single
static void
fadds(ThreadState *state, Instruction instr)
{
   faddGeneric<float>(state, instr);
}

// Floating Divide
template<typename Type>
static void
fdivGeneric(ThreadState *state, Instruction instr)
{
   double a, b, d;
   a = state->fpr[instr.frA].paired0;
   b = state->fpr[instr.frB].paired0;

   state->fpscr.vxzdz = is_zero(a) && is_zero(b);
   state->fpscr.vxidi = is_infinity(a) && is_infinity(b);
   state->fpscr.vxsnan = is_signalling_nan(a) || is_signalling_nan(b);

   d = static_cast<Type>(a / b);
   updateFPSCR(state);

   d = checkNan<Type>(d, a, b);
   updateFPRF(state, d);
   state->fpr[instr.frD].paired0 = d;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Floating Divide Double
static void
fdiv(ThreadState *state, Instruction instr)
{
   fdivGeneric<double>(state, instr);
}

// Floating Divide Single
static void
fdivs(ThreadState *state, Instruction instr)
{
   fdivGeneric<float>(state, instr);
}

// Floating Multiply
template<typename Type>
static void
fmulGeneric(ThreadState *state, Instruction instr)
{
   double a, c, d;
   a = state->fpr[instr.frA].paired0;
   c = state->fpr[instr.frC].paired0;

   state->fpscr.vximz = is_infinity(a) && is_zero(c);
   state->fpscr.vxsnan = is_signalling_nan(a) || is_signalling_nan(c);

   d = static_cast<Type>(a * c);
   updateFPSCR(state);

   d = checkNan<Type>(d, a, c);
   updateFPRF(state, d);
   state->fpr[instr.frD].paired0 = d;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Floating Multiply Double
static void
fmul(ThreadState *state, Instruction instr)
{
   fmulGeneric<double>(state, instr);
}

// Floating Multiply Single
static void
fmuls(ThreadState *state, Instruction instr)
{
   fmulGeneric<float>(state, instr);
}

// Floating Subtract
template<typename Type>
static void
fsubGeneric(ThreadState *state, Instruction instr)
{
   double a, b, d;
   a = state->fpr[instr.frA].paired0;
   b = state->fpr[instr.frB].paired0;

   state->fpscr.vxisi = is_infinity(a) && is_infinity(b);
   state->fpscr.vxsnan = is_signalling_nan(a) || is_signalling_nan(b);

   d = static_cast<Type>(a - b);
   updateFPSCR(state);

   d = checkNan<Type>(d, a, b);
   updateFPRF(state, d);
   state->fpr[instr.frD].paired0 = d;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Floating Subtract Double
static void
fsub(ThreadState *state, Instruction instr)
{
   fsubGeneric<double>(state, instr);
}

// Floating Subtract Single
static void
fsubs(ThreadState *state, Instruction instr)
{
   fsubGeneric<float>(state, instr);
}

// Floating Reciprocal Estimate Single
static void
fres(ThreadState *state, Instruction instr)
{
   double b, d;
   b = state->fpr[instr.frB].paired0;

   state->fpscr.vxsnan |= is_signalling_nan(b);

   d = ppc_estimate_reciprocal(b);

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

   d = a * c;
   if (is_nan(d)) {
      if (is_nan(a)) {
         d = make_quiet(a);
      } else if (is_nan(b)) {
         d = make_quiet(b);
      } else if (is_nan(c)) {
         d = make_quiet(c);
      } else {
         d = make_nan<double>();
      }
   } else {
      if (is_infinity(d) && is_infinity(b) && !(is_infinity(a) || is_infinity(c))) {
         d = b;
      } else {
         d = d + b;
         if (is_nan(d)) {
            if (is_nan(b)) {
               d = make_quiet(b);
            } else {
               d = make_nan<double>();
            }
         }
      }
   }

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
   double a, b, c, d;
   a = state->fpr[instr.frA].paired0;
   b = state->fpr[instr.frB].paired0;
   c = state->fpr[instr.frC].paired0;

   state->fpscr.vxsnan = is_signalling_nan(a) || is_signalling_nan(b) || is_signalling_nan(c);
   state->fpscr.vxisi = is_infinity(a * c) || is_infinity(c);
   state->fpscr.vximz = is_infinity(a * c) && is_zero(c);

   d = a * c;
   if (is_nan(d)) {
      if (is_nan(a)) {
         d = make_quiet(a);
      } else if (is_nan(b)) {
         d = make_quiet(b);
      } else if (is_nan(c)) {
         d = make_quiet(c);
      } else {
         d = make_nan<double>();
      }
   } else {
      if (is_infinity(d) && is_infinity(b) && !(is_infinity(a) || is_infinity(c))) {
         d = b;
      } else {
         d = d + b;
         if (is_nan(d)) {
            if (is_nan(b)) {
               d = make_quiet(b);
            } else {
               d = make_nan<double>();
            }
         }
      }
   }

   updateFPSCR(state);
   updateFPRF(state, d);
   state->fpr[instr.frD].paired0 = static_cast<float>(d);

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
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

   if (b > static_cast<double>(INT_MAX)) {
      bi = INT_MAX;
      state->fpscr.vxcvi = 1;
   } else if (b < static_cast<double>(INT_MIN)) {
      bi = INT_MIN;
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
   state->fpr[instr.frD].iw1 = 0xFFF80000 | (is_negative_zero(b) ? 1 : 0);

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

   if (b > static_cast<double>(INT_MAX)) {
      bi = INT_MAX;
      state->fpscr.vxcvi = 1;
   } else if (b < static_cast<double>(INT_MIN)) {
      bi = INT_MIN;
      state->fpscr.vxcvi = 1;
   } else {
      bi = static_cast<int32_t>(std::trunc(b));
   }

   auto vxsnan = is_signalling_nan(b);
   state->fpscr.vxsnan |= vxsnan;
   state->fpscr.vxcvi |= vxsnan;

   updateFPSCR(state);
   state->fpr[instr.frD].iw0 = bi;
   state->fpr[instr.frD].iw1 = 0xFFF80000 | (is_negative_zero(b) ? 1 : 0);

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
   uint64_t b, d;

   b = state->fpr[instr.frB].value0;
   d = clear_bit(b, 63);
   state->fpr[instr.frD].value0 = d;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Floating Negative Absolute Value
static void
fnabs(ThreadState *state, Instruction instr)
{
   uint64_t b, d;

   b = state->fpr[instr.frB].value0;
   d = set_bit(b, 63);
   state->fpr[instr.frD].value0 = d;

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
   uint64_t b, d;

   b = state->fpr[instr.frB].value0;
   d = flip_bit(b, 63);
   state->fpr[instr.frD].value0 = d;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

void
cpu::interpreter::registerFloatInstructions()
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
   RegisterInstruction(fneg);
}
