#include <cfenv>
#include <cmath>
#include <numeric>
#include "../cpu_internal.h"
#include "interpreter.h"
#include "interpreter_float.h"
#include "interpreter_insreg.h"
#include <common/bitutils.h>
#include <common/floatutils.h>
#include <common/platform_compiler.h>

using espresso::FpscrFlags;
using espresso::FloatingPointResultFlags;
using espresso::FloatingPointRoundMode;

const uint16_t fres_base[] =
{
   0x3FFC, 0x3C1C, 0x3875, 0x3504, 0x31C4, 0x2EB1, 0x2BC8, 0x2904,
   0x2664, 0x23E5, 0x2184, 0x1F40, 0x1D16, 0x1B04, 0x190A, 0x1725,
   0x1554, 0x1396, 0x11EB, 0x104F, 0x0EC4, 0x0D48, 0x0BD7, 0x0A7C,
   0x0922, 0x07DF, 0x069C, 0x056F, 0x0442, 0x0328, 0x020E, 0x0106,
};

const uint16_t fres_delta[] =
{
   0x3E1, 0x3A7, 0x371, 0x340, 0x313, 0x2EA, 0x2C4, 0x2A0,
   0x27F, 0x261, 0x245, 0x22A, 0x212, 0x1FB, 0x1E5, 0x1D1,
   0x1BE, 0x1AC, 0x19B, 0x18B, 0x17C, 0x16E, 0x15B, 0x15B,
   0x143, 0x143, 0x12D, 0x12D, 0x11A, 0x11A, 0x108, 0x106,
};

const uint16_t frsqrte_base[] =
{
   0x7FF4, 0x7852, 0x7154, 0x6AE4, 0x64F2, 0x5F6E, 0x5A4C, 0x5580,
   0x5102, 0x4CCA, 0x48D0, 0x450E, 0x4182, 0x3E24, 0x3AF2, 0x37E8,
   0x34FD, 0x2F97, 0x2AA5, 0x2618, 0x21E4, 0x1DFE, 0x1A5C, 0x16F8,
   0x13CA, 0x10CE, 0x0DFE, 0x0B57, 0x08D4, 0x0673, 0x0431, 0x020B,
};

const uint16_t frsqrte_delta[] =
{
   0x7A4, 0x700, 0x670, 0x5F2, 0x584, 0x524, 0x4CC, 0x47E,
   0x43A, 0x3FA, 0x3C2, 0x38E, 0x35E, 0x332, 0x30A, 0x2E6,
   0x568, 0x4F3, 0x48D, 0x435, 0x3E7, 0x3A2, 0x365, 0x32E,
   0x2FC, 0x2D0, 0x2A8, 0x283, 0x261, 0x243, 0x226, 0x20B,
};

float
ppc_estimate_reciprocal(float v)
{
   auto bits = get_float_bits(v);

   // Check for an infinite or NaN input.
   if (bits.exponent == bits.exponent_max) {
      if (bits.mantissa == 0) {
         return std::copysign(0.0f, v);
      } else {
         std::feraiseexcept(FE_INVALID);
         return make_quiet(v);
      }
   }

   // Check for a zero or denormal input.
   int exponent = bits.exponent;
   uint32_t mantissa = bits.mantissa;
   if (exponent == 0) {
      if (mantissa == 0) {
         std::feraiseexcept(FE_DIVBYZERO);
         return std::copysign(std::numeric_limits<float>::infinity(), v);
      } else if (mantissa < 0x200000) {
         std::feraiseexcept(FE_OVERFLOW | FE_INEXACT);
         return std::copysign(std::numeric_limits<float>::max(), v);
      } else if (mantissa < 0x400000) {
         exponent = -1;
         mantissa = (mantissa << 2) & 0x7FFFFF;
      } else {
         mantissa = (mantissa << 1) & 0x7FFFFF;
      }
   }

   // Calculate the result.  The lookup result has 24 bits; the high 23 bits
   // are copied to the mantissa of the output (except for denormals), and
   // the lowest bit is copied to FPSCR[FI].
   int new_exponent = 253 - exponent;
   int table_index = mantissa >> 18;
   int delta_mult = (mantissa >> 8) & 0x3FF;
   uint32_t lookup_result = ((fres_base[table_index] << 10)
                             - (fres_delta[table_index] * delta_mult));
   uint32_t new_mantissa = lookup_result >> 1;
   bool fpscr_FI = lookup_result & 1;

   // Denormalize the result if necessary.
   if (new_exponent <= 0) {
      fpscr_FI |= new_mantissa & 1;
      new_mantissa = (new_mantissa >> 1) | 0x400000;
      if (new_exponent < 0) {
         fpscr_FI |= new_mantissa & 1;
         new_mantissa >>= 1;
         new_exponent = 0;
      }
      if (fpscr_FI) {
         std::feraiseexcept(FE_UNDERFLOW);
      }
   }

   if (fpscr_FI) {
      std::feraiseexcept(FE_INEXACT);
   }
   bits.exponent = new_exponent;
   bits.mantissa = new_mantissa;
   return bits.v;
}

double
ppc_estimate_reciprocal_root(double v)
{
   auto bits = get_float_bits(v);

   // Check for an infinite or NaN input.
   if (bits.exponent == bits.exponent_max) {
      if (bits.mantissa == 0) {
         if (bits.sign) {
            std::feraiseexcept(FE_INVALID);
            return make_nan<double>();
         } else {
            return 0.0;
         }
      } else {
         std::feraiseexcept(FE_INVALID);
         return make_quiet(v);
      }
   }

   // Check for a zero or denormal input.
   int exponent = bits.exponent;
   uint64_t mantissa = bits.mantissa;
   if (exponent == 0) {
      if (mantissa == 0) {
         std::feraiseexcept(FE_DIVBYZERO);
         return std::copysign(std::numeric_limits<float>::infinity(), v);
      } else {
         int shift = clz64(mantissa) - 11;
         mantissa = (mantissa << shift) & UINT64_C(0xFFFFFFFFFFFFF);
         exponent -= shift - 1;
      }
   }

   // Negative nonzero values are always invalid.  If we get this far, we
   // know the value is not zero or NaN.
   if (bits.sign) {
      std::feraiseexcept(FE_INVALID);
      return make_nan<double>();
   }

   // Calculate the result.
   int new_exponent = (3068 - exponent) / 2;
   int table_index = ((mantissa >> 48) & 15) | (exponent & 1 ? 0 : 16);
   int delta_mult = (mantissa >> 37) & 0x7FF;
   uint64_t lookup_result = ((frsqrte_base[table_index] << 11)
                             - (frsqrte_delta[table_index] * delta_mult));
   uint64_t new_mantissa = lookup_result << 26;

   bits.exponent = new_exponent;
   bits.mantissa = new_mantissa;
   return bits.v;
}

void
updateFEX_VX(cpu::Core *state)
{
   auto &fpscr = state->fpscr;

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

   // FP Enabled Exception Summary
   fpscr.fex =
        (fpscr.vx & fpscr.ve)
      | (fpscr.ox & fpscr.oe)
      | (fpscr.ux & fpscr.ue)
      | (fpscr.zx & fpscr.ze)
      | (fpscr.xx & fpscr.xe);
}


void
updateFX_FEX_VX(cpu::Core *state, uint32_t oldValue)
{
   auto &fpscr = state->fpscr;

   updateFEX_VX(state);

   // FP Exception Summary
   const uint32_t newBits = (oldValue ^ fpscr.value) & fpscr.value;
   if (newBits & FpscrFlags::AllExceptions) {
      fpscr.fx = 1;
   }
}

void
updateFPSCR(cpu::Core *state, uint32_t oldValue)
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

   updateFX_FEX_VX(state, oldValue);

   std::feclearexcept(FE_ALL_EXCEPT);
}

template<typename Type>
void
updateFPRF(cpu::Core *state, Type value)
{
   auto cls = std::fpclassify(value);
   auto flags = 0u;

   if (cls == FP_NAN) {
      flags |= FloatingPointResultFlags::ClassDescriptor;
      flags |= FloatingPointResultFlags::Unordered;
   } else if (value != 0) {
      if (value > 0) {
         flags |= FloatingPointResultFlags::Positive;
      } else {
         flags |= FloatingPointResultFlags::Negative;
      }
      if (cls == FP_INFINITE) {
         flags |= FloatingPointResultFlags::Unordered;
      } else if (cls == FP_SUBNORMAL) {
         flags |= FloatingPointResultFlags::ClassDescriptor;
      }
   } else {
      flags |= FloatingPointResultFlags::Equal;
      if (std::signbit(value)) {
         flags |= FloatingPointResultFlags::ClassDescriptor;
      }
   }

   state->fpscr.fprf = flags;
}

// Make sure both float and double versions are available to other sources:
template void updateFPRF(cpu::Core *state, float value);
template void updateFPRF(cpu::Core *state, double value);

void
updateFloatConditionRegister(cpu::Core *state)
{
   state->cr.cr1 = state->fpscr.cr1;
}

// Helper for fmuls/fmadds to round the second (frC) operand appropriately.
// May also need to modify the first operand, so both operands are passed
// by reference.
void
roundForMultiply(double *a, double *c)
{
   // The mantissa is truncated from 52 to 24 bits, so bit 27 (counting from
   // the LSB) is rounded.
   const uint64_t roundBit = UINT64_C(1) << 27;

   FloatBitsDouble aBits = get_float_bits(*a);
   FloatBitsDouble cBits = get_float_bits(*c);

   // If the second operand has no bits that would be rounded, this whole
   // function is a no-op, so skip out early.
   if (!(cBits.uv & ((roundBit << 1) - 1))) {
      return;
   }

   // If the first operand is zero, the result is always zero (even if the
   // second operand would round to infinity), so avoid generating any
   // exceptions.
   if (is_zero(*a)) {
      return;
   }

   // If the first operand is infinity and the second is not zero, the result
   // is always infinity; get out now so we don't have to worry about it in
   // normalization.
   if (is_infinity(*a)) {
      return;
   }

   // If the second operand is a denormal, we normalize it before rounding,
   // adjusting the exponent of the other operand accordingly.  If the
   // other operand becomes denormal, the product will round to zero in any
   // case, so we just abort and let the operation proceed normally.
   if (is_denormal(*c)) {
      auto cSign = cBits.sign;
      while (cBits.exponent == 0) {
         cBits.uv <<= 1;
         if (aBits.exponent == 0) {
            return;
         }
         aBits.exponent--;
      }
      cBits.sign = cSign;
   }

   // Perform the rounding.  If this causes the value to go to infinity,
   // we move a power of two to the other operand (if possible) for the
   // case of an FMA operation in which we need to keep precision for the
   // intermediate result.  Note that this particular rounding operation
   // ignores FPSCR[RN].
   cBits.uv &= -static_cast<int64_t>(roundBit);
   cBits.uv += cBits.uv & roundBit;
   if (is_infinity(cBits.v)) {
      cBits.exponent--;
      if (aBits.exponent == 0) {
         auto aSign = aBits.sign;
         aBits.uv <<= 1;
         aBits.sign = aSign;
      } else if (aBits.exponent < aBits.exponent_max - 1) {
         aBits.exponent++;
      } else {
         // The product will overflow anyway, so just leave the first
         // operand alone and let the host FPU raise exceptions as
         // appropriate.
      }
   }

   *a = aBits.v;
   *c = cBits.v;
}

// Helper for fmadds to properly round to single precision.  Assumes
// round-to-nearest mode.
CLANG_FPU_BUG_WORKAROUND
float
roundFMAResultToSingle(double result, double a, double b, double c)
{
   if (is_zero(a) || is_zero(b) || is_zero(c)) {
      return static_cast<float>(result);  // Can't lose precision if one addend is zero.
   }

   auto resultBits = get_float_bits(result);

   if (resultBits.exponent < 874 || resultBits.exponent > 1150) {
      return static_cast<float>(result);  // Out of range or inf/NaN.
   }

   uint64_t centerValue = 1<<28;
   uint64_t centerMask = (centerValue << 1) - 1;
   if (resultBits.exponent < 897) {
      centerValue <<= 897 - resultBits.exponent;
      centerMask <<= 897 - resultBits.exponent;
   }
   if ((resultBits.mantissa & centerMask) != centerValue) {
      return static_cast<float>(result);  // Not exactly between two single-precision values.
   }

   // Repeat the operation in round-toward-zero mode to determine which way
   // to round the result.
   const int oldRound = fegetround();
   fesetround(FE_TOWARDZERO);
   feclearexcept(FE_INEXACT);

   double test = fma(a, c, b);

   fesetround(oldRound);

   // We only need to adjust the result if there were dropped bits.
   // If the truncated result is equal to the original (rounded) result,
   // it means the infinitely precise result is greater than the rounded
   // value, so we add 1 ulp (unit in the last place) to force the value
   // to round up when we convert it to float; likewise, if the truncated
   // result is different, we subtract 1 ulp to force rounding down.
   // In both cases, we know the result mantissa is nonzero so it's safe
   // to just increment or decrement the entire value as an integer.
   if (fetestexcept(FE_INEXACT)) {
      if (get_float_bits(test).uv == resultBits.uv) {
         resultBits.uv++;
      } else {
         resultBits.uv--;
      }
   }

   return static_cast<float>(resultBits.v);
}


// Floating Arithmetic
enum FPArithOperator {
    FPAdd,
    FPSub,
    FPMul,
    FPDiv,
};
template<FPArithOperator op, typename Type>
static void
fpArithGeneric(cpu::Core *state, Instruction instr)
{
   double a, b;
   Type d;

   a = state->fpr[instr.frA].value;
   b = state->fpr[op == FPMul ? instr.frC : instr.frB].value;

   const bool vxsnan = is_signalling_nan(a) || is_signalling_nan(b);
   bool vxisi, vximz, vxidi, vxzdz, zx;

   switch (op) {
   case FPAdd:
      vxisi = is_infinity(a) && is_infinity(b) && std::signbit(a) != std::signbit(b);
      vximz = false;
      vxidi = false;
      vxzdz = false;
      zx = false;
      break;
   case FPSub:
      vxisi = is_infinity(a) && is_infinity(b) && std::signbit(a) == std::signbit(b);
      vximz = false;
      vxidi = false;
      vxzdz = false;
      zx = false;
      break;
   case FPMul:
      vxisi = false;
      vximz = (is_infinity(a) && is_zero(b)) || (is_zero(a) && is_infinity(b));
      vxidi = false;
      vxzdz = false;
      zx = false;
      break;
   case FPDiv:
      vxisi = false;
      vximz = false;
      vxidi = is_infinity(a) && is_infinity(b);
      vxzdz = is_zero(a) && is_zero(b);
      zx = !(vxzdz || vxsnan) && is_zero(b);
      break;
   }

   const uint32_t oldFPSCR = state->fpscr.value;
   state->fpscr.vxsnan |= vxsnan;
   state->fpscr.vxisi |= vxisi;
   state->fpscr.vximz |= vximz;
   state->fpscr.vxidi |= vxidi;
   state->fpscr.vxzdz |= vxzdz;

   if ((vxsnan || vxisi || vximz || vxidi || vxzdz) && state->fpscr.ve) {
      updateFX_FEX_VX(state, oldFPSCR);
   } else if (zx && state->fpscr.ze) {
      state->fpscr.zx = 1;
      updateFX_FEX_VX(state, oldFPSCR);
   } else {
      if (is_nan(a)) {
         d = static_cast<Type>(make_quiet(a));
      } else if (is_nan(b)) {
         d = static_cast<Type>(make_quiet(b));
      } else if (vxisi || vximz || vxidi || vxzdz) {
         d = make_nan<Type>();
      } else {
         // The Espresso appears to use double precision arithmetic even for
         // single-precision instructions (for example, 2^128 * 0.5 does not
         // cause overflow), so we do the same here.
         switch (op) {
         case FPAdd:
            d = static_cast<Type>(a + b);
            break;
         case FPSub:
            d = static_cast<Type>(a - b);
            break;
         case FPMul:
            // But!  The second operand to a single-precision multiply
            // operation is rounded to 24 bits.
            if constexpr (std::is_same<Type, float>::value) {
               roundForMultiply(&a, &b);
            }
            d = static_cast<Type>(a * b);
            break;
         case FPDiv:
            d = static_cast<Type>(a / b);
            break;
         }
      }

      // The PowerPC signals underflow if the result is tiny before rounding;
      // this differs from x86, which signals underflow only if the result
      // is tiny after rounding.  Sadly, IEEE allows both of these behaviors,
      // so we can't just say that one is broken, and we have to handle this
      // case manually.  If the rounded result is equal in magnitude to the
      // minimum normal value for the type, then the unrounded result may
      // have had a smaller magnitude, so we temporarily set the rounding
      // mode to round-toward-zero and repeat the operation, discarding the
      // result but allowing it to set the underflow flag if appropriate.
      // (In round-toward-zero mode, any value which is tiny before rounding
      // will also be tiny after rounding.)
      if (possibleUnderflow<Type>(d)) {
         const int oldRound = fegetround();
         fesetround(FE_TOWARDZERO);

         // Use volatile variables to force the operation to be performed
         // and prevent the previous result from being reused.
         volatile double bTemp = b;
         volatile Type dummy;
         switch (op) {
         case FPAdd:
            dummy = static_cast<Type>(a + bTemp);
            break;
         case FPSub:
            dummy = static_cast<Type>(a - bTemp);
            break;
         case FPMul:
            // a and b have already been rounded if necessary.
            dummy = static_cast<Type>(a * bTemp);
            break;
         case FPDiv:
            dummy = static_cast<Type>(a / bTemp);
            break;
         }

         fesetround(oldRound);
      }

      if constexpr (std::is_same<Type, float>::value) {
         double dd = extend_float(d);
         state->fpr[instr.frD].paired0 = dd;
         state->fpr[instr.frD].paired1 = dd;
         updateFPRF(state, d);
      } else {
         state->fpr[instr.frD].value = d;
         updateFPRF(state, d);
      }

      updateFPSCR(state, oldFPSCR);
   }

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Floating Add Double
static void
fadd(cpu::Core *state, Instruction instr)
{
   fpArithGeneric<FPAdd, double>(state, instr);
}

// Floating Add Single
static void
fadds(cpu::Core *state, Instruction instr)
{
   fpArithGeneric<FPAdd, float>(state, instr);
}

// Floating Divide Double
static void
fdiv(cpu::Core *state, Instruction instr)
{
   fpArithGeneric<FPDiv, double>(state, instr);
}

// Floating Divide Single
static void
fdivs(cpu::Core *state, Instruction instr)
{
   fpArithGeneric<FPDiv, float>(state, instr);
}

// Floating Multiply Double
static void
fmul(cpu::Core *state, Instruction instr)
{
   fpArithGeneric<FPMul, double>(state, instr);
}

// Floating Multiply Single
static void
fmuls(cpu::Core *state, Instruction instr)
{
   fpArithGeneric<FPMul, float>(state, instr);
}

// Floating Subtract Double
static void
fsub(cpu::Core *state, Instruction instr)
{
   fpArithGeneric<FPSub, double>(state, instr);
}

// Floating Subtract Single
static void
fsubs(cpu::Core *state, Instruction instr)
{
   fpArithGeneric<FPSub, float>(state, instr);
}

// Floating Reciprocal Estimate Single
static void
fres(cpu::Core *state, Instruction instr)
{
   double b;
   float d;
   b = state->fpr[instr.frB].value;

   const bool vxsnan = is_signalling_nan(b);
   const bool zx = is_zero(b);

   const uint32_t oldFPSCR = state->fpscr.value;
   state->fpscr.vxsnan |= vxsnan;

   if (vxsnan && state->fpscr.ve) {
      updateFX_FEX_VX(state, oldFPSCR);
   } else if (zx && state->fpscr.ze) {
      state->fpscr.zx = 1;
      updateFX_FEX_VX(state, oldFPSCR);
   } else {
      d = ppc_estimate_reciprocal(static_cast<float>(b));
      state->fpr[instr.frD].paired0 = d;
      state->fpr[instr.frD].paired1 = d;
      updateFPRF(state, d);
      state->fpscr.zx |= zx;
      if (std::fetestexcept(FE_INEXACT)) {
         // On inexact result, fres sets FPSCR[FI] without also setting
         // FPSCR[XX].
         std::feclearexcept(FE_INEXACT);
         updateFPSCR(state, oldFPSCR);
         state->fpscr.fi = 1;
      } else {
         updateFPSCR(state, oldFPSCR);
      }
   }

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Floating Reciprocal Square Root Estimate
static void
frsqrte(cpu::Core *state, Instruction instr)
{
   double b, d;
   b = state->fpr[instr.frB].value;

   const bool vxsnan = is_signalling_nan(b);
   const bool vxsqrt = !vxsnan && b < 0.0;
   const bool zx = is_zero(b);

   const uint32_t oldFPSCR = state->fpscr.value;
   state->fpscr.vxsnan |= vxsnan;
   state->fpscr.vxsqrt |= vxsqrt;

   if ((vxsnan || vxsqrt) && state->fpscr.ve) {
      updateFX_FEX_VX(state, oldFPSCR);
   } else if (zx && state->fpscr.ze) {
      state->fpscr.zx = 1;
      updateFX_FEX_VX(state, oldFPSCR);
   } else {
      d = ppc_estimate_reciprocal_root(b);
      state->fpr[instr.frD].value = d;
      updateFPRF(state, d);
      state->fpscr.zx |= zx;
      updateFPSCR(state, oldFPSCR);
   }

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

static void
fsel(cpu::Core *state, Instruction instr)
{
   double a, b, c, d;
   a = state->fpr[instr.frA].value;
   b = state->fpr[instr.frB].value;
   c = state->fpr[instr.frC].value;

   if (a >= 0.0) {
      d = c;
   } else {
      d = b;
   }

   state->fpr[instr.frD].value = d;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Fused multiply-add instructions
enum FMAFlags
{
   FMASubtract   = 1 << 0, // Subtract instead of add
   FMANegate     = 1 << 1, // Negate result
   FMASinglePrec = 1 << 2, // Round result to single precision
};

template<unsigned flags>
static void
fmaGeneric(cpu::Core *state, Instruction instr)
{
   double a, b, c, d;
   a = state->fpr[instr.frA].value;
   b = state->fpr[instr.frB].value;
   c = state->fpr[instr.frC].value;

   const double addend = (flags & FMASubtract) ? -b : b;

   const bool vxsnan = is_signalling_nan(a) || is_signalling_nan(b) || is_signalling_nan(c);
   const bool vximz = (is_infinity(a) && is_zero(c)) || (is_zero(a) && is_infinity(c));
   const bool vxisi = (!vximz && !is_nan(a) && !is_nan(c)
                       && (is_infinity(a) || is_infinity(c)) && is_infinity(b)
                       && (std::signbit(a) ^ std::signbit(c)) != std::signbit(addend));

   const uint32_t oldFPSCR = state->fpscr.value;
   state->fpscr.vxsnan |= vxsnan;
   state->fpscr.vxisi |= vxisi;
   state->fpscr.vximz |= vximz;

   if ((vxsnan || vxisi || vximz) && state->fpscr.ve) {
      updateFX_FEX_VX(state, oldFPSCR);
   } else {
      if (is_nan(a)) {
         d = make_quiet(a);
      } else if (is_nan(b)) {
         d = make_quiet(b);
      } else if (is_nan(c)) {
         d = make_quiet(c);
      } else if (vxisi || vximz) {
         d = make_nan<double>();
      } else {
         if (flags & FMASinglePrec) {
            roundForMultiply(&a, &c);
         }

         d = std::fma(a, c, addend);

         bool checkUnderflow;
         if (flags & FMASinglePrec) {
            checkUnderflow = possibleUnderflow<float>(static_cast<float>(d));
         } else {
            checkUnderflow = possibleUnderflow<double>(d);
         }

         if (checkUnderflow) {
            const int oldRound = fegetround();
            fesetround(FE_TOWARDZERO);

            volatile double addendTemp = addend;
            if (flags & FMASinglePrec) {
               volatile float dummy;
               dummy = static_cast<float>(std::fma(a, c, addendTemp));
            } else {
               volatile double dummy;
               dummy = std::fma(a, c, addendTemp);
            }

            fesetround(oldRound);
         }

         if (flags & FMANegate) {
            d = -d;
         }
      }

      if (flags & FMASinglePrec) {
         float dFloat;
         if (state->fpscr.rn == FloatingPointRoundMode::Nearest) {
            dFloat = roundFMAResultToSingle(d, a, addend, c);
         } else {
            dFloat = static_cast<float>(d);
         }
         d = extend_float(dFloat);
         state->fpr[instr.frD].paired0 = d;
         state->fpr[instr.frD].paired1 = d;
         updateFPRF(state, dFloat);
      } else {
         state->fpr[instr.frD].value = d;
         updateFPRF(state, d);
      }

      updateFPSCR(state, oldFPSCR);
   }

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Floating Multiply-Add
static void
fmadd(cpu::Core *state, Instruction instr)
{
   return fmaGeneric<0>(state, instr);
}

// Floating Multiply-Add Single
static void
fmadds(cpu::Core *state, Instruction instr)
{
   return fmaGeneric<FMASinglePrec>(state, instr);
}

// Floating Multiply-Sub
static void
fmsub(cpu::Core *state, Instruction instr)
{
   return fmaGeneric<FMASubtract>(state, instr);
}

// Floating Multiply-Sub Single
static void
fmsubs(cpu::Core *state, Instruction instr)
{
   return fmaGeneric<FMASubtract | FMASinglePrec>(state, instr);
}

// Floating Negative Multiply-Add
static void
fnmadd(cpu::Core *state, Instruction instr)
{
   return fmaGeneric<FMANegate>(state, instr);
}

// Floating Negative Multiply-Add Single
static void
fnmadds(cpu::Core *state, Instruction instr)
{
   return fmaGeneric<FMANegate | FMASinglePrec>(state, instr);
}

// Floating Negative Multiply-Sub
static void
fnmsub(cpu::Core *state, Instruction instr)
{
   return fmaGeneric<FMANegate | FMASubtract>(state, instr);
}

// Floating Negative Multiply-Sub Single
static void
fnmsubs(cpu::Core *state, Instruction instr)
{
   return fmaGeneric<FMANegate | FMASubtract | FMASinglePrec>(state, instr);
}

// fctiw/fctiwz common implementation
static void
fctiwGeneric(cpu::Core *state, Instruction instr, FloatingPointRoundMode roundMode)
{
   double b;
   int32_t bi;
   b = state->fpr[instr.frB].value;

   const bool vxsnan = is_signalling_nan(b);
   bool vxcvi, fi;

   if (is_nan(b)) {
      vxcvi = true;
      fi = false;
      bi = INT_MIN;
   } else if (b > static_cast<double>(INT_MAX)) {
      vxcvi = true;
      fi = false;
      bi = INT_MAX;
   } else if (b < static_cast<double>(INT_MIN)) {
      vxcvi = true;
      fi = false;
      bi = INT_MIN;
   } else {
      vxcvi = false;
      switch (roundMode) {
      case FloatingPointRoundMode::Nearest:
         // We have to use nearbyint() instead of round() here, because
         // round() rounds 0.5 away from zero instead of to the nearest
         // even integer.  nearbyint() is dependent on the host's FPU
         // rounding mode, but since that will reflect FPSCR here, it's
         // safe to use.
         bi = static_cast<int32_t>(std::nearbyint(b));
         break;
      case FloatingPointRoundMode::Zero:
         bi = static_cast<int32_t>(std::trunc(b));
         break;
      case FloatingPointRoundMode::Positive:
         bi = static_cast<int32_t>(std::ceil(b));
         break;
      case FloatingPointRoundMode::Negative:
         bi = static_cast<int32_t>(std::floor(b));
         break;
      }
      fi = get_float_bits(b).exponent < 1075 && bi != b;
   }

   const uint32_t oldFPSCR = state->fpscr.value;
   state->fpscr.vxsnan |= vxsnan;
   state->fpscr.vxcvi |= vxcvi;

   if ((vxsnan || vxcvi) && state->fpscr.ve) {
      state->fpscr.fr = 0;
      state->fpscr.fi = 0;
      updateFX_FEX_VX(state, oldFPSCR);
   } else {
      state->fpr[instr.frD].iw1 = bi;
      state->fpr[instr.frD].iw0 = 0xFFF80000 | (is_negative_zero(b) ? 1 : 0);
      updateFPSCR(state, oldFPSCR);
      // We need to set FPSCR[FI] manually since the rounding functions
      // don't always raise inexact exceptions.
      if (fi) {
         state->fpscr.fi = 1;
         state->fpscr.xx = 1;
         updateFX_FEX_VX(state, oldFPSCR);
      }
   }

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

static void
fctiw(cpu::Core *state, Instruction instr)
{
   return fctiwGeneric(state, instr, static_cast<FloatingPointRoundMode>(state->fpscr.rn));
}

// Floating Convert to Integer Word with Round toward Zero
static void
fctiwz(cpu::Core *state, Instruction instr)
{
   return fctiwGeneric(state, instr, FloatingPointRoundMode::Zero);
}

// Floating Round to Single
static void
frsp(cpu::Core *state, Instruction instr)
{
   auto b = state->fpr[instr.frB].value;
   auto vxsnan = is_signalling_nan(b);

   const uint32_t oldFPSCR = state->fpscr.value;
   state->fpscr.vxsnan |= vxsnan;

   if (vxsnan && state->fpscr.ve) {
      updateFX_FEX_VX(state, oldFPSCR);
   } else {
      auto d = static_cast<float>(b);
      state->fpr[instr.frD].paired0 = d;
      // frD(ps1) is left undefined in the 750CL manual, but the processor
      // actually copies the result to ps1 like other single-precision
      // instructions.
      state->fpr[instr.frD].paired1 = d;
      updateFPRF(state, d);
      updateFPSCR(state, oldFPSCR);
   }

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// TODO: do fabs/fnabs/fneg behave like fmr w.r.t. paired singles?
// Floating Absolute Value
static void
fabs(cpu::Core *state, Instruction instr)
{
   uint64_t b, d;

   b = state->fpr[instr.frB].idw;
   d = clear_bit(b, 63);
   state->fpr[instr.frD].idw = d;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Floating Negative Absolute Value
static void
fnabs(cpu::Core *state, Instruction instr)
{
   uint64_t b, d;

   b = state->fpr[instr.frB].idw;
   d = set_bit(b, 63);
   state->fpr[instr.frD].idw = d;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Floating Move Register
static void
fmr(cpu::Core *state, Instruction instr)
{
   state->fpr[instr.frD].idw = state->fpr[instr.frB].idw;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Floating Negate
static void
fneg(cpu::Core *state, Instruction instr)
{
   uint64_t b, d;

   b = state->fpr[instr.frB].idw;
   d = flip_bit(b, 63);
   state->fpr[instr.frD].idw = d;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Move from FPSCR
static void
mffs(cpu::Core *state, Instruction instr)
{
   state->fpr[instr.frD].iw1 = state->fpscr.value;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Move to FPSCR Bit 0
static void
mtfsb0(cpu::Core *state, Instruction instr)
{
   state->fpscr.value = clear_bit(state->fpscr.value, 31 - instr.crbD);
   updateFEX_VX(state);
   if (instr.crbD >= 30) {
      cpu::this_core::updateRoundingMode();
   }

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Move to FPSCR Bit 1
static void
mtfsb1(cpu::Core *state, Instruction instr)
{
   const uint32_t oldValue = state->fpscr.value;
   state->fpscr.value = set_bit(state->fpscr.value, 31 - instr.crbD);
   updateFX_FEX_VX(state, oldValue);
   if (instr.crbD >= 30) {
      cpu::this_core::updateRoundingMode();
   }

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Move to FPSCR Fields
static void
mtfsf(cpu::Core *state, Instruction instr)
{
   const uint32_t value = state->fpr[instr.frB].iw1;
   for (int field = 0; field < 8; field++) {
      // Technically field 0 is at the high end, but as long as the bit
      // position in the mask and the field we operate on match up, it
      // doesn't matter which direction we go in.  So we use host bit
      // order for simplicity.
      if (get_bit(instr.fm, field)) {
         const uint32_t mask = make_bitmask(4 * field, 4 * field + 3);
         state->fpscr.value &= ~mask;
         state->fpscr.value |= value & mask;
      }
   }
   updateFEX_VX(state);
   if (get_bit(instr.fm, 0)) {
      cpu::this_core::updateRoundingMode();
   }

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Move to FPSCR Field Immediate
static void
mtfsfi(cpu::Core *state, Instruction instr)
{
   const int shift = 4 * (7 - instr.crfD);
   state->fpscr.value &= ~(0xF << shift);
   state->fpscr.value |= instr.imm << shift;
   updateFEX_VX(state);
   if (instr.crfD == 7) {
      cpu::this_core::updateRoundingMode();
   }

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
   RegisterInstruction(mffs);
   RegisterInstruction(mtfsb0);
   RegisterInstruction(mtfsb1);
   RegisterInstruction(mtfsf);
   RegisterInstruction(mtfsfi);
}
