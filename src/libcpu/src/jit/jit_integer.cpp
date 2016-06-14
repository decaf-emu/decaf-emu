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
updateConditionRegister(PPCEmuAssembler& a, const asmjit::X86GpReg& value, const asmjit::X86GpReg& tmp, const asmjit::X86GpReg& tmp2)
{
   auto crtarget = 0;
   auto crshift = (7 - crtarget) * 4;

   a.mov(tmp, a.ppccr);
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

   a.mov(tmp2, a.ppcxer);
   a.and_(tmp2, XERegisterBits::StickyOV);
   a.shiftTo(tmp2, XERegisterBits::StickyOVShift, crshift + ConditionRegisterFlag::SummaryOverflowShift);
   a.or_(tmp, tmp2);

   a.mov(a.ppccr, tmp);
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
   auto lblZero = a.newLabel();

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
   // Need to fallback due to overflow at the moment.
   return jit_fallback(a, instr);
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
   a.mov(a.eax, a.ppcgpr[instr.rS]);
   a.mov(a.ecx, a.ppcgpr[instr.rB]);

   a.xor_(a.eax, a.ecx);
   a.not_(a.eax);

   a.mov(a.ppcgpr[instr.rA], a.eax);

   if (instr.rc) {
      updateConditionRegister(a, a.eax, a.ecx, a.edx);
   }

   return true;
}

// Extend Sign Byte
static bool
extsb(PPCEmuAssembler& a, Instruction instr)
{
   a.mov(a.eax, a.ppcgpr[instr.rS]);

   a.movsx(a.eax, a.eax.r8());

   a.mov(a.ppcgpr[instr.rA], a.eax);

   if (instr.rc) {
      updateConditionRegister(a, a.eax, a.ecx, a.edx);
   }

   return true;
}

// Extend Sign Half Word
static bool
extsh(PPCEmuAssembler& a, Instruction instr)
{
   a.mov(a.eax, a.ppcgpr[instr.rS]);

   a.movsx(a.eax, a.eax.r16());

   a.mov(a.ppcgpr[instr.rA], a.eax);

   if (instr.rc) {
      updateConditionRegister(a, a.eax, a.ecx, a.edx);
   }

   return true;
}

// Multiply
enum MulFlags
{
   MulLow         = 1 << 0, // (uint32_t)d
   MulHigh        = 1 << 1, // d >> 32
   MulImmediate   = 1 << 2, // b = simm
   MulCheckRecord = 1 << 3, // Check rc then update cr
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

   a.imul(a.ecx);

   if (flags & MulLow) {
      a.mov(a.ppcgpr[instr.rD], a.eax);

      if (flags & MulCheckRecord) {
         if (instr.rc) {
            updateConditionRegister(a, a.eax, a.ecx, a.edx);
         }
      }
   } else if (flags & MulHigh) {
      a.mov(a.ppcgpr[instr.rD], a.edx);

      if (flags & MulCheckRecord) {
         if (instr.rc) {
            updateConditionRegister(a, a.edx, a.ecx, a.eax);
         }
      }
   } else {
      assert(0);
   }

   return true;
}

// Unsigned multiply
template<unsigned flags>
static bool
mulUnsignedGeneric(PPCEmuAssembler& a, Instruction instr)
{
   a.mov(a.eax, a.ppcgpr[instr.rA]);

   if (flags & MulImmediate) {
      a.mov(a.ecx, sign_extend<16>(instr.simm));
   } else {
      a.mov(a.ecx, a.ppcgpr[instr.rB]);
   }

   a.mul(a.ecx);

   if (flags & MulLow) {
      a.mov(a.ppcgpr[instr.rD], a.eax);

      if (flags & MulCheckRecord) {
         if (instr.rc) {
            updateConditionRegister(a, a.eax, a.ecx, a.edx);
         }
      }
   } else if (flags & MulHigh) {
      a.mov(a.ppcgpr[instr.rD], a.edx);

      if (flags & MulCheckRecord) {
         if (instr.rc) {
            updateConditionRegister(a, a.edx, a.ecx, a.eax);
         }
      }
   } else {
      assert(0);
   }

   return true;
}

static bool
mulhw(PPCEmuAssembler& a, Instruction instr)
{
   return mulSignedGeneric<MulHigh | MulCheckRecord>(a, instr);
}

static bool
mulhwu(PPCEmuAssembler& a, Instruction instr)
{
   return mulUnsignedGeneric<MulHigh | MulCheckRecord>(a, instr);
}

static bool
mulli(PPCEmuAssembler& a, Instruction instr)
{
   return mulSignedGeneric<MulImmediate | MulLow>(a, instr);
}

static bool
mullw(PPCEmuAssembler& a, Instruction instr)
{
   return mulSignedGeneric<MulLow | MulCheckRecord>(a, instr);
}

// NAND
static bool
nand(PPCEmuAssembler& a, Instruction instr)
{
   a.mov(a.eax, a.ppcgpr[instr.rS]);
   a.mov(a.ecx, a.ppcgpr[instr.rB]);

   a.and_(a.eax, a.ecx);
   a.not_(a.eax);

   a.mov(a.ppcgpr[instr.rA], a.eax);

   if (instr.rc) {
      updateConditionRegister(a, a.eax, a.ecx, a.edx);
   }

   return true;
}

// Negate
static bool
neg(PPCEmuAssembler& a, Instruction instr)
{
   a.mov(a.eax, a.ppcgpr[instr.rA]);
   a.neg(a.eax);
   a.mov(a.ppcgpr[instr.rD], a.eax);

   if (instr.oe) {
      a.mov(a.ecx, 0);
      a.seto(a.ecx.r8());

      // Reset overflow
      a.mov(a.edx, a.ppcxer);
      a.and_(a.edx, ~XERegisterBits::Overflow);

      a.shiftTo(a.ecx, 0, XERegisterBits::Overflow);
      a.or_(a.edx, a.ecx);
      a.shiftTo(a.ecx, XERegisterBits::Overflow, XERegisterBits::StickyOV);
      a.or_(a.edx, a.ecx);

      a.mov(a.ppcxer, a.edx);
   }

   if (instr.rc) {
      updateConditionRegister(a, a.eax, a.ecx, a.edx);
   }

   return true;
}

// NOR
static bool
nor(PPCEmuAssembler& a, Instruction instr)
{
   a.mov(a.eax, a.ppcgpr[instr.rS]);
   a.mov(a.ecx, a.ppcgpr[instr.rB]);

   a.or_(a.eax, a.ecx);
   a.not_(a.eax);

   a.mov(a.ppcgpr[instr.rA], a.eax);

   if (instr.rc) {
      updateConditionRegister(a, a.eax, a.ecx, a.edx);
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
   a.mov(a.eax, a.ppcgpr[instr.rS]);

   if (flags & OrImmediate) {
      a.mov(a.ecx, instr.uimm);
   } else {
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
   } else if (flags & OrCheckRecord) {
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
   a.mov(a.eax, a.ppcgpr[instr.rS]);

   if (flags & RlwImmediate) {
      a.rol(a.eax, instr.sh);
   } else {
      a.mov(a.ecx, a.ppcgpr[instr.rB]);
      a.and_(a.ecx, 0x1f);
      a.rol(a.eax, a.ecx.r8());
   }

   auto m = make_ppc_bitmask(instr.mb, instr.me);

   if (flags & RlwAnd) {
      a.and_(a.eax, m);
   } else if (flags & RlwInsert) {
      a.and_(a.eax, m);
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
   a.mov(a.eax, a.ppcgpr[instr.rS]);

   if (flags & ShiftImmediate) {
      if (flags & ShiftLeft) {
         a.shl(a.zax, instr.sh);
      } else if (flags & ShiftRight) {
         a.shr(a.zax, instr.sh);
      } else {
		 throw;
      }
   } else {
      a.mov(a.ecx, a.ppcgpr[instr.rB]);

      if (flags & ShiftLeft) {
         a.shl(a.zax, a.ecx.r8());
      } else if (flags & ShiftRight) {
         a.shr(a.zax, a.ecx.r8());
      } else {
		 throw;
      }
   }

   a.mov(a.ppcgpr[instr.rA], a.eax);

   if (instr.rc) {
      updateConditionRegister(a, a.eax, a.ecx, a.edx);
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

   a.movsxd(a.zax, a.ppcgpr[instr.rS]);
   a.mov(a.edx, a.eax);

   if (flags & ShiftImmediate) {
      a.sar(a.zax, instr.sh);

      a.shl(a.zdx, 32 - instr.sh);
   } else {
      a.mov(a.ecx, a.ppcgpr[instr.rB]);
      a.mov(a.r8d, a.ecx);

      a.sar(a.zax, a.ecx.r8());

      a.shl(a.zdx, 32);
      a.shr(a.zdx, a.ecx.r8());
   }

   a.test(a.edx, a.eax);
   a.mov(a.ecx, 0);
   a.setnz(a.ecx.r8());
   a.shl(a.ecx, XERegisterBits::CarryShift);

   a.mov(a.edx, a.ppcxer);
   a.and_(a.edx, ~XERegisterBits::Carry);
   a.or_(a.edx, a.ecx);
   a.mov(a.ppcxer, a.edx);

   a.mov(a.ppcgpr[instr.rA], a.eax);

   if (instr.rc) {
      updateConditionRegister(a, a.eax, a.ecx, a.edx);
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
