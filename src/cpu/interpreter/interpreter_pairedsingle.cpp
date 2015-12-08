#include <cmath>
#include "interpreter_insreg.h"
#include "interpreter_float.h"
#include "utils/floatutils.h"

// Absolute
static void
ps_abs(ThreadState *state, Instruction instr)
{
   setFPRPairedSingle(state, instr.frB);

   float b0, b1, d0, d1;
   b0 = state->fpr[instr.frB].paired0;
   b1 = state->fpr[instr.frB].paired1;

   d1 = std::fabs(b1);
   d0 = std::fabs(b0);

   state->fpr[instr.frD].paired0 = d0;
   state->fpr[instr.frD].paired1 = d1;
   state->fpr_ps[instr.frD] = true;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Add
static void
ps_add(ThreadState *state, Instruction instr)
{
   setFPRPairedSingle(state, instr.frA);
   setFPRPairedSingle(state, instr.frB);

   float a0, a1, b0, b1, d0, d1;

   a0 = state->fpr[instr.frA].paired0;
   a1 = state->fpr[instr.frA].paired1;

   b0 = state->fpr[instr.frB].paired0;
   b1 = state->fpr[instr.frB].paired1;

   const uint32_t oldFPSCR = state->fpscr.value;

   state->fpscr.vxisi |=
         (is_infinity(a0) && is_infinity(b0))
      || (is_infinity(a1) && is_infinity(b1));

   state->fpscr.vxsnan |=
         is_signalling_nan(a0) || is_signalling_nan(a1)
      || is_signalling_nan(b0) || is_signalling_nan(b1);

   d1 = a1 + b1;
   d0 = a0 + b0;
   updateFPSCR(state, oldFPSCR);
   updateFPRF(state, d0);

   state->fpr[instr.frD].paired0 = d0;
   state->fpr[instr.frD].paired1 = d1;
   state->fpr_ps[instr.frD] = true;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Divide
static void
ps_div(ThreadState *state, Instruction instr)
{
   setFPRPairedSingle(state, instr.frA);
   setFPRPairedSingle(state, instr.frB);

   float a0, a1, b0, b1, d0, d1;

   a0 = state->fpr[instr.frA].paired0;
   a1 = state->fpr[instr.frA].paired1;

   b0 = state->fpr[instr.frB].paired0;
   b1 = state->fpr[instr.frB].paired1;

   const uint32_t oldFPSCR = state->fpscr.value;

   state->fpscr.vxidi |=
         (is_infinity(a0) && is_infinity(b0))
      || (is_infinity(a1) && is_infinity(b1));

   state->fpscr.vxzdz |=
         (is_zero(a0) && is_zero(b0))
      || (is_zero(a1) && is_zero(b1));

   state->fpscr.vxsnan |=
         is_signalling_nan(a0) || is_signalling_nan(a1)
      || is_signalling_nan(b0) || is_signalling_nan(b1);

   d1 = a1 / b1;
   d0 = a0 / b0;
   updateFPSCR(state, oldFPSCR);
   updateFPRF(state, d0);

   state->fpr[instr.frD].paired0 = d0;
   state->fpr[instr.frD].paired1 = d1;
   state->fpr_ps[instr.frD] = true;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
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
   setFPRPairedSingle(state, instr.frA);
   setFPRPairedSingle(state, instr.frB);
   setFPRPairedSingle(state, instr.frC);

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
   state->fpr_ps[instr.frD] = true;

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
   setFPRPairedSingle(state, instr.frA);
   setFPRPairedSingle(state, instr.frB);

   float d0, d1;

   if (flags & MergeValue0) {
      d0 = state->fpr[instr.frA].paired1;
   } else {
      d0 = state->fpr[instr.frA].paired0;
   }

   if (flags & MergeValue1) {
      d1 = state->fpr[instr.frB].paired1;
   } else {
      d1 = state->fpr[instr.frB].paired0;
   }

   state->fpr[instr.frD].paired0 = d0;
   state->fpr[instr.frD].paired1 = d1;
   state->fpr_ps[instr.frD] = true;

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

// Move Register
static void
ps_mr(ThreadState *state, Instruction instr)
{
   setFPRPairedSingle(state, instr.frB);

   state->fpr[instr.frD].paired0 = state->fpr[instr.frB].paired0;
   state->fpr[instr.frD].paired1 = state->fpr[instr.frB].paired1;
   state->fpr_ps[instr.frD] = true;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
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
   setFPRPairedSingle(state, instr.frA);
   setFPRPairedSingle(state, instr.frB);
   setFPRPairedSingle(state, instr.frC);

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
   state->fpr_ps[instr.frD] = true;

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

// Multiply
enum MultiplyFlags
{
   MultiplyPaired    = 1 << 0,
   MultiplyScalar0   = 1 << 1,
   MultiplyScalar1   = 1 << 2
};

template<unsigned flags>
static void
mulGeneric(ThreadState *state, Instruction instr)
{
   setFPRPairedSingle(state, instr.frA);
   setFPRPairedSingle(state, instr.frC);

   float a0, a1, c0, c1, d0, d1;

   a0 = state->fpr[instr.frA].paired0;
   a1 = state->fpr[instr.frA].paired1;

   if (flags & MultiplyPaired) {
      c0 = state->fpr[instr.frC].paired0;
      c1 = state->fpr[instr.frC].paired1;
   } else if (flags & MultiplyScalar0) {
      c0 = state->fpr[instr.frC].paired0;
      c1 = state->fpr[instr.frC].paired0;
   } else if (flags & MultiplyScalar1) {
      c0 = state->fpr[instr.frC].paired1;
      c1 = state->fpr[instr.frC].paired1;
   }

   const uint32_t oldFPSCR = state->fpscr.value;

   state->fpscr.vximz |=
         (is_infinity(a0) && is_zero(c0))
      || (is_infinity(a1) && is_zero(c1))
      || (is_zero(a0) && is_infinity(c0))
      || (is_zero(a1) && is_infinity(c1));

   state->fpscr.vxsnan |=
         is_signalling_nan(a0) || is_signalling_nan(a1)
      || is_signalling_nan(c0) || is_signalling_nan(c1);

   d1 = a1 * c1;
   d0 = a0 * c0;
   updateFPSCR(state, oldFPSCR);
   updateFPRF(state, d0);

   state->fpr[instr.frD].paired0 = d0;
   state->fpr[instr.frD].paired1 = d1;
   state->fpr_ps[instr.frD] = true;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

static void
ps_mul(ThreadState *state, Instruction instr)
{
   return mulGeneric<MultiplyPaired>(state, instr);
}

static void
ps_muls0(ThreadState *state, Instruction instr)
{
   return mulGeneric<MultiplyScalar0>(state, instr);
}

static void
ps_muls1(ThreadState *state, Instruction instr)
{
   return mulGeneric<MultiplyScalar1>(state, instr);
}

// Negative Absolute
static void
ps_nabs(ThreadState *state, Instruction instr)
{
   setFPRPairedSingle(state, instr.frB);

   float b0, b1, d0, d1;

   b0 = state->fpr[instr.frB].paired0;
   b1 = state->fpr[instr.frB].paired1;

   d1 = -std::fabs(b1);
   d0 = -std::fabs(b0);

   state->fpr[instr.frD].paired0 = d0;
   state->fpr[instr.frD].paired1 = d1;
   state->fpr_ps[instr.frD] = true;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Negate
static void
ps_neg(ThreadState *state, Instruction instr)
{
   setFPRPairedSingle(state, instr.frB);

   float b0, b1, d0, d1;

   b0 = state->fpr[instr.frB].paired0;
   b1 = state->fpr[instr.frB].paired1;

   d1 = -b1;
   d0 = -b0;

   state->fpr[instr.frD].paired0 = d0;
   state->fpr[instr.frD].paired1 = d1;
   state->fpr_ps[instr.frD] = true;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Reciprocal
static void
ps_res(ThreadState *state, Instruction instr)
{
   setFPRPairedSingle(state, instr.frB);

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
   state->fpr_ps[instr.frD] = true;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Reciprocal Square Root
static void
ps_rsqrte(ThreadState *state, Instruction instr)
{
   setFPRPairedSingle(state, instr.frB);

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
   state->fpr_ps[instr.frD] = true;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Select
static void
ps_sel(ThreadState *state, Instruction instr)
{
   setFPRPairedSingle(state, instr.frA);
   setFPRPairedSingle(state, instr.frB);
   setFPRPairedSingle(state, instr.frC);

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
   state->fpr_ps[instr.frD] = true;

   if (instr.rc) {
      updateFloatConditionRegister(state);
   }
}

// Subtract
static void
ps_sub(ThreadState *state, Instruction instr)
{
   setFPRPairedSingle(state, instr.frA);
   setFPRPairedSingle(state, instr.frB);

   float a0, a1, b0, b1, d0, d1;

   a0 = state->fpr[instr.frA].paired0;
   a1 = state->fpr[instr.frA].paired1;

   b0 = state->fpr[instr.frB].paired0;
   b1 = state->fpr[instr.frB].paired1;

   const uint32_t oldFPSCR = state->fpscr.value;

   state->fpscr.vxisi |=
         (is_infinity(a0) && is_infinity(b0))
      || (is_infinity(a1) && is_infinity(b1));

   state->fpscr.vxsnan |=
         is_signalling_nan(a0) || is_signalling_nan(a1)
      || is_signalling_nan(b0) || is_signalling_nan(b1);

   d1 = a1 - b1;
   d0 = a0 - b0;
   updateFPSCR(state, oldFPSCR);
   updateFPRF(state, d0);

   state->fpr[instr.frD].paired0 = d0;
   state->fpr[instr.frD].paired1 = d1;
   state->fpr_ps[instr.frD] = true;

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
   setFPRPairedSingle(state, instr.frA);
   setFPRPairedSingle(state, instr.frC);
   setFPRPairedSingle(state, instr.frB);

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
   state->fpr_ps[instr.frD] = true;

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
