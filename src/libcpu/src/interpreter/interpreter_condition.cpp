#include <utility>
#include <cfenv>
#include "interpreter_float.h"
#include "interpreter_insreg.h"
#include <common/bitutils.h>
#include <common/floatutils.h>

using espresso::ConditionRegisterFlag;
using espresso::FpscrFlags;

// TODO: Move these getCRX functions

static std::pair<uint32_t, uint32_t>
getCRFRange(uint32_t field)
{
   auto me = ((8 - field) * 4) - 1;
   auto mb = me - 3;
   return { mb, me };
}

uint32_t
getCRF(cpu::Core *state, uint32_t field)
{
   auto bits = getCRFRange(field);
   return (state->cr.value >> bits.first) & 0xf;
}

void
setCRF(cpu::Core *state, uint32_t field, uint32_t value)
{
   auto cr = state->cr.value;
   auto bits = getCRFRange(field);
   auto mask = make_bitmask<uint32_t>(bits.first, bits.second);

   cr = (cr & ~mask) | (value << bits.first);
   state->cr.value = cr;
}

uint32_t
getCRB(cpu::Core *state, uint32_t bit)
{
   return get_bit(state->cr.value, 31 - bit);
}

void
setCRB(cpu::Core *state, uint32_t bit, uint32_t value)
{
   state->cr.value = set_bit_value(state->cr.value, 31 - bit, value);
}

// Compare
enum CmpFlags
{
   CmpImmediate = 1 << 0, // b = imm
};

template<typename Type, unsigned flags = 0>
static void
cmpGeneric(cpu::Core *state, Instruction instr)
{
   Type a, b;
   uint32_t c;

   a = state->gpr[instr.rA];

   if (flags & CmpImmediate) {
      if (std::is_signed<Type>::value) {
         b = sign_extend<16>(instr.simm);
      } else {
         b = instr.uimm;
      }
   } else {
      b = state->gpr[instr.rB];
   }

   if (a < b) {
      c = ConditionRegisterFlag::LessThan;
   } else if (a > b) {
      c = ConditionRegisterFlag::GreaterThan;
   } else {
      c = ConditionRegisterFlag::Equal;
   }

   if (state->xer.so) {
      c |= ConditionRegisterFlag::SummaryOverflow;
   }

   setCRF(state, instr.crfD, c);
}

static void
cmp(cpu::Core *state, Instruction instr)
{
   return cmpGeneric<int32_t>(state, instr);
}

static void
cmpi(cpu::Core *state, Instruction instr)
{
   return cmpGeneric<int32_t, CmpImmediate>(state, instr);
}

static void
cmpl(cpu::Core *state, Instruction instr)
{
   return cmpGeneric<uint32_t>(state, instr);
}

static void
cmpli(cpu::Core *state, Instruction instr)
{
   return cmpGeneric<uint32_t, CmpImmediate>(state, instr);
}

// Floating Compare
enum FCmpFlags
{
   FCmpOrdered    = 1 << 0,
   FCmpUnordered  = 1 << 1,
   FCmpPS1        = 1 << 2,
};

template<unsigned flags>
static void
fcmpGeneric(cpu::Core *state, Instruction instr)
{
   double a, b;
   uint32_t c;

   if (flags & FCmpPS1) {
      a = state->fpr[instr.frA].paired1;
      b = state->fpr[instr.frB].paired1;
   } else {
      a = state->fpr[instr.frA].value;
      b = state->fpr[instr.frB].value;
   }

   const uint32_t oldFPSCR = state->fpscr.value;

   if (is_nan(a) || is_nan(b)) {
      c = ConditionRegisterFlag::Unordered;
      const bool vxsnan = is_signalling_nan(a) || is_signalling_nan(b);
      state->fpscr.vxsnan |= vxsnan;
      if ((flags & FCmpOrdered) && !(vxsnan && state->fpscr.ve)) {
         state->fpscr.vxvc = 1;
      }
   } else if (a < b) {
      c = ConditionRegisterFlag::LessThan;
   } else if (a > b) {
      c = ConditionRegisterFlag::GreaterThan;
   } else {  // a == b
      c = ConditionRegisterFlag::Equal;
   }

   setCRF(state, instr.crfD, c);
   state->fpscr.fpcc = c;
   updateFX_FEX_VX(state, oldFPSCR);
}

static void
fcmpo(cpu::Core *state, Instruction instr)
{
   return fcmpGeneric<FCmpOrdered>(state, instr);
}

static void
fcmpu(cpu::Core *state, Instruction instr)
{
   return fcmpGeneric<FCmpUnordered>(state, instr);
}

static void
ps_cmpo0(cpu::Core *state, Instruction instr)
{
   return fcmpGeneric<FCmpOrdered>(state, instr);
}

static void
ps_cmpo1(cpu::Core *state, Instruction instr)
{
   return fcmpGeneric<FCmpOrdered | FCmpPS1>(state, instr);
}

static void
ps_cmpu0(cpu::Core *state, Instruction instr)
{
   return fcmpGeneric<FCmpUnordered>(state, instr);
}

static void
ps_cmpu1(cpu::Core *state, Instruction instr)
{
   return fcmpGeneric<FCmpUnordered | FCmpPS1>(state, instr);
}

// Condition Register AND
static void
crand(cpu::Core *state, Instruction instr)
{
   uint32_t a, b, d;
   a = getCRB(state, instr.crbA);
   b = getCRB(state, instr.crbB);

   d = a & b;
   setCRB(state, instr.crbD, d);
}

// Condition Register AND with Complement
static void
crandc(cpu::Core *state, Instruction instr)
{
   uint32_t a, b, d;
   a = getCRB(state, instr.crbA);
   b = getCRB(state, instr.crbB);

   d = a & ~b;
   setCRB(state, instr.crbD, d);
}

// Condition Register Equivalent
static void
creqv(cpu::Core *state, Instruction instr)
{
   uint32_t a, b, d;
   a = getCRB(state, instr.crbA);
   b = getCRB(state, instr.crbB);

   d = ~(a ^ b);
   setCRB(state, instr.crbD, d);
}

// Condition Register NAND
static void
crnand(cpu::Core *state, Instruction instr)
{
   uint32_t a, b, d;
   a = getCRB(state, instr.crbA);
   b = getCRB(state, instr.crbB);

   d = ~(a & b);
   setCRB(state, instr.crbD, d);
}

// Condition Register NOR
static void
crnor(cpu::Core *state, Instruction instr)
{
   uint32_t a, b, d;
   a = getCRB(state, instr.crbA);
   b = getCRB(state, instr.crbB);

   d = ~(a | b);
   setCRB(state, instr.crbD, d);
}

// Condition Register OR
static void
cror(cpu::Core *state, Instruction instr)
{
   uint32_t a, b, d;
   a = getCRB(state, instr.crbA);
   b = getCRB(state, instr.crbB);

   d = a | b;
   setCRB(state, instr.crbD, d);
}

// Condition Register OR with Complement
static void
crorc(cpu::Core *state, Instruction instr)
{
   uint32_t a, b, d;
   a = getCRB(state, instr.crbA);
   b = getCRB(state, instr.crbB);

   d = a | ~b;
   setCRB(state, instr.crbD, d);
}

// Condition Register XOR
static void
crxor(cpu::Core *state, Instruction instr)
{
   uint32_t a, b, d;
   a = getCRB(state, instr.crbA);
   b = getCRB(state, instr.crbB);

   d = a ^ b;
   setCRB(state, instr.crbD, d);
}

// Move Condition Register Field
static void
mcrf(cpu::Core *state, Instruction instr)
{
   setCRF(state, instr.crfD, getCRF(state, instr.crfS));
}

// Move to Condition Register from FPSCR
static void
mcrfs(cpu::Core *state, Instruction instr)
{
   const int shiftS = 4 * (7 - instr.crfS);

   const uint32_t fpscrBits = (state->fpscr.value >> shiftS) & 0xF;
   setCRF(state, instr.crfD, fpscrBits);

   // All exception bits copied are cleared; other bits are left alone.
   // FEX and VX are updated following the normal rules.
   const uint32_t exceptionBits = FpscrFlags::FX | FpscrFlags::AllExceptions;
   const uint32_t bitsToClear = exceptionBits & (0xF << shiftS);
   state->fpscr.value &= ~bitsToClear;
   updateFEX_VX(state);
}

// Move to Condition Register from XER
static void
mcrxr(cpu::Core *state, Instruction instr)
{
   setCRF(state, instr.crfD, state->xer.crxr);
   state->xer.crxr = 0;
}

// Move from Condition Register
static void
mfcr(cpu::Core *state, Instruction instr)
{
   state->gpr[instr.rD] = state->cr.value;
}

// Move to Condition Register Fields
static void
mtcrf(cpu::Core *state, Instruction instr)
{
   uint32_t cr, crm, s, mask;

   s = state->gpr[instr.rS];
   cr = state->cr.value;
   crm = instr.crm;
   mask = 0;

   for (auto i = 0u; i < 8; ++i) {
      if (crm & (1 << i)) {
         mask |= 0xf << (i * 4);
      }
   }

   cr = (s & mask) | (cr & ~mask);
   state->cr.value = cr;
}

void
cpu::interpreter::registerConditionInstructions()
{
   RegisterInstruction(cmp);
   RegisterInstruction(cmpi);
   RegisterInstruction(cmpl);
   RegisterInstruction(cmpli);
   RegisterInstruction(fcmpo);
   RegisterInstruction(fcmpu);
   RegisterInstruction(crand);
   RegisterInstruction(crandc);
   RegisterInstruction(creqv);
   RegisterInstruction(crnand);
   RegisterInstruction(crnor);
   RegisterInstruction(cror);
   RegisterInstruction(crorc);
   RegisterInstruction(crxor);
   RegisterInstruction(mcrf);
   RegisterInstruction(mcrfs);
   RegisterInstruction(mcrxr);
   RegisterInstruction(mfcr);
   RegisterInstruction(mtcrf);
   RegisterInstruction(ps_cmpu0);
   RegisterInstruction(ps_cmpo0);
   RegisterInstruction(ps_cmpu1);
   RegisterInstruction(ps_cmpo1);
}
