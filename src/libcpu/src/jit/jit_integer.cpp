#include <cassert>
#include "jit_insreg.h"
#include "common/bitutils.h"

using espresso::ConditionRegisterFlag;
using espresso::XERegisterBits;

namespace cpu
{

namespace jit
{

// Update cr0 with value
static void
updateConditionRegister(PPCEmuAssembler& a,
                        const PPCEmuAssembler::GpRegister& value,
                        const PPCEmuAssembler::RegLockout& eaxLockout)
{
   decaf_check(eaxLockout.isRegister(asmjit::x86::rax));

   auto crtarget = 0;
   auto crshift = (7 - crtarget) * 4;

   auto ppccr = a.loadRegisterReadWrite(a.cr);
   auto tmp = a.allocGpTmp().r32();
   auto tmp2 = a.allocGpTmp().r32();

   a.mov(tmp, ppccr);
   a.and_(tmp, ~(0xF << crshift));

   a.cmp(value, 0);

   a.lahf();
   a.mov(tmp2, 0);
   a.sete(tmp2.r8());
   a.shl(tmp2, crshift + ConditionRegisterFlag::ZeroShift);
   a.or_(tmp, tmp2);

   a.sahf();
   a.mov(tmp2, 0);
   a.setg(tmp2.r8());
   a.shl(tmp2, crshift + ConditionRegisterFlag::PositiveShift);
   a.or_(tmp, tmp2);

   a.sahf();
   a.mov(tmp2, 0);
   a.setl(tmp2.r8());
   a.shl(tmp2, crshift + ConditionRegisterFlag::NegativeShift);
   a.or_(tmp, tmp2);

   auto ppcxer = a.loadRegisterRead(a.xer);
   a.mov(tmp2, ppcxer);
   a.and_(tmp2, XERegisterBits::StickyOV);
   a.shiftTo(tmp2, XERegisterBits::StickyOVShift, crshift + ConditionRegisterFlag::SummaryOverflowShift);
   a.or_(tmp, tmp2);

   a.mov(ppccr, tmp);
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
static bool
addGeneric(PPCEmuAssembler& a, Instruction instr)
{
   auto eaxLockout = a.lockRegister(asmjit::x86::rax);

   bool recordCarry = false;
   bool recordOverflow = false;
   bool recordCond = false;

   if (flags & AddCarry) {
      recordCarry = true;
   }

   if (flags & AddAlwaysRecord) {
      recordOverflow = false;
      recordCond = true;
   } else if (flags & AddCheckRecord) {
      if (instr.oe) {
         recordOverflow = true;
      }

      if (instr.rc) {
         recordCond = true;
      }
   }

   auto dst = a.loadRegisterWrite(a.gpr[instr.rD]);

   {
      auto src0 = a.allocGpTmp().r32();

      if ((flags & AddZeroRA) && instr.rA == 0) {
         a.mov(src0, 0);
      } else {
         a.mov(src0, a.loadRegisterRead(a.gpr[instr.rA]));
      }

      if (flags & AddSubtract) {
         a.not_(src0);
      }

      auto src1 = a.allocGpTmp().r32();

      if (flags & AddImmediate) {
         a.mov(src1, sign_extend<16>(instr.simm));
      } else if (flags & AddToZero) {
         a.mov(src1, 0);
      } else if (flags & AddToMinusOne) {
         a.mov(src1, -1);
      } else {
         a.mov(src1, a.loadRegisterRead(a.gpr[instr.rB]));
      }

      if (flags & AddShifted) {
         a.shl(src1, 16);
      }

      // Mark x64 CF based on PPC CF
      if (flags & AddExtended) {
         auto carry = a.allocGpTmp().r32();
         a.mov(carry, a.loadRegisterRead(a.xer));
         a.and_(carry, XERegisterBits::Carry);
         a.add(carry, 0xffffffff);

         a.adc(src0, src1);
      } else if (flags & AddSubtract) {
         a.stc();

         a.adc(src0, src1);
      } else {
         a.add(src0, src1);
      }

      a.mov(dst, src0);

      // We reuse src0, src1 below to avoid any register spills from
      //  interferring with the SETxx instructions.
      auto xerbits = src0;
      auto tmp = src1;
      if (recordCarry && recordOverflow) {
         a.mov(xerbits, 0);
         a.setc(xerbits.r8());
         a.mov(tmp, 0);
         a.seto(tmp.r8());

         a.shiftTo(xerbits, 0, XERegisterBits::CarryShift);
         a.shiftTo(tmp, 0, XERegisterBits::OverflowShift);
         a.or_(xerbits, tmp);
         a.shiftTo(tmp, XERegisterBits::OverflowShift, XERegisterBits::StickyOVShift);
         a.or_(xerbits, tmp);
      } else if (recordCarry) {
         a.mov(xerbits, 0);
         a.setc(xerbits.r8());
         a.shiftTo(xerbits, 0, XERegisterBits::CarryShift);
      } else if (recordOverflow) {
         a.mov(xerbits, 0);
         a.seto(xerbits.r8());
         a.mov(tmp, xerbits);

         a.shiftTo(xerbits, 0, XERegisterBits::OverflowShift);
         a.shiftTo(tmp, 0, XERegisterBits::StickyOVShift);
         a.or_(xerbits, tmp);
      }

      if (recordCarry || recordOverflow) {
         uint32_t mask = 0xFFFFFFFF;

         if (recordCarry) {
            mask &= ~XERegisterBits::Carry;
         }

         if (recordOverflow) {
            mask &= ~XERegisterBits::Overflow;
         }

         auto ppcxer = a.loadRegisterReadWrite(a.xer);
         a.and_(ppcxer, mask);
         a.or_(ppcxer, xerbits);
      }
   }

   if (recordCond) {
      updateConditionRegister(a, dst, eaxLockout);
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
   AndComplement     = 1 << 0, // b = ~b
   AndCheckRecord    = 1 << 1, // Check rc then update cr
   AndImmediate      = 1 << 2, // b = uimm
   AndShifted        = 1 << 3, // b = (b << 16)
   AndAlwaysRecord   = 1 << 4, // Always update cr
};

template<unsigned flags>
static bool
andGeneric(PPCEmuAssembler& a, Instruction instr)
{
   auto eaxLockout = a.lockRegister(asmjit::x86::rax);

   auto dst = a.loadRegisterWrite(a.gpr[instr.rA]);

   {
      auto src0 = a.loadRegisterRead(a.gpr[instr.rS]);
      auto tmp = a.allocGpTmp().r32();

      if (flags & AndImmediate) {
         a.mov(tmp, instr.uimm);
      } else {
         a.mov(tmp, a.loadRegisterRead(a.gpr[instr.rB]));
      }

      if (flags & AndShifted) {
         a.shl(tmp, 16);
      }

      if (flags & AndComplement) {
         a.not_(tmp);
      }

      a.and_(tmp, src0);

      a.mov(dst, tmp);
   }

   if (flags & AndAlwaysRecord) {
      updateConditionRegister(a, dst, eaxLockout);
   } else if (flags & AndCheckRecord) {
      if (instr.rc) {
         updateConditionRegister(a, dst, eaxLockout);
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
   auto eaxLockout = a.lockRegister(asmjit::x86::rax);

   auto dst = a.loadRegisterWrite(a.gpr[instr.rA]);

   {
      auto lblZero = a.newLabel();

      auto src0 = a.loadRegisterRead(a.gpr[instr.rS]);
      auto tmp = a.allocGpTmp().r32();
      auto tmp2 = a.allocGpTmp().r32();

      a.mov(tmp, 32);

      a.cmp(src0, 0);
      a.je(lblZero);

      a.mov(tmp2, 0);
      a.bsr(tmp2, src0);

      a.dec(tmp);
      a.sub(tmp, tmp2);

      a.bind(lblZero);
      a.mov(dst, tmp);
   }

   if (instr.rc) {
      updateConditionRegister(a, dst, eaxLockout);
   }

   return true;
}

// Divide
template<typename Type>
static bool
divGeneric(PPCEmuAssembler& a, Instruction instr)
{
   auto eaxLockout = a.lockRegister(asmjit::x86::rax);
   auto edxLockout = a.lockRegister(asmjit::x86::rdx);

   auto dst = a.loadRegisterWrite(a.gpr[instr.rD]);
   auto srcA = a.loadRegisterRead(a.gpr[instr.rA]);
   auto srcB = a.loadRegisterRead(a.gpr[instr.rB]);

   auto ppcxer = a.loadRegisterReadWrite(a.xer);

   auto overflowLbl = a.newLabel();
   auto endLbl = a.newLabel();

   a.test(srcB, srcB);
   a.jz(overflowLbl);

   if (std::is_signed<Type>::value) {
      auto noOverflowLbl = a.newLabel();
      a.cmp(srcA, 0x80000000);
      a.jne(noOverflowLbl);
      a.cmp(srcB, 0xffffffff);
      a.je(overflowLbl);
      a.bind(noOverflowLbl);

      a.mov(asmjit::x86::edx, srcA);
      a.sar(asmjit::x86::edx, 31);
      a.mov(asmjit::x86::eax, srcA);
      a.idiv(srcB);
   } else {
      a.xor_(asmjit::x86::edx, asmjit::x86::edx);
      a.mov(asmjit::x86::eax, srcA);
      a.div(srcB);
   }

   // Move the result to the destination register
   a.mov(dst, asmjit::x86::eax);

   // Clear the Overflow flag
   a.and_(ppcxer, ~XERegisterBits::Overflow);

   a.jmp(endLbl);
   a.bind(overflowLbl);

   if (!std::is_signed<Type>::value) {
      // If unsigned division, overflow makes dst 0
      a.mov(dst, 0);
   } else {
      // If signed divison, overflow is 0/-1 depending on sign of srcB
      a.mov(dst, srcA);
      a.sar(dst, 31);
   }

   // Set Overflow bits
   a.or_(ppcxer, XERegisterBits::StickyOV | XERegisterBits::Overflow);

   a.bind(endLbl);

   if (instr.rc) {
      updateConditionRegister(a, dst, eaxLockout);
   }

   return true;
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
   auto eaxLockout = a.lockRegister(asmjit::x86::rax);

   auto dst = a.loadRegisterWrite(a.gpr[instr.rA]);

   {
      auto tmp = a.allocGpTmp(a.loadRegisterRead(a.gpr[instr.rS]));
      auto src1 = a.loadRegisterRead(a.gpr[instr.rB]);

      a.xor_(tmp, src1);
      a.not_(tmp);

      a.mov(dst, tmp);
   }

   if (instr.rc) {
      updateConditionRegister(a, dst, eaxLockout);
   }

   return true;
}

// Extend Sign Byte
static bool
extsb(PPCEmuAssembler& a, Instruction instr)
{
   auto eaxLockout = a.lockRegister(asmjit::x86::rax);

   auto src = a.loadRegisterRead(a.gpr[instr.rS]);
   auto dst = a.loadRegisterWrite(a.gpr[instr.rA]);
   a.movsx(dst, src.r8());

   if (instr.rc) {
      updateConditionRegister(a, dst, eaxLockout);
   }

   return true;
}

// Extend Sign Half Word
static bool
extsh(PPCEmuAssembler& a, Instruction instr)
{
   auto eaxLockout = a.lockRegister(asmjit::x86::rax);

   auto src = a.loadRegisterRead(a.gpr[instr.rS]);
   auto dst = a.loadRegisterWrite(a.gpr[instr.rA]);
   a.movsx(dst, src.r16());

   if (instr.rc) {
      updateConditionRegister(a, dst, eaxLockout);
   }

   return true;
}

// Multiply
enum MulFlags
{
   MulLow = 1 << 0, // (uint32_t)d
   MulHigh = 1 << 1, // d >> 32
   MulImmediate = 1 << 2, // b = simm
   MulCheckOverflow = 1 << 3, // Check oe then update xer
   MulCheckRecord = 1 << 4, // Check rc then update cr
   MulSigned = 1 << 5, // Is a signed multiply
};

// Multiply
template<unsigned flags>
static bool
mulGeneric(PPCEmuAssembler& a, Instruction instr)
{
   static_assert((flags & MulLow) || (flags & MulHigh), "Unexpected mulGeneric flags");
   static_assert(!(flags & MulCheckOverflow) || ((flags & MulSigned) && (flags & MulLow)),
      "X64 cannot do overflow on high bits, or on unsigned mul");

   bool recordOverflow = (flags & MulCheckOverflow) && instr.oe;

   auto eaxLockout = a.lockRegister(asmjit::x86::rax);
   auto edxLockout = a.lockRegister(asmjit::x86::rdx);

   auto dst = a.loadRegisterWrite(a.gpr[instr.rD]);

   if (!recordOverflow) {
      a.mov(asmjit::x86::eax, a.loadRegisterRead(a.gpr[instr.rA]));

      if (flags & MulImmediate) {
         a.mov(asmjit::x86::edx, sign_extend<16>(instr.simm));
         if (flags & MulSigned) {
            a.imul(asmjit::x86::edx);
         } else {
            a.mul(asmjit::x86::edx);
         }
      } else {
         auto src1 = a.loadRegisterRead(a.gpr[instr.rB]);
         if (flags & MulSigned) {
            a.imul(src1);
         } else {
            a.mul(src1);
         }
      }

      if (flags & MulLow) {
         a.mov(dst, asmjit::x86::eax);
      } else if (flags & MulHigh) {
         a.mov(dst, asmjit::x86::edx);
      }
   } else {
      // This logic assumes based on the static_assert above that we will only
      //  be handling MulSigned and MulLow for any case of recordOverflow.

      a.movsxd(asmjit::x86::rax, a.loadRegisterRead(a.gpr[instr.rA]));

      if (flags & MulImmediate) {
         a.mov(asmjit::x86::rdx, sign_extend<16>(instr.simm));
      } else {
         auto src1 = a.loadRegisterRead(a.gpr[instr.rB]);
         a.movsxd(asmjit::x86::rdx, src1);
      }

      a.imul(asmjit::x86::rdx);

      a.mov(dst, asmjit::x86::eax);

      a.movsxd(asmjit::x86::rdx, asmjit::x86::eax);
      a.cmp(asmjit::x86::rax, asmjit::x86::rdx);

      auto overflowReg = asmjit::x86::eax;
      a.mov(overflowReg, 0);
      a.setne(overflowReg.r8());

      auto ppcxer = a.loadRegisterReadWrite(a.xer);
      a.and_(ppcxer, ~XERegisterBits::Overflow);

      a.shiftTo(overflowReg, 0, XERegisterBits::OverflowShift);
      a.or_(ppcxer, overflowReg);
      a.shiftTo(overflowReg, XERegisterBits::OverflowShift, XERegisterBits::StickyOVShift);
      a.or_(ppcxer, overflowReg);
   }

   edxLockout.unlock();

   if (flags & MulCheckRecord) {
      if (instr.rc) {
         updateConditionRegister(a, dst, eaxLockout);
      }
   }

   return true;
}

static bool
mulhw(PPCEmuAssembler& a, Instruction instr)
{
   return mulGeneric<MulSigned | MulHigh | MulCheckRecord>(a, instr);
}

static bool
mulhwu(PPCEmuAssembler& a, Instruction instr)
{
   return mulGeneric<MulHigh | MulCheckRecord>(a, instr);
}

static bool
mulli(PPCEmuAssembler& a, Instruction instr)
{
   return mulGeneric<MulSigned | MulImmediate | MulLow>(a, instr);
}

static bool
mullw(PPCEmuAssembler& a, Instruction instr)
{
   return mulGeneric<MulSigned | MulLow | MulCheckRecord | MulCheckOverflow>(a, instr);
}

// NAND
static bool
nand(PPCEmuAssembler& a, Instruction instr)
{
   auto eaxLockout = a.lockRegister(asmjit::x86::rax);

   auto dst = a.loadRegisterWrite(a.gpr[instr.rA]);

   {
      auto tmp = a.allocGpTmp(a.loadRegisterRead(a.gpr[instr.rS]));
      auto src1 = a.loadRegisterRead(a.gpr[instr.rB]);

      a.and_(tmp, src1);
      a.not_(tmp);

      a.mov(dst, tmp);
   }

   if (instr.rc) {
      updateConditionRegister(a, dst, eaxLockout);
   }

   return true;
}

// Negate
static bool
neg(PPCEmuAssembler& a, Instruction instr)
{
   auto eaxLockout = a.lockRegister(asmjit::x86::rax);

   auto dst = a.loadRegisterWrite(a.gpr[instr.rD]);
   auto src = a.loadRegisterRead(a.gpr[instr.rA]);

   if (!instr.oe) {
      a.mov(dst, src);
      a.neg(dst);
   } else {
      auto tmp = a.allocGpTmp().r32();

      a.mov(dst, src);
      a.neg(dst);

      a.mov(tmp, 0);
      a.seto(tmp.r8());

      // Reset overflow
      auto ppcxer = a.loadRegisterReadWrite(a.xer);
      a.and_(ppcxer, ~XERegisterBits::Overflow);

      a.shiftTo(tmp, 0, XERegisterBits::OverflowShift);
      a.or_(ppcxer, tmp);
      a.shiftTo(tmp, XERegisterBits::OverflowShift, XERegisterBits::StickyOVShift);
      a.or_(ppcxer, tmp);
   }

   if (instr.rc) {
      updateConditionRegister(a, dst, eaxLockout);
   }

   return true;
}

// NOR
static bool
nor(PPCEmuAssembler& a, Instruction instr)
{
   auto eaxLockout = a.lockRegister(asmjit::x86::rax);

   auto dst = a.loadRegisterWrite(a.gpr[instr.rA]);

   {
      auto tmp = a.allocGpTmp(a.loadRegisterRead(a.gpr[instr.rS]));
      auto src1 = a.loadRegisterRead(a.gpr[instr.rB]);

      a.or_(tmp, src1);
      a.not_(tmp);

      a.mov(dst, tmp);
   }

   if (instr.rc) {
      updateConditionRegister(a, dst, eaxLockout);
   }

   return true;
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
   auto eaxLockout = a.lockRegister(asmjit::x86::rax);

   auto dst = a.loadRegisterWrite(a.gpr[instr.rA]);

   {
      auto src0 = a.loadRegisterRead(a.gpr[instr.rS]);
      auto tmp = a.allocGpTmp().r32();

      if (flags & OrImmediate) {
         a.mov(tmp, instr.uimm);
      } else {
         a.mov(tmp, a.loadRegisterRead(a.gpr[instr.rB]));
      }

      if (flags & OrShifted) {
         a.shl(tmp, 16);
      }

      if (flags & OrComplement) {
         a.not_(tmp);
      }

      a.or_(tmp, src0);
      a.mov(dst, tmp);
   }

   if (flags & OrAlwaysRecord) {
      updateConditionRegister(a, dst, eaxLockout);
   } else if (flags & OrCheckRecord) {
      if (instr.rc) {
         updateConditionRegister(a, dst, eaxLockout);
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
   return orGeneric<OrImmediate>(a, instr);
}

static bool
oris(PPCEmuAssembler& a, Instruction instr)
{
   return orGeneric<OrImmediate | OrShifted>(a, instr);
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
   auto eaxLockout = a.lockRegister(asmjit::x86::rax);

   // This is needed for the ROL instruction...
   auto ecxLockout = a.lockRegister(asmjit::x86::rcx);

   auto dst = a.loadRegisterWrite(a.gpr[instr.rA]);

   {
      auto tmp = a.allocGpTmp().r32();

      a.mov(tmp, a.loadRegisterRead(a.gpr[instr.rS]));

      if (flags & RlwImmediate) {
         a.rol(tmp, instr.sh);
      } else {
         a.mov(asmjit::x86::ecx, a.loadRegisterRead(a.gpr[instr.rB]));
         a.and_(asmjit::x86::ecx, 0x1f);
         a.rol(tmp, asmjit::x86::cl);
      }

      auto m = make_ppc_bitmask(instr.mb, instr.me);

      if (flags & RlwAnd) {
         a.and_(tmp, m);
      } else if (flags & RlwInsert) {
         a.and_(tmp, m);

         auto tmp2 = a.allocGpTmp(a.loadRegisterRead(a.gpr[instr.rA]));
         a.and_(tmp2, ~m);
         a.or_(tmp, tmp2);
      }

      a.mov(dst, tmp);
   }

   if (instr.rc) {
      updateConditionRegister(a, dst, eaxLockout);
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
   auto eaxLockout = a.lockRegister(asmjit::x86::rax);

   // Needed for the register-based SHL instruction
   auto ecxLockout = a.lockRegister(asmjit::x86::rcx);

   auto dst = a.loadRegisterWrite(a.gpr[instr.rA]);

   {
      auto tmp = a.allocGpTmp().r64();

      a.mov(tmp, a.loadRegisterRead(a.gpr[instr.rS]));

      if (flags & ShiftImmediate) {
         if (flags & ShiftLeft) {
            a.shl(tmp, instr.sh);
         } else if (flags & ShiftRight) {
            a.shr(tmp, instr.sh);
         } else {
            throw;
         }
      } else {
         a.mov(asmjit::x86::ecx, a.loadRegisterRead(a.gpr[instr.rB]));

         if (flags & ShiftLeft) {
            a.shl(tmp, asmjit::x86::cl);
         } else if (flags & ShiftRight) {
            a.shr(tmp, asmjit::x86::cl);
         } else {
            throw;
         }
      }

      a.mov(dst, tmp);
   }

   if (instr.rc) {
      updateConditionRegister(a, dst, eaxLockout);
   }

   return true;
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
   if (!(flags & ShiftRight)) {
      throw;
   }

   auto eaxLockout = a.lockRegister(asmjit::x86::rax);

   // Needed for the register-based SHL instruction
   auto ecxLockout = a.lockRegister(asmjit::x86::rcx);

   auto dst = a.loadRegisterWrite(a.gpr[instr.rA]);

   {
      auto tmp = a.allocGpTmp().r64();
      auto tmp2 = a.allocGpTmp().r32();

      a.movsxd(tmp, a.loadRegisterRead(a.gpr[instr.rS]));
      a.mov(tmp2, tmp.r32());

      if (flags & ShiftImmediate) {
         a.sar(tmp.r64(), instr.sh);

         a.shl(tmp2.r64(), 32 - instr.sh);
      } else {
         a.mov(asmjit::x86::ecx, a.loadRegisterRead(a.gpr[instr.rB]));

         a.sar(tmp.r64(), asmjit::x86::cl);

         a.shl(tmp2.r64(), 32);
         a.shr(tmp2.r64(), asmjit::x86::cl);
      }

      a.test(tmp2, tmp.r32());
      a.mov(tmp2, 0);
      a.setnz(tmp2.r8());
      a.shl(tmp2, XERegisterBits::CarryShift);

      auto ppcxer = a.loadRegisterReadWrite(a.xer);
      a.and_(ppcxer, ~XERegisterBits::Carry);
      a.or_(ppcxer, tmp2);

      a.mov(dst, tmp);
   }

   if (instr.rc) {
      updateConditionRegister(a, dst, eaxLockout);
   }

   return true;
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
   auto eaxLockout = a.lockRegister(asmjit::x86::rax);

   auto dst = a.loadRegisterWrite(a.gpr[instr.rA]);

   {
      auto src0 = a.loadRegisterRead(a.gpr[instr.rS]);

      auto tmp = a.allocGpTmp().r32();

      if (flags & XorImmediate) {
         a.mov(tmp, instr.uimm);
      } else {
         a.mov(tmp, a.loadRegisterRead(a.gpr[instr.rB]));
      }

      if (flags & XorShifted) {
         a.shl(tmp, 16);
      }

      a.xor_(tmp, src0);
      a.mov(dst, tmp);
   }

   if (flags & XorCheckRecord) {
      if (instr.rc) {
         updateConditionRegister(a, dst, eaxLockout);
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

void registerIntegerInstructions()
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
   RegisterInstruction(eqv);
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

} // namespace jit

} // namespace cpu
