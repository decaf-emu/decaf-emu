#include <cfenv>
#include <cmath>
#include "interpreter_insreg.h"
#include "interpreter_float.h"
#include <common/floatutils.h>
#include <common/platform_compiler.h>

// Register move / sign bit manipulation
enum MoveMode
{
   MoveDirect,
   MoveNegate,
   MoveAbsolute,
   MoveNegAbsolute,
};

template<MoveMode mode>
static void
moveGeneric(cpu::Core *state, Instruction instr)
{
   uint32_t b0, b1, d0, d1;
   const bool ps0_nan = is_signalling_nan(state->fpr[instr.frB].paired0);
   const bool ps1_nan = is_signalling_nan(state->fpr[instr.frB].paired1);
   if (ps0_nan) {
      b0 = truncate_double_bits(state->fpr[instr.frB].idw);
   } else {
      // We have to round ps0 if it has excess precision, so we can't just
      // chop off the trailing bits.
      b0 = bit_cast<uint32_t>(static_cast<float>(state->fpr[instr.frB].paired0));
   }
   // ps1 is always truncated, whether NaN or not.
   b1 = bit_cast<uint32_t>(truncate_double(state->fpr[instr.frB].paired1));

   switch (mode) {
   case MoveDirect:
      d0 = b0;
      d1 = b1;
      break;
   case MoveNegate:
      d0 = b0 ^ 0x80000000;
      d1 = b1 ^ 0x80000000;
      break;
   case MoveAbsolute:
      d0 = b0 & ~0x80000000;
      d1 = b1 & ~0x80000000;
      break;
   case MoveNegAbsolute:
      d0 = b0 | 0x80000000;
      d1 = b1 | 0x80000000;
      break;
   }

   if (!ps0_nan) {
      state->fpr[instr.frD].paired0 = static_cast<double>(bit_cast<float>(d0));
   } else {
      state->fpr[instr.frD].idw = extend_float_nan_bits(d0);
   }
   if (!ps1_nan) {
      state->fpr[instr.frD].paired1 = static_cast<double>(bit_cast<float>(d1));
   } else {
      state->fpr[instr.frD].idw_paired1 = extend_float_nan_bits(d1);
   }

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Move Register
static void
ps_mr(cpu::Core *state, Instruction instr)
{
   return moveGeneric<MoveDirect>(state, instr);
}

// Negate
static void
ps_neg(cpu::Core *state, Instruction instr)
{
   return moveGeneric<MoveNegate>(state, instr);
}

// Absolute
static void
ps_abs(cpu::Core *state, Instruction instr)
{
   return moveGeneric<MoveAbsolute>(state, instr);
}

// Negative Absolute
static void
ps_nabs(cpu::Core *state, Instruction instr)
{
   return moveGeneric<MoveNegAbsolute>(state, instr);
}

// Paired-single arithmetic
enum PSArithOperator {
    PSAdd,
    PSSub,
    PSMul,
    PSDiv,
};

// Returns whether a result value was written (i.e., not aborted by an
// exception).
template<PSArithOperator op, int slotA, int slotB>
static bool
psArithSingle(cpu::Core *state, Instruction instr, float *result)
{
   double a, b;
   if (slotA == 0) {
      a = state->fpr[instr.frA].paired0;
   } else {
      a = state->fpr[instr.frA].paired1;
   }
   if (slotB == 0) {
      b = state->fpr[op == PSMul ? instr.frC : instr.frB].paired0;
   } else {
      b = state->fpr[op == PSMul ? instr.frC : instr.frB].paired1;
   }

   const bool vxsnan = is_signalling_nan(a) || is_signalling_nan(b);
   bool vxisi, vximz, vxidi, vxzdz, zx;
   switch (op) {
   case PSAdd:
      vxisi = is_infinity(a) && is_infinity(b) && std::signbit(a) != std::signbit(b);
      vximz = false;
      vxidi = false;
      vxzdz = false;
      zx = false;
      break;
   case PSSub:
      vxisi = is_infinity(a) && is_infinity(b) && std::signbit(a) == std::signbit(b);
      vximz = false;
      vxidi = false;
      vxzdz = false;
      zx = false;
      break;
   case PSMul:
      vxisi = false;
      vximz = (is_infinity(a) && is_zero(b)) || (is_zero(a) && is_infinity(b));
      vxidi = false;
      vxzdz = false;
      zx = false;
      break;
   case PSDiv:
      vxisi = false;
      vximz = false;
      vxidi = is_infinity(a) && is_infinity(b);
      vxzdz = is_zero(a) && is_zero(b);
      zx = !(vxzdz || vxsnan) && is_zero(b);
      break;
   }

   state->fpscr.vxsnan |= vxsnan;
   state->fpscr.vxisi |= vxisi;
   state->fpscr.vximz |= vximz;
   state->fpscr.vxidi |= vxidi;
   state->fpscr.vxzdz |= vxzdz;
   state->fpscr.zx |= zx;

   const bool vxEnabled = (vxsnan || vxisi || vximz || vxidi || vxzdz) && state->fpscr.ve;
   const bool zxEnabled = zx && state->fpscr.ze;
   if (vxEnabled || zxEnabled) {
      return false;
   }

   float d;
   if (is_nan(a)) {
      d = make_quiet(truncate_double(a));
   } else if (is_nan(b)) {
      d = make_quiet(truncate_double(b));
   } else if (vxisi || vximz || vxidi || vxzdz) {
      d = make_nan<float>();
   } else {
      switch (op) {
      case PSAdd:
         d = static_cast<float>(a + b);
         break;
      case PSSub:
         d = static_cast<float>(a - b);
         break;
      case PSMul:
         if (slotB == 0) {
            roundForMultiply(&a, &b);  // Not necessary for slot 1.
         }
         d = static_cast<float>(a * b);
         break;
      case PSDiv:
         d = static_cast<float>(a / b);
         break;
      }

      if (possibleUnderflow<float>(d)) {
         const int oldRound = fegetround();
         fesetround(FE_TOWARDZERO);

         volatile double bTemp = b;
         volatile float dummy;
         switch (op) {
         case PSAdd:
            dummy = static_cast<float>(a + bTemp);
            break;
         case PSSub:
            dummy = static_cast<float>(a - bTemp);
            break;
         case PSMul:
            dummy = static_cast<float>(a * bTemp);
            break;
         case PSDiv:
            dummy = static_cast<float>(a / bTemp);
            break;
         }
         fesetround(oldRound);
      }
   }

   *result = d;
   return true;
}

template<PSArithOperator op, int slotB0, int slotB1>
static void
psArithGeneric(cpu::Core *state, Instruction instr)
{
   const uint32_t oldFPSCR = state->fpscr.value;

   float d0, d1;
   const bool wrote0 = psArithSingle<op, 0, slotB0>(state, instr, &d0);
   const bool wrote1 = psArithSingle<op, 1, slotB1>(state, instr, &d1);
   if (wrote0 && wrote1) {
      state->fpr[instr.frD].paired0 = extend_float(d0);
      state->fpr[instr.frD].paired1 = extend_float(d1);
   }

   if (wrote0) {
      updateFPRF(state, d0);
   }
   updateFPSCR(state, oldFPSCR);

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Add
static void
ps_add(cpu::Core *state, Instruction instr)
{
   return psArithGeneric<PSAdd, 0, 1>(state, instr);
}

// Subtract
static void
ps_sub(cpu::Core *state, Instruction instr)
{
   return psArithGeneric<PSSub, 0, 1>(state, instr);
}

// Multiply
static void
ps_mul(cpu::Core *state, Instruction instr)
{
   return psArithGeneric<PSMul, 0, 1>(state, instr);
}

static void
ps_muls0(cpu::Core *state, Instruction instr)
{
   return psArithGeneric<PSMul, 0, 0>(state, instr);
}

static void
ps_muls1(cpu::Core *state, Instruction instr)
{
   return psArithGeneric<PSMul, 1, 1>(state, instr);
}

// Divide
static void
ps_div(cpu::Core *state, Instruction instr)
{
   return psArithGeneric<PSDiv, 0, 1>(state, instr);
}

template<int slot>
static void
psSumGeneric(cpu::Core *state, Instruction instr) CLANG_FPU_BUG_WORKAROUND
{
   const uint32_t oldFPSCR = state->fpscr.value;
   float d;

   if (psArithSingle<PSAdd, 0, 1>(state, instr, &d)) {
      updateFPRF(state, d);

      if (slot == 0) {
          state->fpr[instr.frD].paired0 = extend_float(d);
          state->fpr[instr.frD].idw_paired1 = state->fpr[instr.frC].idw_paired1;
      } else {
          float ps0;

          if (is_nan(state->fpr[instr.frC].paired0)) {
             ps0 = truncate_double(state->fpr[instr.frC].paired0);
          } else {
             const auto inexact = !!std::fetestexcept(FE_INEXACT);
             const auto overflow = !!std::fetestexcept(FE_OVERFLOW);

             ps0 = static_cast<float>(state->fpr[instr.frC].paired0);

             if (!inexact) {
                 std::feclearexcept(FE_INEXACT);
             }

             if (!overflow) {
                 std::feclearexcept(FE_OVERFLOW);
             }
          }

          state->fpr[instr.frD].paired0 = extend_float(ps0);
          state->fpr[instr.frD].paired1 = extend_float(d);
      }
   }

   updateFPSCR(state, oldFPSCR);

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Sum High
static void
ps_sum0(cpu::Core *state, Instruction instr)
{
   return psSumGeneric<0>(state, instr);
}

// Sum Low
static void
ps_sum1(cpu::Core *state, Instruction instr)
{
   return psSumGeneric<1>(state, instr);
}

// Fused multiply-add instructions
enum FMAFlags
{
   FMASubtract   = 1 << 0, // Subtract instead of add
   FMANegate     = 1 << 1, // Negate result
};

// Returns whether a result value was written (i.e., not aborted by an
// exception).
template<unsigned flags, int slotAB, int slotC>
static bool
fmaSingle(cpu::Core *state, Instruction instr, float *result)
{
   double a, b, c;
   if (slotAB == 0) {
      a = state->fpr[instr.frA].paired0;
      b = state->fpr[instr.frB].paired0;
   } else {
      a = state->fpr[instr.frA].paired1;
      b = state->fpr[instr.frB].paired1;
   }
   if (slotC == 0) {
      c = state->fpr[instr.frC].paired0;
   } else {
      c = state->fpr[instr.frC].paired1;
   }
   const double addend = (flags & FMASubtract) ? -b : b;

   const bool vxsnan = is_signalling_nan(a) || is_signalling_nan(b) || is_signalling_nan(c);
   const bool vximz = (is_infinity(a) && is_zero(c)) || (is_zero(a) && is_infinity(c));
   const bool vxisi = (!vximz && !is_nan(a) && !is_nan(c)
                       && (is_infinity(a) || is_infinity(c)) && is_infinity(b)
                       && (std::signbit(a) ^ std::signbit(c)) != std::signbit(addend));

   state->fpscr.vxsnan |= vxsnan;
   state->fpscr.vxisi |= vxisi;
   state->fpscr.vximz |= vximz;

   if ((vxsnan || vxisi || vximz) && state->fpscr.ve) {
      return false;
   }

   float d;
   if (is_nan(a)) {
      d = make_quiet(truncate_double(a));
   } else if (is_nan(b)) {
      d = make_quiet(truncate_double(b));
   } else if (is_nan(c)) {
      d = make_quiet(truncate_double(c));
   } else if (vxisi || vximz) {
      d = make_nan<float>();
   } else {
      if (slotC == 0) {
         roundForMultiply(&a, &c);  // Not necessary for slot 1.
      }

      double d64 = std::fma(a, c, addend);
      if (state->fpscr.rn == espresso::FloatingPointRoundMode::Nearest) {
         d = roundFMAResultToSingle(d64, a, addend, c);
      } else {
         d = static_cast<float>(d64);
      }

      if (possibleUnderflow<float>(d)) {
         const int oldRound = fegetround();
         fesetround(FE_TOWARDZERO);

         volatile double addendTemp = addend;
         volatile float dummy;
         dummy = (float)std::fma(a, c, addendTemp);

         fesetround(oldRound);
      }

      if (flags & FMANegate) {
         d = -d;
      }
   }

   *result = d;
   return true;
}

template<unsigned flags, int slotC0, int slotC1>
static void
fmaGeneric(cpu::Core *state, Instruction instr)
{
   const uint32_t oldFPSCR = state->fpscr.value;

   float d0, d1;
   const bool wrote0 = fmaSingle<flags, 0, slotC0>(state, instr, &d0);
   const bool wrote1 = fmaSingle<flags, 1, slotC1>(state, instr, &d1);
   if (wrote0 && wrote1) {
      state->fpr[instr.frD].paired0 = extend_float(d0);
      state->fpr[instr.frD].paired1 = extend_float(d1);
   }

   if (wrote0) {
      updateFPRF(state, d0);
   }
   updateFPSCR(state, oldFPSCR);

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

static void
ps_madd(cpu::Core *state, Instruction instr)
{
   return fmaGeneric<0, 0, 1>(state, instr);
}

static void
ps_madds0(cpu::Core *state, Instruction instr)
{
   return fmaGeneric<0, 0, 0>(state, instr);
}

static void
ps_madds1(cpu::Core *state, Instruction instr)
{
   return fmaGeneric<0, 1, 1>(state, instr);
}

static void
ps_msub(cpu::Core *state, Instruction instr)
{
   return fmaGeneric<FMASubtract, 0, 1>(state, instr);
}

static void
ps_nmadd(cpu::Core *state, Instruction instr)
{
   return fmaGeneric<FMANegate, 0, 1>(state, instr);
}

static void
ps_nmsub(cpu::Core *state, Instruction instr)
{
   return fmaGeneric<FMANegate | FMASubtract, 0, 1>(state, instr);
}

// Merge registers
enum MergeFlags
{
   MergeValue0 = 1 << 0,
   MergeValue1 = 1 << 1
};

template<unsigned flags = 0>
static void
mergeGeneric(cpu::Core *state, Instruction instr)
{
   float d0, d1;

   if (flags & MergeValue0) {
      if (!is_signalling_nan(state->fpr[instr.frA].paired1)) {
         d0 = static_cast<float>(state->fpr[instr.frA].paired1);
      } else {
         d0 = truncate_double(state->fpr[instr.frA].paired1);
      }
   } else {
      if (!is_signalling_nan(state->fpr[instr.frA].paired0)) {
         d0 = static_cast<float>(state->fpr[instr.frA].paired0);
      } else {
         d0 = truncate_double(state->fpr[instr.frA].paired0);
      }
   }

   // When inserting a double-precision value into slot 1, the value is
   // truncated rather than rounded.
   double d1_double;
   if (flags & MergeValue1) {
      d1_double = state->fpr[instr.frB].paired1;
   } else {
      d1_double = state->fpr[instr.frB].paired0;
   }
   auto d1_bits = get_float_bits(d1_double);
   if (d1_bits.exponent >= 1151 && d1_bits.exponent < 2047) {
      d1 = std::numeric_limits<float>::max();
   } else {
      d1 = truncate_double(d1_double);
   }

   state->fpr[instr.frD].paired0 = extend_float(d0);
   state->fpr[instr.frD].paired1 = extend_float(d1);

   // Don't leak any exceptions (inexact, overflow etc.) to later instructions.
   std::feclearexcept(FE_ALL_EXCEPT);

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

static void
ps_merge00(cpu::Core *state, Instruction instr)
{
   return mergeGeneric(state, instr);
}

static void
ps_merge01(cpu::Core *state, Instruction instr)
{
   return mergeGeneric<MergeValue1>(state, instr);
}

static void
ps_merge11(cpu::Core *state, Instruction instr)
{
   return mergeGeneric<MergeValue0 | MergeValue1>(state, instr);
}

static void
ps_merge10(cpu::Core *state, Instruction instr)
{
   return mergeGeneric<MergeValue0>(state, instr);
}

// Reciprocal
static void
ps_res(cpu::Core *state, Instruction instr)
{
   const double b0 = state->fpr[instr.frB].paired0;
   const double b1 = state->fpr[instr.frB].paired1;

   const bool vxsnan0 = is_signalling_nan(b0);
   const bool vxsnan1 = is_signalling_nan(b1);
   const bool zx0 = is_zero(b0);
   const bool zx1 = is_zero(b1);

   const uint32_t oldFPSCR = state->fpscr.value;
   state->fpscr.vxsnan |= vxsnan0 || vxsnan1;
   state->fpscr.zx |= zx0 || zx1;

   float d0, d1;
   auto write = true;

   if ((vxsnan0 && state->fpscr.ve) || (zx0 && state->fpscr.ze)) {
      write = false;
   } else {
      d0 = ppc_estimate_reciprocal(truncate_double(b0));
      updateFPRF(state, d0);
   }

   if ((vxsnan1 && state->fpscr.ve) || (zx1 && state->fpscr.ze)) {
      write = false;
   } else {
      d1 = ppc_estimate_reciprocal(truncate_double(b1));
   }

   if (write) {
      state->fpr[instr.frD].paired0 = extend_float(d0);
      state->fpr[instr.frD].paired1 = extend_float(d1);
   }

   if (std::fetestexcept(FE_INEXACT)) {
      // On inexact result, ps_res sets FPSCR[FI] without also setting
      // FPSCR[XX] (like fres).
      std::feclearexcept(FE_INEXACT);
      updateFPSCR(state, oldFPSCR);
      state->fpscr.fi = 1;
   } else {
      updateFPSCR(state, oldFPSCR);
   }

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Reciprocal Square Root
static void
ps_rsqrte(cpu::Core *state, Instruction instr)
{
   const double b0 = state->fpr[instr.frB].paired0;
   const double b1 = state->fpr[instr.frB].paired1;

   const bool vxsnan0 = is_signalling_nan(b0);
   const bool vxsnan1 = is_signalling_nan(b1);
   const bool vxsqrt0 = !vxsnan0 && std::signbit(b0) && !is_zero(b0);
   const bool vxsqrt1 = !vxsnan1 && std::signbit(b1) && !is_zero(b1);
   const bool zx0 = is_zero(b0);
   const bool zx1 = is_zero(b1);

   const uint32_t oldFPSCR = state->fpscr.value;
   state->fpscr.vxsnan |= vxsnan0 || vxsnan1;
   state->fpscr.vxsqrt |= vxsqrt0 || vxsqrt1;
   state->fpscr.zx |= zx0 || zx1;

   double d0, d1;
   bool write = true;
   if (((vxsnan0 || vxsqrt0) && state->fpscr.ve) || (zx0 && state->fpscr.ze)) {
      write = false;
   } else {
      d0 = ppc_estimate_reciprocal_root(b0);
      updateFPRF(state, d0);
   }
   if (((vxsnan1 || vxsqrt1) && state->fpscr.ve) || (zx1 && state->fpscr.ze)) {
      write = false;
   } else {
      d1 = ppc_estimate_reciprocal_root(b1);
   }

   if (write) {
      // ps_rsqrte behaves strangely when the result's magnitude is out of
      // range: ps0 keeps its double-precision exponent, while ps1 appears
      // to get an arbitrary value from the floating-point circuitry.  The
      // details of how ps1's exponent is affected are unknown, but the
      // logic below works for double-precision inputs 0x7FE...FFF (maximum
      // normal) and 0x000...001 (minimum denormal).

      auto bits0 = get_float_bits(d0);
      bits0.mantissa &= UINT64_C(0xFFFFFE0000000);
      state->fpr[instr.frD].paired0 = bits0.v;

      auto bits1 = get_float_bits(d1);
      if (bits1.exponent == 0) {
         // Leave as zero (reciprocal square root can never be a denormal).
      } else if (bits1.exponent < 1151) {
         int8_t exponent8 = (bits1.exponent - 1023) & 0xFF;
         bits1.exponent = 1023 + exponent8;
      } else if (bits1.exponent < 2047) {
         bits1.exponent = 1022;
      }
      bits1.mantissa &= UINT64_C(0xFFFFFE0000000);
      state->fpr[instr.frD].paired1 = bits1.v;
   }

   updateFPSCR(state, oldFPSCR);
   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Select
static void
ps_sel(cpu::Core *state, Instruction instr)
{
   auto a0 = state->fpr[instr.frA].paired0;
   auto a1 = state->fpr[instr.frA].paired1;

   auto b0 = state->fpr[instr.frB].paired0;
   auto b1 = state->fpr[instr.frB].paired1;

   auto c0 = state->fpr[instr.frC].paired0;
   auto c1 = state->fpr[instr.frC].paired1;

   auto d1 = (a1 >= 0) ? c1 : b1;
   auto d0 = (a0 >= 0) ? c0 : b0;

   state->fpr[instr.frD].paired0 = d0;
   state->fpr[instr.frD].paired1 = d1;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

void
cpu::interpreter::registerPairedInstructions()
{
   RegisterInstruction(ps_add);
   RegisterInstruction(ps_div);
   RegisterInstruction(ps_mul);
   RegisterInstruction(ps_sub);
   RegisterInstruction(ps_abs);
   RegisterInstruction(ps_nabs);
   RegisterInstruction(ps_neg);
   RegisterInstruction(ps_sel);
   RegisterInstruction(ps_res);
   RegisterInstruction(ps_rsqrte);
   RegisterInstruction(ps_msub);
   RegisterInstruction(ps_madd);
   RegisterInstruction(ps_nmsub);
   RegisterInstruction(ps_nmadd);
   RegisterInstruction(ps_mr);
   RegisterInstruction(ps_sum0);
   RegisterInstruction(ps_sum1);
   RegisterInstruction(ps_muls0);
   RegisterInstruction(ps_muls1);
   RegisterInstruction(ps_madds0);
   RegisterInstruction(ps_madds1);
   RegisterInstruction(ps_merge00);
   RegisterInstruction(ps_merge01);
   RegisterInstruction(ps_merge10);
   RegisterInstruction(ps_merge11);
}
