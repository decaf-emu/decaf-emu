#include "bitutils.h"
#include "jit.h"

// Update cr0 with value
static void
updateConditionRegister(PPCEmuAssembler& a, const asmjit::X86GpReg& value, const asmjit::X86GpReg& tmp, const asmjit::X86GpReg& tmp2)
{
   asmjit::Label bbNegative(a);
   asmjit::Label bbPositive(a);
   asmjit::Label bbEnd(a);
   asmjit::Label bbEnd2(a);

   a.mov(tmp, a.ppccr);
   a.and_(tmp, 0x0FFFFFFF);

   a.cmp(value, 0);
   a.jl(bbNegative);
   a.jg(bbPositive);
   
   a.or_(tmp, ConditionRegisterFlag::Zero << 28);
   a.jmp(bbEnd);

   a.bind(bbNegative);
   a.or_(tmp, ConditionRegisterFlag::Negative << 28);
   a.jmp(bbEnd);

   a.bind(bbPositive);
   a.or_(tmp, ConditionRegisterFlag::Positive << 28);

   a.bind(bbEnd);

   a.mov(tmp2, a.ppcxer);
   a.and_(tmp2, XERegisterBits::StickyOV);
   a.cmp(tmp2, 0);
   a.je(bbEnd2);
   a.or_(tmp, ConditionRegisterFlag::SummaryOverflow << 28);

   a.bind(bbEnd2);
   a.mov(a.ppccr, tmp);
}

// Add
enum AddFlags
{
   AddCarry = 1 << 0, // xer[ca] = carry
   AddExtended = 1 << 1, // d = a + b + xer[ca]
   AddImmediate = 1 << 2, // d = a + simm
   AddCheckRecord = 1 << 3, // Check rc and oe then update cr0 and xer
   AddAlwaysRecord = 1 << 4, // Always update cr0 and xer
   AddShifted = 1 << 5, // d = a + (b << 16)
   AddToZero = 1 << 6, // d = a + 0
   AddToMinusOne = 1 << 7, // d = a + -1
   AddZeroRA = 1 << 8, // a = (rA == 0) ? 0 : gpr[rA]
   AddSubtract = 1 << 9, // a = ~a and +1 when not carry
};

template<unsigned flags = 0>
static bool
addGeneric(PPCEmuAssembler& a, Instruction instr)
{
   bool recordCarry = false;
   bool recordOverflow = false;
   bool recordCond = false;
   if (flags & AddCarry) {
      recordCarry = true;
   }
   if (flags & AddAlwaysRecord) {
      recordOverflow = true;
      recordCond = true;
   } else if (flags & AddCheckRecord) {
      if (instr.oe) {
         recordOverflow = true;
      }
      if (instr.rc) {
         recordCond = true;
      }
   }

   if ((flags & AddZeroRA) && instr.rA == 0) {
      a.mov(a.eax, 0);
   } else {
      a.mov(a.eax, a.ppcgpr[instr.rA]);
   }

   if (flags & AddSubtract) {
      a.not_(a.eax);
   }

   if (flags & AddImmediate) {
      a.mov(a.ecx, sign_extend<16>(instr.simm));
   } else if (flags & AddToZero) {
      a.mov(a.ecx, 0);
   } else if (flags & AddToMinusOne) {
      a.mov(a.ecx, -1);
   } else {
      a.mov(a.ecx, a.ppcgpr[instr.rB]);
   }

   if (flags & AddShifted) {
      a.shl(a.ecx, 16);
   }

   // Mark x64 CF based on PPC CF
   if (flags & AddExtended) {
      a.mov(a.edx, a.ppcxer);
      a.and_(a.edx, XERegisterBits::Carry);
      a.add(a.edx, 0xffffffff);

      a.adc(a.eax, a.ecx);
   } else if (flags & AddSubtract) {
      a.stc();

      a.adc(a.eax, a.ecx);
   } else {
      a.add(a.eax, a.ecx);
   }

   if (recordCarry && recordOverflow) {
      a.mov(a.ecx, 0);
      a.setc(a.ecx.r8());
      a.mov(a.edx, 0);
      a.seto(a.edx.r8());

      a.shl(a.ecx, XERegisterBits::CarryShift);
      a.shl(a.edx, XERegisterBits::OverflowShift);
      a.or_(a.ecx, a.edx);
   } else if (recordCarry) {
      a.mov(a.ecx, 0);
      a.setc(a.ecx.r8());
      a.shl(a.ecx, XERegisterBits::CarryShift);
   } else if (recordOverflow) {
      a.mov(a.ecx, 0);
      a.seto(a.ecx.r8());
      a.shl(a.ecx, XERegisterBits::OverflowShift);
   }
   
   if (recordCarry || recordOverflow) {
      uint32_t mask = 0xFFFFFFFF;
      if (recordCarry) {
         mask &= ~XERegisterBits::Carry;
      }
      if (recordOverflow) {
         mask &= ~XERegisterBits::Overflow;
      }

      a.mov(a.edx, a.ppcxer);
      a.and_(a.edx, mask);
      a.or_(a.edx, a.ecx);
      a.mov(a.ppcxer, a.edx);
   }

   a.mov(a.ppcgpr[instr.rD], a.eax);

   if (recordCond) {
      updateConditionRegister(a, a.eax, a.ecx, a.edx);
   }

   return true;
}

static bool
add(PPCEmuAssembler& a, Instruction instr)
{
   return addGeneric<AddCheckRecord>(a, instr);
}

static bool
addc(PPCEmuAssembler& a, Instruction instr)
{
   return addGeneric<AddCarry | AddCheckRecord>(a, instr);
}

static bool
adde(PPCEmuAssembler& a, Instruction instr)
{
   return addGeneric<AddExtended | AddCarry | AddCheckRecord>(a, instr);
}

static bool
addi(PPCEmuAssembler& a, Instruction instr)
{
   return addGeneric<AddImmediate | AddZeroRA>(a, instr);
}

static bool
addic(PPCEmuAssembler& a, Instruction instr)
{
   return addGeneric<AddImmediate | AddCarry>(a, instr);
}

static bool
addicx(PPCEmuAssembler& a, Instruction instr)
{
   return addGeneric<AddImmediate | AddCarry | AddAlwaysRecord>(a, instr);
}

static bool
addis(PPCEmuAssembler& a, Instruction instr)
{
   return addGeneric<AddImmediate | AddShifted | AddZeroRA>(a, instr);
}

static bool
addme(PPCEmuAssembler& a, Instruction instr)
{
   return addGeneric<AddCheckRecord | AddCarry | AddExtended | AddToMinusOne>(a, instr);
}

static bool
addze(PPCEmuAssembler& a, Instruction instr)
{
   return addGeneric<AddCheckRecord | AddCarry | AddExtended | AddToZero>(a, instr);
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
static bool
andGeneric(PPCEmuAssembler& a, Instruction instr)
{
   a.mov(a.eax, a.ppcgpr[instr.rS]);

   if (flags & AndImmediate) {
      a.mov(a.ecx, instr.uimm);
   } else {
      a.mov(a.ecx, a.ppcgpr[instr.rB]);
   }
   
   if (flags & AndShifted) {
      a.shl(a.ecx, 16);
   }

   if (flags & AndComplement) {
      a.not_(a.ecx);
   }

   a.and_(a.eax, a.ecx);

   a.mov(a.ppcgpr[instr.rA], a.eax);

   if (flags & AndAlwaysRecord) {
      updateConditionRegister(a, a.eax, a.ecx, a.edx);
   } else if (flags & AndCheckRecord) {
      if (instr.rc) {
         updateConditionRegister(a, a.eax, a.ecx, a.edx);
      }
   }

   return true;
}

static bool
and_(PPCEmuAssembler& a, Instruction instr)
{
   return andGeneric<AndCheckRecord>(a, instr);
}

static bool
andc(PPCEmuAssembler& a, Instruction instr)
{
   return andGeneric<AndCheckRecord | AndComplement>(a, instr);
}

static bool
andi(PPCEmuAssembler& a, Instruction instr)
{
   return andGeneric<AndAlwaysRecord | AndImmediate>(a, instr);
}

static bool
andis(PPCEmuAssembler& a, Instruction instr)
{
   return andGeneric<AndAlwaysRecord | AndImmediate | AndShifted>(a, instr);
}

// Count Leading Zeroes Word
static bool
cntlzw(PPCEmuAssembler& a, Instruction instr)
{
   asmjit::Label lblZero(a);

   a.mov(a.ecx, a.ppcgpr[instr.rS]);
   a.mov(a.eax, 32);

   a.cmp(a.ecx, 0);
   a.je(lblZero);

   a.mov(a.edx, 0);
   a.bsr(a.edx, a.ecx);

   a.dec(a.eax);
   a.sub(a.eax, a.edx);
   
   a.bind(lblZero);
   a.mov(a.ppcgpr[instr.rA], a.eax);

   if (instr.rc) {
      updateConditionRegister(a, a.eax, a.ecx, a.edx);
   }

   return true;
}

// Divide
template<typename Type>
static bool
divGeneric(PPCEmuAssembler& a, Instruction instr)
{
   return false; /*
   Type a, b, d;
   a = state->gpr[instr.rA];
   b = state->gpr[instr.rB];

   d = a / b;
   state->gpr[instr.rD] = d;

   auto overflow = (b == 0);

   if (std::is_signed<Type>::value && (a == 0x80000000 && b == -1)) {
      overflow = true;
   }

   if (instr.oe) {
      updateOverflow(state, overflow);
   }

   if (instr.rc) {
      updateConditionRegister(state, d);
   }
   */
}

static bool
divw(PPCEmuAssembler& a, Instruction instr)
{
   return divGeneric<int32_t>(a, instr);
}

static bool
divwu(PPCEmuAssembler& a, Instruction instr)
{
   return divGeneric<uint32_t>(a, instr);
}

// Equivalent
static bool
eqv(PPCEmuAssembler& a, Instruction instr)
{
   return false; /*
   uint32_t a, s, b;

   s = state->gpr[instr.rS];
   b = state->gpr[instr.rB];

   a = ~(s ^ b);
   state->gpr[instr.rA] = a;

   if (instr.rc) {
      updateConditionRegister(state, a);
   }
   */
}

// Extend Sign Byte
static bool
extsh(PPCEmuAssembler& a, Instruction instr)
{
   return false; /*
   uint32_t a, s;

   s = state->gpr[instr.rS];

   a = sign_extend<8>(s);
   state->gpr[instr.rA] = a;

   if (instr.rc) {
      updateConditionRegister(state, a);
   }
   */
}

// Extend Sign Half Word
static bool
extsb(PPCEmuAssembler& a, Instruction instr)
{
   return false; /*
   uint32_t a, s;

   s = state->gpr[instr.rS];

   a = sign_extend<16>(s);
   state->gpr[instr.rA] = a;

   if (instr.rc) {
      updateConditionRegister(state, a);
   }
   */
}

// Multiply
enum MulFlags
{
   MulLow = 1 << 0, // (uint32_t)d
   MulHigh = 1 << 1, // d >> 32
   MulImmediate = 1 << 2, // b = simm
};

// Signed multiply
template<unsigned flags>
static bool
mulSignedGeneric(PPCEmuAssembler& a, Instruction instr)
{
   a.mov(a.eax, a.ppcgpr[instr.rA]);

   if (flags & MulImmediate) {
      a.mov(a.ecx, sign_extend<16>(instr.simm));
   } else {
      a.mov(a.ecx, a.ppcgpr[instr.rB]);
   }

   a.mul(a.zcx);

   if (flags & MulLow) {
      a.mov(a.ppcgpr[instr.rD], a.eax);

      if (instr.rc) {
         updateConditionRegister(a, a.eax, a.ecx, a.edx);
      }
   } else if (flags & MulHigh) {
      a.mov(a.ppcgpr[instr.rD], a.edx);

      if (instr.rc) {
         updateConditionRegister(a, a.edx, a.ecx, a.eax);
      }
   } else {
      assert(0);
   }

   return true;
}

// Unsigned multiply
template<MulFlags flags>
static bool
mulUnsignedGeneric(PPCEmuAssembler& a, Instruction instr)
{
   return false; /*
   uint64_t a, b;
   uint32_t d;
   a = state->gpr[instr.rA];
   b = state->gpr[instr.rB];

   if (flags & MulLow) {
      d = static_cast<uint32_t>(a * b);
   }
   else if (flags & MulHigh) {
      d = (a * b) >> 32;
   }

   state->gpr[instr.rD] = d;

   if (instr.rc) {
      updateConditionRegister(state, d);
   }
   */
}

static bool
mulhw(PPCEmuAssembler& a, Instruction instr)
{
   return mulSignedGeneric<MulHigh>(a, instr);
}

static bool
mulhwu(PPCEmuAssembler& a, Instruction instr)
{
   return mulUnsignedGeneric<MulHigh>(a, instr);
}

static bool
mulli(PPCEmuAssembler& a, Instruction instr)
{
   return mulSignedGeneric<MulImmediate | MulLow>(a, instr);
}

static bool
mullw(PPCEmuAssembler& a, Instruction instr)
{
   return mulSignedGeneric<MulLow>(a, instr);
}

// NAND
static bool
nand(PPCEmuAssembler& a, Instruction instr)
{
   return false; /*
   uint32_t a, s, b;

   s = state->gpr[instr.rS];
   b = state->gpr[instr.rB];

   a = ~(s & b);
   state->gpr[instr.rA] = a;

   if (instr.rc) {
      updateConditionRegister(state, a);
   }
   */
}

// Negate
static bool
neg(PPCEmuAssembler& a, Instruction instr)
{
   a.mov(a.eax, a.ppcgpr[instr.rA]);

   if (instr.oe) {
      asmjit::Label lblNoOverflow(a);

      // Reset overflow
      a.mov(a.edx, a.ppcxer);

      a.and_(a.edx, ~XERegisterBits::Overflow);
      a.cmp(a.eax, 0x80000000);
      a.jne(lblNoOverflow);

      a.or_(a.edx, XERegisterBits::Overflow);
      a.or_(a.edx, XERegisterBits::StickyOV);

      a.bind(lblNoOverflow);
      a.mov(a.ppcxer, a.edx);
   }

   a.neg(a.eax);
   a.mov(a.ppcgpr[instr.rD], a.eax);

   if (instr.rc) {
      updateConditionRegister(a, a.eax, a.ecx, a.edx);
   }

   return true; /*
   uint32_t a, d;

   a = state->gpr[instr.rA];

   d = ~a + 1;
   state->gpr[instr.rD] = d;

   bool overflow = (a == 0x80000000);

   if (instr.oe) {
      updateOverflow(state, overflow);
   }

   if (instr.rc) {
      updateConditionRegister(state, d);
   }
   */
}

// NOR
static bool
nor(PPCEmuAssembler& a, Instruction instr)
{
   return false; /*
   uint32_t a, s, b;

   s = state->gpr[instr.rS];
   b = state->gpr[instr.rB];

   a = ~(s | b);
   state->gpr[instr.rA] = a;

   if (instr.rc) {
      updateConditionRegister(state, a);
   }
   */
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
static bool
orGeneric(PPCEmuAssembler& a, Instruction instr)
{
   a.mov(a.eax, a.ppcgpr[instr.rS]);

   if (flags & OrImmediate) {
      a.mov(a.ecx, instr.uimm);
   }
   else {
      a.mov(a.ecx, a.ppcgpr[instr.rB]);
   }
   
   if (flags & OrShifted) {
      a.shl(a.ecx, 16);
   }

   if (flags & OrComplement) {
      a.not_(a.ecx);
   }

   a.or_(a.eax, a.ecx);
   a.mov(a.ppcgpr[instr.rA], a.eax);

   if (flags & OrAlwaysRecord) {
      updateConditionRegister(a, a.eax, a.ecx, a.edx);
   }
   else if (flags & OrCheckRecord) {
      if (instr.rc) {
         updateConditionRegister(a, a.eax, a.ecx, a.edx);
      }
   }

   return true;
}

static bool
or_(PPCEmuAssembler& a, Instruction instr)
{
   return orGeneric<OrCheckRecord>(a, instr);
}

static bool
orc(PPCEmuAssembler& a, Instruction instr)
{
   return orGeneric<OrCheckRecord | OrComplement>(a, instr);
}

static bool
ori(PPCEmuAssembler& a, Instruction instr)
{
   return orGeneric<OrAlwaysRecord | OrImmediate>(a, instr);
}

static bool
oris(PPCEmuAssembler& a, Instruction instr)
{
   return orGeneric<OrAlwaysRecord | OrImmediate | OrShifted>(a, instr);
}

// Rotate left word
enum RlwFlags
{
   RlwImmediate = 1 << 0, // n = sh
   RlwAnd = 1 << 1, // a = r & m
   RlwInsert = 1 << 2  // a = (r & m) | (r & ~m)
};

template<unsigned flags>
static bool
rlwGeneric(PPCEmuAssembler& a, Instruction instr)
{
   a.mov(a.eax, a.ppcgpr[instr.rS]);

   if (flags & RlwImmediate) {
      a.rol(a.eax, instr.sh);
   } else {
      a.mov(a.edx, a.ppcgpr[instr.rB]);
      a.and_(a.edx, 0x1f);
      a.rol(a.eax, a.edx);
   }

   uint32_t m = make_bitmask<uint32_t>(31 - instr.me, 31 - instr.mb);
   if (flags & RlwAnd) {
      a.and_(a.eax, m);
   }
   if (flags & RlwInsert) {
      a.mov(a.ecx, a.ppcgpr[instr.rA]);
      a.and_(a.ecx, ~m);
      a.or_(a.eax, a.ecx);
   }

   a.mov(a.ppcgpr[instr.rA], a.eax);

   if (instr.rc) {
      updateConditionRegister(a, a.eax, a.ecx, a.edx);
   }

   return true;
}

// Rotate Left Word Immediate then Mask Insert
static bool
rlwimi(PPCEmuAssembler& a, Instruction instr)
{
   return rlwGeneric<RlwImmediate | RlwInsert>(a, instr);
}

// Rotate Left Word Immediate then AND with Mask
static bool
rlwinm(PPCEmuAssembler& a, Instruction instr)
{
   return rlwGeneric<RlwImmediate | RlwAnd>(a, instr);
}

// Rotate Left Word then AND with Mask
static bool
rlwnm(PPCEmuAssembler& a, Instruction instr)
{
   return rlwGeneric<RlwAnd>(a, instr);
}

// Shift Logical
enum ShiftFlags
{
   ShiftLeft = 1 << 0,
   ShiftRight = 1 << 1,
   ShiftImmediate = 1 << 2,
};

template<unsigned flags>
static bool
shiftLogical(PPCEmuAssembler& a, Instruction instr)
{
   return false; /*
   uint32_t n, s, b, a;

   s = state->gpr[instr.rS];

   if (flags & ShiftImmediate) {
      b = instr.sh;
   }
   else {
      b = state->gpr[instr.rB];
   }

   n = b & 0x1f;

   if (flags & ShiftLeft) {
      a = s << n;
   }
   else if (flags & ShiftRight) {
      a = s >> n;
   }

   state->gpr[instr.rA] = a;

   if (instr.rc) {
      updateConditionRegister(state, a);
   }
   */
}

// Shift Left Word
static bool
slw(PPCEmuAssembler& a, Instruction instr)
{
   return shiftLogical<ShiftLeft>(a, instr);
}

// Shift Right Word
static bool
srw(PPCEmuAssembler& a, Instruction instr)
{
   return shiftLogical<ShiftRight>(a, instr);
}

// Shift Arithmetic
template<unsigned flags>
static bool
shiftArithmetic(PPCEmuAssembler& a, Instruction instr)
{
   return false; /*
   int32_t s, a, n, b;

   s = state->gpr[instr.rS];

   if (flags & ShiftImmediate) {
      b = instr.sh;
   }
   else {
      b = state->gpr[instr.rB];
   }

   n = b & 0x1f;

   if (flags & ShiftLeft) {
      a = s << n;
   }
   else if (flags & ShiftRight) {
      a = s >> n;
   }

   state->gpr[instr.rA] = a;

   // XER[CA] is set if rS contains a negative number and any
   // 1 bits are shifted out of position 31
   bool carry = s < 0 && !get_bit<31>(a);

   // Shift amounts from 32 to 63 give a result of 32 sign bits
   // and cause XER[CA] to receive the sign bit of rS
   if (b & 0x20) {
      carry = !!get_bit<31>(s);
   }

   // A shift amount of zero causes rA = rS and XER[CA] cleared
   if (n == 0) {
      carry = false;
   }

   updateCarry(state, carry);

   if (instr.rc) {
      updateConditionRegister(state, a);
   }
   */
}

static bool
sraw(PPCEmuAssembler& a, Instruction instr)
{
   return shiftArithmetic<ShiftRight>(a, instr);
}

static bool
srawi(PPCEmuAssembler& a, Instruction instr)
{
   return shiftArithmetic<ShiftRight | ShiftImmediate>(a, instr);
}

// Because sub is rD = ~rA + rB + 1 we can reuse our generic add
static bool
subf(PPCEmuAssembler& a, Instruction instr)
{
   return addGeneric<AddSubtract | AddCheckRecord>(a, instr);
}

static bool
subfc(PPCEmuAssembler& a, Instruction instr)
{
   return addGeneric<AddCarry | AddSubtract | AddCheckRecord>(a, instr);
}

static bool
subfe(PPCEmuAssembler& a, Instruction instr)
{
   return addGeneric<AddExtended | AddCarry | AddSubtract | AddCheckRecord>(a, instr);
}

static bool
subfic(PPCEmuAssembler& a, Instruction instr)
{
   return addGeneric<AddImmediate | AddCarry | AddSubtract>(a, instr);
}

static bool
subfme(PPCEmuAssembler& a, Instruction instr)
{
   return addGeneric<AddToMinusOne | AddExtended | AddCarry | AddCheckRecord | AddSubtract>(a, instr);
}

static bool
subfze(PPCEmuAssembler& a, Instruction instr)
{
   return addGeneric<AddToZero | AddExtended | AddCarry | AddCheckRecord | AddSubtract>(a, instr);
}

// XOR
enum XorFlags
{
   XorCheckRecord = 1 << 1, // Check rc then update cr
   XorImmediate = 1 << 2, // b = uimm
   XorShifted = 1 << 3, // b = (b << 16)
};

template<unsigned flags>
static bool
xorGeneric(PPCEmuAssembler& a, Instruction instr)
{
   a.mov(a.eax, a.ppcgpr[instr.rS]);

   if (flags & XorImmediate) {
      a.mov(a.ecx, instr.uimm);
   } else {
      a.mov(a.ecx, a.ppcgpr[instr.rB]);
   }

   if (flags & XorShifted) {
      a.shl(a.ecx, 16);
   }

   a.xor_(a.eax, a.ecx);
   a.mov(a.ppcgpr[instr.rA], a.eax);

   if (flags & XorCheckRecord) {
      if (instr.rc) {
         updateConditionRegister(a, a.eax, a.ecx, a.edx);
      }
   }
   
   return true;
}

static bool
xor_(PPCEmuAssembler& a, Instruction instr)
{
   return xorGeneric<XorCheckRecord>(a, instr);
}

static bool
xori(PPCEmuAssembler& a, Instruction instr)
{
   return xorGeneric<XorImmediate>(a, instr);
}

static bool
xoris(PPCEmuAssembler& a, Instruction instr)
{
   return xorGeneric<XorImmediate | XorShifted>(a, instr);
}

void
JitManager::registerIntegerInstructions()
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
