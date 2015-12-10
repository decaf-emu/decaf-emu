#include <cmath>
#include "interpreter_insreg.h"
#include "interpreter_float.h"
#include "utils/floatutils.h"

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
moveGeneric(ThreadState *state, Instruction instr)
{
   uint32_t b0, b1, d0, d1;
   const bool ps0_nan = is_signalling_nan(state->fpr[instr.frB].paired0);
   if (!ps0_nan) {
      // We have to round this if it has excess precision, so we can't just
      // chop off the trailing bits.
      b0 = bit_cast<uint32_t>(static_cast<float>(state->fpr[instr.frB].paired0));
   } else {
      b0 = truncate_double_bits(state->fpr[instr.frB].idw);
   }
   b1 = state->fpr[instr.frB].iw_paired1;

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
   state->fpr[instr.frD].iw_paired1 = d1;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Move Register
static void
ps_mr(ThreadState *state, Instruction instr)
{
   return moveGeneric<MoveDirect>(state, instr);
}

// Negate
static void
ps_neg(ThreadState *state, Instruction instr)
{
   return moveGeneric<MoveNegate>(state, instr);
}

// Absolute
static void
ps_abs(ThreadState *state, Instruction instr)
{
   return moveGeneric<MoveAbsolute>(state, instr);
}

// Negative Absolute
static void
ps_nabs(ThreadState *state, Instruction instr)
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
psArithSingle(ThreadState *state, Instruction instr, float *result)
{
   double a, b;
   if (slotA == 0) {
      a = state->fpr[instr.frA].paired0;
   } else {
      a = extend_float(state->fpr[instr.frA].paired1);
   }
   if (slotB == 0) {
      b = state->fpr[op == PSMul ? instr.frC : instr.frB].paired0;
   } else {
      b = extend_float(state->fpr[op == PSMul ? instr.frC : instr.frB].paired1);
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
         d = static_cast<float>(a * b);
         break;
      case PSDiv:
         d = static_cast<float>(a / b);
         break;
      }
   }

   *result = d;
   return true;
}

template<PSArithOperator op, int slotB0, int slotB1>
static void
psArithGeneric(ThreadState *state, Instruction instr)
{
   const uint32_t oldFPSCR = state->fpscr.value;

   float d0, d1;
   const bool wrote0 = psArithSingle<op, 0, slotB0>(state, instr, &d0);
   const bool wrote1 = psArithSingle<op, 1, slotB1>(state, instr, &d1);
   if (wrote0 && wrote1) {
      state->fpr[instr.frD].paired0 = extend_float(d0);
      state->fpr[instr.frD].paired1 = d1;
   }

   if (wrote0) {
      updateFPRF(state, extend_float(d0));
   }
   updateFPSCR(state, oldFPSCR);

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Add
static void
ps_add(ThreadState *state, Instruction instr)
{
   return psArithGeneric<PSAdd, 0, 1>(state, instr);
}

// Subtract
static void
ps_sub(ThreadState *state, Instruction instr)
{
   return psArithGeneric<PSSub, 0, 1>(state, instr);
}

// Multiply
static void
ps_mul(ThreadState *state, Instruction instr)
{
   return psArithGeneric<PSMul, 0, 1>(state, instr);
}

static void
ps_muls0(ThreadState *state, Instruction instr)
{
   return psArithGeneric<PSMul, 0, 0>(state, instr);
}

static void
ps_muls1(ThreadState *state, Instruction instr)
{
   return psArithGeneric<PSMul, 1, 1>(state, instr);
}

// Divide
static void
ps_div(ThreadState *state, Instruction instr)
{
   return psArithGeneric<PSDiv, 0, 1>(state, instr);
}

// Multiply and Add
enum MaddFlags
{
   MaddPaired     = 1 << 0,
   MaddScalar1    = 1 << 1,
   MaddScalar0    = 1 << 2,
   MaddNegate     = 1 << 3
};

template<unsigned flags>
static void
maddGeneric(ThreadState *state, Instruction instr)
{
   float a0, a1, b0, b1, c0, c1, d0, d1;

   a0 = state->fpr[instr.frA].paired0;
   a1 = state->fpr[instr.frA].paired1;

   b0 = state->fpr[instr.frB].paired0;
   b1 = state->fpr[instr.frB].paired1;

   if (flags & MaddPaired) {
      c0 = state->fpr[instr.frC].paired0;
      c1 = state->fpr[instr.frC].paired1;
   } else if (flags & MaddScalar0) {
      c0 = state->fpr[instr.frC].paired0;
      c1 = state->fpr[instr.frC].paired0;
   } else if (flags & MaddScalar1) {
      c0 = state->fpr[instr.frC].paired1;
      c1 = state->fpr[instr.frC].paired1;
   }

   const uint32_t oldFPSCR = state->fpscr.value;

   state->fpscr.vxisi |=
         (is_infinity(a0 * c0) && is_infinity(b0))
      || (is_infinity(a1 * c1) && is_infinity(b1));

   state->fpscr.vximz |=
         (is_infinity(a0) && is_zero(c0))
      || (is_infinity(a1) && is_zero(c1))
      || (is_zero(a0) && is_infinity(c0))
      || (is_zero(a1) && is_infinity(c1));

   state->fpscr.vxsnan |=
         is_signalling_nan(a0) || is_signalling_nan(a1)
      || is_signalling_nan(b0) || is_signalling_nan(b1)
      || is_signalling_nan(c0) || is_signalling_nan(c1);

   if (flags & MaddNegate) {
      d1 = -((a1 * c1) + b1);
      d0 = -((a0 * c0) + b0);
   } else {
      d1 = (a1 * c1) + b1;
      d0 = (a0 * c0) + b0;
   }

   updateFPSCR(state, oldFPSCR);
   updateFPRF(state, d0);

   state->fpr[instr.frD].paired0 = d0;
   state->fpr[instr.frD].paired1 = d1;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

static void
ps_madd(ThreadState *state, Instruction instr)
{
   return maddGeneric<MaddPaired>(state, instr);
}

static void
ps_madds0(ThreadState *state, Instruction instr)
{
   return maddGeneric<MaddScalar0>(state, instr);
}

static void
ps_madds1(ThreadState *state, Instruction instr)
{
   return maddGeneric<MaddScalar1>(state, instr);
}

static void
ps_nmadd(ThreadState *state, Instruction instr)
{
   return maddGeneric<MaddPaired | MaddNegate>(state, instr);
}

// Merge registers
enum MergeFlags
{
   MergeValue0 = 1 << 0,
   MergeValue1 = 1 << 1
};

template<unsigned flags = 0>
static void
mergeGeneric(ThreadState *state, Instruction instr)
{
   float d0, d1;

   if (flags & MergeValue0) {
      d0 = state->fpr[instr.frA].paired1;
   } else {
      if (!is_signalling_nan(state->fpr[instr.frA].paired0)) {
         d0 = static_cast<float>(state->fpr[instr.frA].paired0);
      } else {
         d0 = truncate_double(state->fpr[instr.frA].paired0);
      }
   }

   if (flags & MergeValue1) {
      d1 = state->fpr[instr.frB].paired1;
   } else {
      // When inserting a double-precision value into slot 1, the mantissa
      // is truncated rather than rounded.
      d1 = truncate_double(state->fpr[instr.frB].paired0);
   }

   state->fpr[instr.frD].paired0 = extend_float(d0);
   state->fpr[instr.frD].paired1 = d1;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

static void
ps_merge00(ThreadState *state, Instruction instr)
{
   return mergeGeneric(state, instr);
}

static void
ps_merge01(ThreadState *state, Instruction instr)
{
   return mergeGeneric<MergeValue1>(state, instr);
}

static void
ps_merge11(ThreadState *state, Instruction instr)
{
   return mergeGeneric<MergeValue0 | MergeValue1>(state, instr);
}

static void
ps_merge10(ThreadState *state, Instruction instr)
{
   return mergeGeneric<MergeValue0>(state, instr);
}

// Multiply and sub
enum MsubFlags
{
   MsubNegate  = 1 << 0
};

template<unsigned flags = 0>
static void
msubGeneric(ThreadState *state, Instruction instr)
{
   float a0, a1, b0, b1, c0, c1, d0, d1;

   a0 = state->fpr[instr.frA].paired0;
   a1 = state->fpr[instr.frA].paired1;

   b0 = state->fpr[instr.frB].paired0;
   b1 = state->fpr[instr.frB].paired1;

   c0 = state->fpr[instr.frC].paired0;
   c1 = state->fpr[instr.frC].paired1;

   const uint32_t oldFPSCR = state->fpscr.value;

   state->fpscr.vxisi |=
         (is_infinity(a0 * c0) && is_infinity(b0))
      || (is_infinity(a1 * c1) && is_infinity(b1));

   state->fpscr.vximz |=
         (is_infinity(a0) && is_zero(c0))
      || (is_infinity(a1) && is_zero(c1))
      || (is_zero(a0) && is_infinity(c0))
      || (is_zero(a1) && is_infinity(c1));

   state->fpscr.vxsnan |=
         is_signalling_nan(a0) || is_signalling_nan(a1)
      || is_signalling_nan(b0) || is_signalling_nan(b1)
      || is_signalling_nan(c0) || is_signalling_nan(c1);

   if (flags & MsubNegate) {
      d1 = -((a1 * c1) - b1);
      d0 = -((a0 * c0) - b0);
   } else {
      d1 = (a1 * c1) - b1;
      d0 = (a0 * c0) - b0;
   }

   updateFPSCR(state, oldFPSCR);
   updateFPRF(state, d0);

   state->fpr[instr.frD].paired0 = d0;
   state->fpr[instr.frD].paired1 = d1;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

static void
ps_msub(ThreadState *state, Instruction instr)
{
   return msubGeneric(state, instr);
}

static void
ps_nmsub(ThreadState *state, Instruction instr)
{
   return msubGeneric<MsubNegate>(state, instr);
}

// Reciprocal
static void
ps_res(ThreadState *state, Instruction instr)
{
   float b0, b1, d0, d1;

   b0 = state->fpr[instr.frB].paired0;
   b1 = state->fpr[instr.frB].paired1;

   const uint32_t oldFPSCR = state->fpscr.value;

   state->fpscr.vxsnan |=
      is_signalling_nan(b0) || is_signalling_nan(b1);

   state->fpscr.zx |=
      is_zero(b0) || is_zero(b1);

   d1 = 1.0f / b1;
   d0 = 1.0f / b0;
   updateFPSCR(state, oldFPSCR);
   updateFPRF(state, d0);

   state->fpr[instr.frD].paired0 = d0;
   state->fpr[instr.frD].paired1 = d1;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Reciprocal Square Root
static void
ps_rsqrte(ThreadState *state, Instruction instr)
{
   float b0, b1, d0, d1;

   b0 = state->fpr[instr.frB].paired0;
   b1 = state->fpr[instr.frB].paired1;

   const uint32_t oldFPSCR = state->fpscr.value;

   state->fpscr.vxsnan |=
      is_signalling_nan(b0) || is_signalling_nan(b1);

   state->fpscr.vxsqrt |=
      is_negative(b0) || is_negative(b1);

   d1 = 1.0f / std::sqrt(b1);
   d0 = 1.0f / std::sqrt(b0);
   updateFPSCR(state, oldFPSCR);
   updateFPRF(state, d0);

   state->fpr[instr.frD].paired0 = d0;
   state->fpr[instr.frD].paired1 = d1;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Select
static void
ps_sel(ThreadState *state, Instruction instr)
{
   float a0, a1, b0, b1, c0, c1, d0, d1;

   a0 = state->fpr[instr.frA].paired0;
   a1 = state->fpr[instr.frA].paired1;

   b0 = state->fpr[instr.frB].paired0;
   b1 = state->fpr[instr.frB].paired1;

   c0 = state->fpr[instr.frC].paired0;
   c1 = state->fpr[instr.frC].paired1;

   d1 = (a1 >= 0) ? c1 : b1;
   d0 = (a0 >= 0) ? c0 : b0;

   state->fpr[instr.frD].paired0 = d0;
   state->fpr[instr.frD].paired1 = d1;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Sum
enum SumFlags
{
   Sum0  = 1 << 0,
   Sum1  = 1 << 1
};

template<unsigned flags = 0>
static void
sumGeneric(ThreadState *state, Instruction instr)
{
   float a0, b0, b1, c0, c1, d0, d1;

   a0 = state->fpr[instr.frA].paired0;

   b0 = state->fpr[instr.frB].paired0;
   b1 = state->fpr[instr.frB].paired1;

   c0 = state->fpr[instr.frC].paired0;
   c1 = state->fpr[instr.frC].paired1;

   const uint32_t oldFPSCR = state->fpscr.value;

   state->fpscr.vxisi |=
      (is_infinity(a0) && is_infinity(b1));

   state->fpscr.vxsnan |=
         is_signalling_nan(a0)
      || is_signalling_nan(b1)
      || is_signalling_nan(c0)
      || is_signalling_nan(c1);

   if (flags & Sum0) {
      d0 = a0 + b1;
      d1 = c1;
   } else if (flags & Sum1) {
      d0 = c0;
      d1 = a0 + b1;
   }

   updateFPSCR(state, oldFPSCR);
   updateFPRF(state, d0);

   state->fpr[instr.frD].paired0 = d0;
   state->fpr[instr.frD].paired1 = d1;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

static void
ps_sum0(ThreadState *state, Instruction instr)
{
   return sumGeneric<Sum0>(state, instr);
}

static void
ps_sum1(ThreadState *state, Instruction instr)
{
   return sumGeneric<Sum1>(state, instr);
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
