#include "jit_insreg.h"
#include "common/bitutils.h"

using espresso::XERegisterBits;
using espresso::ConditionRegisterFlag;

namespace cpu
{

namespace jit
{

PPCEmuAssembler::GpRegister
getCRB(PPCEmuAssembler& a,
       uint32_t bit)
{
   auto shift = 31 - bit;

   auto out = a.allocGpTmp().r32();

   a.mov(out, a.loadRegisterRead(a.cr));
   a.shr(out, shift);
   a.and_(out, 1);

   return out;
}

void
setCRB(PPCEmuAssembler& a,
       uint32_t bit,
       const PPCEmuAssembler::GpRegister& value)
{
   auto shift = 31 - bit;

   auto ppccr = a.loadRegisterReadWrite(a.cr);
   a.and_(ppccr, ~(1 << shift));
   a.and_(value, 1);
   a.shl(value, shift);
   a.or_(ppccr, value);
}

// Compare
enum CmpFlags
{
   CmpImmediate = 1 << 0, // b = imm
};

template<typename Type, unsigned flags = 0>
static bool
cmpGeneric(PPCEmuAssembler& a, Instruction instr)
{
   uint32_t crshift = (7 - instr.crfD) * 4;

   // We allocate this up here so that any spill/alloc that
   //  need to occur do not interfer with SETxx.
   auto tmp = a.allocGpTmp().r32();

   // Load CRF
   auto ppccr = a.loadRegisterReadWrite(a.cr);

   // Mask CRF
   uint32_t mask = 0xFFFFFFFF;
   mask &= ~(0xF << crshift);
   a.and_(ppccr, mask);

   // Summary Overflow
   {
      auto ppcxer = a.loadRegisterRead(a.xer);
      auto tmp = a.allocGpTmp().r32();
      a.mov(tmp, ppcxer);
      a.and_(tmp, XERegisterBits::StickyOV);
      a.shr(tmp, XERegisterBits::StickyOVShift);
      a.shl(tmp, ConditionRegisterFlag::SummaryOverflowShift + crshift);
      a.or_(ppccr, tmp);
   }

   // Perform Comparison
   auto src0 = a.loadRegisterRead(a.gpr[instr.rA]);

   PPCEmuAssembler::GpRegister src1;
   if (flags & CmpImmediate) {
      src1 = a.allocGpTmp().r32();
      if (std::is_signed<Type>::value) {
         a.mov(src1, sign_extend<16>(instr.simm));
      } else {
         a.mov(src1, instr.uimm);
      }
   } else {
      src1 = a.loadRegisterRead(a.gpr[instr.rB]);
   }

   a.cmp(src0, src1);

   auto isGreater = a.allocGpTmp().r32();
   a.mov(isGreater, 0);
   if (std::is_unsigned<Type>::value) {
      a.seta(isGreater.r8());
   } else {
      a.setg(isGreater.r8());
   }

   auto isLesser = a.allocGpTmp().r32();
   a.mov(isLesser, 0);
   if (std::is_unsigned<Type>::value) {
      a.setb(isLesser.r8());
   } else {
      a.setl(isLesser.r8());
   }

   a.mov(tmp, 0);
   a.sete(tmp.r8());
   a.shl(tmp, crshift + ConditionRegisterFlag::ZeroShift);
   a.or_(ppccr, tmp);

   a.mov(tmp, isGreater);
   a.shl(tmp, crshift + ConditionRegisterFlag::PositiveShift);
   a.or_(ppccr, tmp);

   a.mov(tmp, isLesser);
   a.shl(tmp, crshift + ConditionRegisterFlag::NegativeShift);
   a.or_(ppccr, tmp);

   return true;
}

static bool
cmp(PPCEmuAssembler& a, Instruction instr)
{
   return cmpGeneric<int32_t>(a, instr);
}

static bool
cmpi(PPCEmuAssembler& a, Instruction instr)
{
   return cmpGeneric<int32_t, CmpImmediate>(a, instr);
}

static bool
cmpl(PPCEmuAssembler& a, Instruction instr)
{
   return cmpGeneric<uint32_t>(a, instr);
}

static bool
cmpli(PPCEmuAssembler& a, Instruction instr)
{
   return cmpGeneric<uint32_t, CmpImmediate>(a, instr);
}

// Floating Compare
enum FCmpFlags
{
   FCmpOrdered = 1 << 0,
   FCmpUnordered = 1 << 1,
   FCmpSingle0 = 1 << 2,
   FCmpSingle1 = 1 << 3,
};

template<typename Type, unsigned flags>
static bool
fcmpGeneric(PPCEmuAssembler& a, Instruction instr)
{
   return jit_fallback(a, instr);
}

static bool
fcmpo(PPCEmuAssembler& a, Instruction instr)
{
   return fcmpGeneric<double, FCmpOrdered>(a, instr);
}

static bool
fcmpu(PPCEmuAssembler& a, Instruction instr)
{
   return fcmpGeneric<double, FCmpUnordered>(a, instr);
}

// Condition Register AND
static bool
crand(PPCEmuAssembler& a, Instruction instr)
{
   auto crbA = getCRB(a, instr.crbA);
   auto crbB = getCRB(a, instr.crbB);
   a.and_(crbA, crbB);
   setCRB(a, instr.crbD, crbA);
   return true;
}

// Condition Register AND with Complement
static bool
crandc(PPCEmuAssembler& a, Instruction instr)
{
   auto crbA = getCRB(a, instr.crbA);
   auto crbB = getCRB(a, instr.crbB);
   a.not_(crbB);
   a.and_(crbA, crbB);
   setCRB(a, instr.crbD, crbA);
   return true;
}

// Condition Register Equivalent
static bool
creqv(PPCEmuAssembler& a, Instruction instr)
{
   auto crbA = getCRB(a, instr.crbA);
   auto crbB = getCRB(a, instr.crbB);
   a.xor_(crbA, crbB);
   a.not_(crbA);
   setCRB(a, instr.crbD, crbA);
   return true;
}

// Condition Register NAND
static bool
crnand(PPCEmuAssembler& a, Instruction instr)
{
   auto crbA = getCRB(a, instr.crbA);
   auto crbB = getCRB(a, instr.crbB);
   a.and_(crbA, crbB);
   a.not_(crbA);
   setCRB(a, instr.crbD, crbA);
   return true;
}

// Condition Register NOR
static bool
crnor(PPCEmuAssembler& a, Instruction instr)
{
   auto crbA = getCRB(a, instr.crbA);
   auto crbB = getCRB(a, instr.crbB);
   a.or_(crbA, crbB);
   a.not_(crbA);
   setCRB(a, instr.crbD, crbA);
   return true;
}

// Condition Register OR
static bool
cror(PPCEmuAssembler& a, Instruction instr)
{
   auto crbA = getCRB(a, instr.crbA);
   auto crbB = getCRB(a, instr.crbB);
   a.or_(crbA, crbB);
   setCRB(a, instr.crbD, crbA);
   return true;
}

// Condition Register OR with Complement
static bool
crorc(PPCEmuAssembler& a, Instruction instr)
{
   auto crbA = getCRB(a, instr.crbA);
   auto crbB = getCRB(a, instr.crbB);
   a.not_(crbB);
   a.or_(crbA, crbB);
   setCRB(a, instr.crbD, crbA);
   return true;
}

// Condition Register XOR
static bool
crxor(PPCEmuAssembler& a, Instruction instr)
{
   auto crbA = getCRB(a, instr.crbA);
   auto crbB = getCRB(a, instr.crbB);
   a.xor_(crbA, crbB);
   setCRB(a, instr.crbD, crbA);
   return true;
}

// Move Condition Register Field
static bool
mcrf(PPCEmuAssembler& a, Instruction instr)
{
   uint32_t crshifts = (7 - instr.crfS) * 4;
   uint32_t crshiftd = (7 - instr.crfD) * 4;

   auto tmp = a.allocGpTmp().r32();
   auto ppccr = a.loadRegisterReadWrite(a.cr);

   a.mov(tmp, ppccr);
   a.shiftTo(tmp, crshifts, crshiftd);
   a.and_(tmp, 0xF << crshiftd);
   a.and_(ppccr, ~(0xF << crshiftd));
   a.or_(ppccr, tmp);

   return true;
}

// Move to Condition Register from XER
static bool
mcrxr(PPCEmuAssembler& a, Instruction instr)
{
   uint32_t crshift = (7 - instr.crfD) * 4;

   auto ppcxer = a.loadRegisterReadWrite(a.xer);
   auto crxr = a.allocGpTmp().r32();

   // Grab CRXR
   a.mov(crxr, ppcxer);
   a.shr(crxr, XERegisterBits::XRShift);
   a.and_(crxr, 0xF);

   // Clear XER CRXR
   a.and_(ppcxer, ~XERegisterBits::XR);

   // Set CRF
   auto ppccr = a.loadRegisterReadWrite(a.cr);
   a.shl(crxr, crshift);
   a.and_(ppccr, ~(0xF << crshift));
   a.or_(ppccr, crxr);

   return true;
}


// Move from Condition Register
static bool
mfcr(PPCEmuAssembler& a, Instruction instr)
{
   auto ppccr = a.loadRegisterRead(a.cr);
   auto dst = a.loadRegisterWrite(a.gpr[instr.rD]);
   a.mov(dst, ppccr);

   return true;
}

// Move to Condition Register Fields
static bool
mtcrf(PPCEmuAssembler& a, Instruction instr)
{
   uint32_t crm = instr.crm;
   uint32_t mask = 0;
   for (auto i = 0u; i < 8; ++i) {
      if (crm & (1 << i)) {
         mask |= 0xf << (i * 4);
      }
   }

   auto tmp = a.allocGpTmp().r32();
   a.mov(tmp, a.loadRegisterRead(a.gpr[instr.rS]));
   a.and_(tmp, mask);

   auto ppccr = a.loadRegisterReadWrite(a.cr);
   a.and_(ppccr, ~mask);
   a.or_(ppccr, tmp);

   return true;
}

void registerConditionInstructions()
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
   RegisterInstruction(mcrxr);
   RegisterInstruction(mfcr);
   RegisterInstruction(mtcrf);
   RegisterInstructionFallback(ps_cmpu0);
   RegisterInstructionFallback(ps_cmpo0);
   RegisterInstructionFallback(ps_cmpu1);
   RegisterInstructionFallback(ps_cmpo1);
}

} // namespace jit

} // namespace cpu
