#include <type_traits>
#include "interpreter_insreg.h"
#include <common/bitutils.h>

using espresso::ConditionRegisterFlag;

// Update cr0 with value
static void
updateConditionRegister(cpu::Core *state, uint32_t value)
{
   auto flags = 0;

   if (value == 0) {
      flags |= ConditionRegisterFlag::Zero;
   } else if (get_bit<31>(value)) {
      flags |= ConditionRegisterFlag::Negative;
   } else {
      flags |= ConditionRegisterFlag::Positive;
   }

   if (state->xer.so) {
      flags |= ConditionRegisterFlag::SummaryOverflow;
   }

   state->cr.cr0 = flags;
}

// Update carry flags
static void
updateCarry(cpu::Core *state, bool carry)
{
   state->xer.ca = carry;
}

// Update overflow flags
static void
updateOverflow(cpu::Core *state, bool overflow)
{
   state->xer.ov = overflow;
   state->xer.so |= overflow;
}

// Add
enum AddFlags
{
   AddCarry          = 1 << 0, // xer[ca] = carry
   AddExtended       = 1 << 1, // d = a + b + xer[ca]
   AddImmediate      = 1 << 2, // d = a + simm
   AddCheckRecord    = 1 << 3, // Check rc and oe then update cr0 and xer
   AddAlwaysRecord   = 1 << 4, // Always update cr0 and xer
   AddShifted        = 1 << 5, // d = a + (b << 16)
   AddToZero         = 1 << 6, // d = a + 0
   AddToMinusOne     = 1 << 7, // d = a + -1
   AddZeroRA         = 1 << 8, // a = (rA == 0) ? 0 : gpr[rA]
   AddSubtract       = 1 << 9, // a = ~a and +1 when not carry
};

template<unsigned flags = 0>
static void
addGeneric(cpu::Core *state, Instruction instr)
{
   uint32_t a, b, d;

   // Get a value
   if ((flags & AddZeroRA) && instr.rA == 0) {
      a = 0;
   } else {
      a = state->gpr[instr.rA];
   }

   if (flags & AddSubtract) {
      a = ~a;
   }

   // Get b value
   if (flags & AddImmediate) {
      b = sign_extend<16>(instr.simm);
   } else if (flags & AddToZero) {
      b = 0;
   } else if (flags & AddToMinusOne) {
      b = -1;
   } else {
      b = state->gpr[instr.rB];
   }

   if (flags & AddShifted) {
      b <<= 16;
   }

   // Calculate d
   d = a + b;

   // Add xer[ca] if needed
   if (flags & AddExtended) {
      d += state->xer.ca;
   } else if (flags & AddSubtract) {
      d += 1;
   }

   // Update rD
   state->gpr[instr.rD] = d;

   // Check for carry/overflow
   auto carry = d < a || (d == a && b > 0);
   auto overflow = !!get_bit<31>((a ^ d) & (b ^ d));

   if (flags & AddCarry) {
      updateCarry(state, carry);
   }

   if (flags & AddAlwaysRecord) {
      // Always record only means update CR0, NOT overflow
      updateConditionRegister(state, d);
   } else if (flags & AddCheckRecord) {
      if (instr.oe) {
         updateOverflow(state, overflow);
      }

      if (instr.rc) {
         updateConditionRegister(state, d);
      }
   }
}

static void
add(cpu::Core *state, Instruction instr)
{
   return addGeneric<AddCheckRecord>(state, instr);
}

static void
addc(cpu::Core *state, Instruction instr)
{
   return addGeneric<AddCarry | AddCheckRecord>(state, instr);
}

static void
adde(cpu::Core *state, Instruction instr)
{
   return addGeneric<AddExtended | AddCarry | AddCheckRecord>(state, instr);
}

static void
addi(cpu::Core *state, Instruction instr)
{
   return addGeneric<AddImmediate | AddZeroRA>(state, instr);
}

static void
addic(cpu::Core *state, Instruction instr)
{
   return addGeneric<AddImmediate | AddCarry>(state, instr);
}

static void
addicx(cpu::Core *state, Instruction instr)
{
   return addGeneric<AddImmediate | AddCarry | AddAlwaysRecord>(state, instr);
}

static void
addis(cpu::Core *state, Instruction instr)
{
   return addGeneric<AddImmediate | AddShifted | AddZeroRA>(state, instr);
}

static void
addme(cpu::Core *state, Instruction instr)
{
   return addGeneric<AddCheckRecord | AddCarry | AddExtended | AddToMinusOne>(state, instr);
}

static void
addze(cpu::Core *state, Instruction instr)
{
   return addGeneric<AddCheckRecord | AddCarry | AddExtended | AddToZero>(state, instr);
}

// AND
enum AndFlags
{
   AndComplement = 1 << 0, // b = ~b
   AndCheckRecord = 1 << 1, // Check rc then update cr
   AndImmediate = 1 << 2, // b = uimm
   AndShifted = 1 << 3, // b = (b << 16)
   AndAlwaysRecord = 1 << 4, // Always update cr
};

template<unsigned flags>
static void
andGeneric(cpu::Core *state, Instruction instr)
{
   uint32_t s, a, b;

   s = state->gpr[instr.rS];

   if (flags & AndImmediate) {
      b = instr.uimm;
   } else {
      b = state->gpr[instr.rB];
   }

   if (flags & AndShifted) {
      b <<= 16;
   }

   if (flags & AndComplement) {
      b = ~b;
   }

   a = s & b;
   state->gpr[instr.rA] = a;

   if (flags & AndAlwaysRecord) {
      updateConditionRegister(state, a);
   } else if (flags & AndCheckRecord) {
      if (instr.rc) {
         updateConditionRegister(state, a);
      }
   }
}

static void
and_(cpu::Core *state, Instruction instr)
{
   return andGeneric<AndCheckRecord>(state, instr);
}

static void
andc(cpu::Core *state, Instruction instr)
{
   return andGeneric<AndCheckRecord | AndComplement>(state, instr);
}

static void
andi(cpu::Core *state, Instruction instr)
{
   return andGeneric<AndAlwaysRecord | AndImmediate>(state, instr);
}

static void
andis(cpu::Core *state, Instruction instr)
{
   return andGeneric<AndAlwaysRecord | AndImmediate | AndShifted>(state, instr);
}

// Count Leading Zeroes Word
static void
cntlzw(cpu::Core *state, Instruction instr)
{
   unsigned long a;
   uint32_t s;

   s = state->gpr[instr.rS];

   if (!bit_scan_reverse(&a, s)) {
      a = 32;
   } else {
      a = 31 - a;
   }

   state->gpr[instr.rA] = a;

   if (instr.rc) {
      updateConditionRegister(state, a);
   }
}

// Divide
template<typename Type>
static void
divGeneric(cpu::Core *state, Instruction instr)
{
   Type a, b, d;
   a = state->gpr[instr.rA];
   b = state->gpr[instr.rB];

   auto overflow = (b == 0);

   if (std::is_signed<Type>::value && (a == 0x80000000 && b == -1)) {
      overflow = true;
   }

   if (!overflow) {
      d = a / b;
   } else {
      // rD = -1 when rA is negative, 0 when rA is positive
      d = a < 0 ? -1 : 0;
   }

   state->gpr[instr.rD] = d;

   if (instr.oe) {
      updateOverflow(state, overflow);
   }

   if (instr.rc) {
      updateConditionRegister(state, d);
   }
}

static void
divw(cpu::Core *state, Instruction instr)
{
   return divGeneric<int32_t>(state, instr);
}

static void
divwu(cpu::Core *state, Instruction instr)
{
   return divGeneric<uint32_t>(state, instr);
}

// Equivalent
static void
eqv(cpu::Core *state, Instruction instr)
{
   uint32_t a, s, b;

   s = state->gpr[instr.rS];
   b = state->gpr[instr.rB];

   a = ~(s ^ b);
   state->gpr[instr.rA] = a;

   if (instr.rc) {
      updateConditionRegister(state, a);
   }
}

// Extend Sign Byte
static void
extsb(cpu::Core *state, Instruction instr)
{
   uint32_t a, s;

   s = state->gpr[instr.rS];

   a = sign_extend<8>(s);
   state->gpr[instr.rA] = a;

   if (instr.rc) {
      updateConditionRegister(state, a);
   }
}

// Extend Sign Half Word
static void
extsh(cpu::Core *state, Instruction instr)
{
   uint32_t a, s;

   s = state->gpr[instr.rS];

   a = sign_extend<16>(s);
   state->gpr[instr.rA] = a;

   if (instr.rc) {
      updateConditionRegister(state, a);
   }
}

// Multiply
enum MulFlags
{
   MulLow           = 1 << 0, // (uint32_t)d
   MulHigh          = 1 << 1, // d >> 32
   MulImmediate     = 1 << 2, // b = simm
   MulCheckOverflow = 1 << 3, // Check oe then update xer
   MulCheckRecord   = 1 << 4, // Check rc then update cr
};

// Signed multiply
template<unsigned flags>
static void
mulSignedGeneric(cpu::Core *state, Instruction instr)
{
   int64_t a, b;
   int32_t d;
   bool overflow;
   a = static_cast<int32_t>(state->gpr[instr.rA]);

   if (flags & MulImmediate) {
      b = sign_extend<16>(instr.simm);
   } else {
      b = static_cast<int32_t>(state->gpr[instr.rB]);
   }

   if (flags & MulLow) {
      const int64_t product = a * b;
      d = static_cast<int32_t>(product);
      if (flags & MulCheckOverflow) {
         overflow = (product < INT64_C(-0x80000000) || product > 0x7FFFFFFF);
      }
   } else if (flags & MulHigh) {
      d = (a * b) >> 32;
      // oe is ignored for mulhw* instructions.
   }

   state->gpr[instr.rD] = d;

   if (flags & MulCheckOverflow) {
      if (instr.oe) {
         updateOverflow(state, overflow);
      }
   }
   if (flags & MulCheckRecord) {
      if (instr.rc) {
         updateConditionRegister(state, d);
      }
   }
}

// Unsigned multiply
template<unsigned flags>
static void
mulUnsignedGeneric(cpu::Core *state, Instruction instr)
{
   uint64_t a, b;
   uint32_t d;
   a = state->gpr[instr.rA];
   b = state->gpr[instr.rB];

   if (flags & MulLow) {
      d = static_cast<uint32_t>(a * b);
   } else if (flags & MulHigh) {
      d = (a * b) >> 32;
   }

   state->gpr[instr.rD] = d;

   if (flags & MulCheckRecord) {
      if (instr.rc) {
         updateConditionRegister(state, d);
      }
   }
}

static void
mulhw(cpu::Core *state, Instruction instr)
{
   return mulSignedGeneric<MulHigh | MulCheckRecord>(state, instr);
}

static void
mulhwu(cpu::Core *state, Instruction instr)
{
   return mulUnsignedGeneric<MulHigh | MulCheckRecord>(state, instr);
}

static void
mulli(cpu::Core *state, Instruction instr)
{
   return mulSignedGeneric<MulImmediate | MulLow>(state, instr);
}

static void
mullw(cpu::Core *state, Instruction instr)
{
   return mulSignedGeneric<MulLow | MulCheckRecord | MulCheckOverflow>(state, instr);
}

// NAND
static void
nand(cpu::Core *state, Instruction instr)
{
   uint32_t a, s, b;

   s = state->gpr[instr.rS];
   b = state->gpr[instr.rB];

   a = ~(s & b);
   state->gpr[instr.rA] = a;

   if (instr.rc) {
      updateConditionRegister(state, a);
   }
}

// Negate
static void
neg(cpu::Core *state, Instruction instr)
{
   uint32_t a, d;

   a = state->gpr[instr.rA];

   d = (~a) + 1;
   state->gpr[instr.rD] = d;

   bool overflow = (a == 0x80000000);

   if (instr.oe) {
      updateOverflow(state, overflow);
   }

   if (instr.rc) {
      updateConditionRegister(state, d);
   }
}

// NOR
static void
nor(cpu::Core *state, Instruction instr)
{
   uint32_t a, s, b;

   s = state->gpr[instr.rS];
   b = state->gpr[instr.rB];

   a = ~(s | b);
   state->gpr[instr.rA] = a;

   if (instr.rc) {
      updateConditionRegister(state, a);
   }
}

// OR
enum OrFlags
{
   OrComplement = 1 << 0, // b = ~b
   OrCheckRecord = 1 << 1, // Check rc then update cr
   OrImmediate = 1 << 2, // b = uimm
   OrShifted = 1 << 3, // b = (b << 16)
   OrAlwaysRecord = 1 << 4, // Always update cr
};

template<unsigned flags>
static void
orGeneric(cpu::Core *state, Instruction instr)
{
   uint32_t s, a, b;

   s = state->gpr[instr.rS];

   if (flags & OrImmediate) {
      b = instr.uimm;
   } else {
      b = state->gpr[instr.rB];
   }

   if (flags & OrShifted) {
      b <<= 16;
   }

   if (flags & OrComplement) {
      b = ~b;
   }

   a = s | b;
   state->gpr[instr.rA] = a;

   if (flags & OrAlwaysRecord) {
      updateConditionRegister(state, a);
   } else if (flags & OrCheckRecord) {
      if (instr.rc) {
         updateConditionRegister(state, a);
      }
   }
}

static void
or_(cpu::Core *state, Instruction instr)
{
   return orGeneric<OrCheckRecord>(state, instr);
}

static void
orc(cpu::Core *state, Instruction instr)
{
   return orGeneric<OrCheckRecord | OrComplement>(state, instr);
}

static void
ori(cpu::Core *state, Instruction instr)
{
   return orGeneric<OrImmediate>(state, instr);
}

static void
oris(cpu::Core *state, Instruction instr)
{
   return orGeneric<OrImmediate | OrShifted>(state, instr);
}

// Rotate left word
enum RlwFlags
{
   RlwImmediate   = 1 << 0, // n = sh
   RlwAnd         = 1 << 1, // a = r & m
   RlwInsert      = 1 << 2  // a = (r & m) | (r & ~m)
};

template<unsigned flags>
static void
rlwGeneric(cpu::Core *state, Instruction instr)
{
   uint32_t s, n, r, m, a;

   s = state->gpr[instr.rS];
   a = state->gpr[instr.rA];

   if (flags & RlwImmediate) {
      n = instr.sh;
   } else {
      n = state->gpr[instr.rB] & 0x1f;
   }

   r = bit_rotate_left(s, n);
   m = make_ppc_bitmask(instr.mb, instr.me);

   if (flags & RlwAnd) {
      a = (r & m);
   } else if (flags & RlwInsert) {
      a = (r & m) | (a & ~m);
   }

   state->gpr[instr.rA] = a;

   if (instr.rc) {
      updateConditionRegister(state, a);
   }
}

// Rotate Left Word Immediate then Mask Insert
static void
rlwimi(cpu::Core *state, Instruction instr)
{
   return rlwGeneric<RlwImmediate | RlwInsert>(state, instr);
}

// Rotate Left Word Immediate then AND with Mask
static void
rlwinm(cpu::Core *state, Instruction instr)
{
   return rlwGeneric<RlwImmediate | RlwAnd>(state, instr);
}

// Rotate Left Word then AND with Mask
static void
rlwnm(cpu::Core *state, Instruction instr)
{
   return rlwGeneric<RlwAnd>(state, instr);
}

// Shift Logical
enum ShiftFlags
{
   ShiftLeft = 1 << 0,
   ShiftRight = 1 << 1,
   ShiftImmediate = 1 << 2,
};

template<unsigned flags>
static void
shiftLogical(cpu::Core *state, Instruction instr)
{
   uint32_t n, s, b, a;

   s = state->gpr[instr.rS];

   if (flags & ShiftImmediate) {
      b = instr.sh;
   } else {
      b = state->gpr[instr.rB];
   }

   n = b & 0x1f;

   if (b & 0x20) {
      a = 0;
   } else if (flags & ShiftLeft) {
      a = s << n;
   } else if (flags & ShiftRight) {
      a = s >> n;
   }

   state->gpr[instr.rA] = a;

   if (instr.rc) {
      updateConditionRegister(state, a);
   }
}

// Shift Left Word
static void
slw(cpu::Core *state, Instruction instr)
{
   shiftLogical<ShiftLeft>(state, instr);
}

// Shift Right Word
static void
srw(cpu::Core *state, Instruction instr)
{
   shiftLogical<ShiftRight>(state, instr);
}

// Shift Arithmetic
template<unsigned flags>
static void
shiftArithmetic(cpu::Core *state, Instruction instr)
{
   static_assert(flags & ShiftRight, "Shift Arithmetic only supports ShiftRight");
   int32_t s, a, n, b;

   s = state->gpr[instr.rS];

   if (flags & ShiftImmediate) {
      b = instr.sh;
   } else {
      b = state->gpr[instr.rB];
   }

   bool carry = false;
   if (b & 0x20) {
      if (s >= 0) {
         a = 0;
      } else {
         a = -1;
         carry = true;
      }
   } else {
      n = b & 0x1f;
      if (n == 0) {
         a = s;
      } else {
         a = s >> n;

         if ((s < 0) && (s << (32 - n))) {
            carry = true;
         }
      }
   }

   state->gpr[instr.rA] = a;

   updateCarry(state, carry);

   if (instr.rc) {
      updateConditionRegister(state, a);
   }
}

static void
sraw(cpu::Core *state, Instruction instr)
{
   shiftArithmetic<ShiftRight>(state, instr);
}

static void
srawi(cpu::Core *state, Instruction instr)
{
   shiftArithmetic<ShiftRight | ShiftImmediate>(state, instr);
}

// Because sub is rD = ~rA + rB + 1 we can reuse our generic add
static void
subf(cpu::Core *state, Instruction instr)
{
   addGeneric<AddSubtract | AddCheckRecord>(state, instr);
}

static void
subfc(cpu::Core *state, Instruction instr)
{
   addGeneric<AddCarry | AddSubtract | AddCheckRecord>(state, instr);
}

static void
subfe(cpu::Core *state, Instruction instr)
{
   addGeneric<AddExtended | AddCarry | AddSubtract | AddCheckRecord>(state, instr);
}

static void
subfic(cpu::Core *state, Instruction instr)
{
   addGeneric<AddImmediate | AddCarry | AddSubtract>(state, instr);
}

static void
subfme(cpu::Core *state, Instruction instr)
{
   addGeneric<AddToMinusOne | AddExtended | AddCarry | AddCheckRecord | AddSubtract>(state, instr);
}

static void
subfze(cpu::Core *state, Instruction instr)
{
   addGeneric<AddToZero | AddExtended | AddCarry | AddCheckRecord | AddSubtract>(state, instr);
}

// XOR
enum XorFlags
{
   XorCheckRecord = 1 << 1, // Check rc then update cr
   XorImmediate   = 1 << 2, // b = uimm
   XorShifted     = 1 << 3, // b = (b << 16)
};

template<unsigned flags>
static void
xorGeneric(cpu::Core *state, Instruction instr)
{
   uint32_t s, a, b;

   s = state->gpr[instr.rS];

   if (flags & XorImmediate) {
      b = instr.uimm;
   } else {
      b = state->gpr[instr.rB];
   }

   if (flags & XorShifted) {
      b <<= 16;
   }

   a = s ^ b;
   state->gpr[instr.rA] = a;

   if (flags & XorCheckRecord) {
      if (instr.rc) {
         updateConditionRegister(state, a);
      }
   }
}

static void
xor_(cpu::Core *state, Instruction instr)
{
   return xorGeneric<XorCheckRecord>(state, instr);
}

static void
xori(cpu::Core *state, Instruction instr)
{
   return xorGeneric<XorImmediate>(state, instr);
}

static void
xoris(cpu::Core *state, Instruction instr)
{
   return xorGeneric<XorImmediate | XorShifted>(state, instr);
}

void
cpu::interpreter::registerIntegerInstructions()
{
   RegisterInstruction(add);
   RegisterInstruction(addc);
   RegisterInstruction(adde);
   RegisterInstruction(addi);
   RegisterInstruction(addic);
   RegisterInstruction(addicx);
   RegisterInstruction(addis);
   RegisterInstruction(addme);
   RegisterInstruction(addze);
   RegisterInstruction(and_);
   RegisterInstruction(andc);
   RegisterInstruction(andi);
   RegisterInstruction(andis);
   RegisterInstruction(cntlzw);
   RegisterInstruction(divw);
   RegisterInstruction(divwu);
   RegisterInstruction(extsb);
   RegisterInstruction(extsh);
   RegisterInstruction(eqv);
   RegisterInstruction(mulhw);
   RegisterInstruction(mulhwu);
   RegisterInstruction(mulli);
   RegisterInstruction(mullw);
   RegisterInstruction(nand);
   RegisterInstruction(neg);
   RegisterInstruction(nor);
   RegisterInstruction(or_);
   RegisterInstruction(orc);
   RegisterInstruction(ori);
   RegisterInstruction(oris);
   RegisterInstruction(rlwimi);
   RegisterInstruction(rlwinm);
   RegisterInstruction(rlwnm);
   RegisterInstruction(srw);
   RegisterInstruction(slw);
   RegisterInstruction(sraw);
   RegisterInstruction(srawi);
   RegisterInstruction(subf);
   RegisterInstruction(subf);
   RegisterInstruction(subfc);
   RegisterInstruction(subfe);
   RegisterInstruction(subfic);
   RegisterInstruction(subfme);
   RegisterInstruction(subfze);
   RegisterInstruction(xor_);
   RegisterInstruction(xori);
   RegisterInstruction(xoris);
}
