#include "bitutils.h"
#include "jit.h"

void getTwoCRB(PPCEmuAssembler& a,
      uint32_t bita, const asmjit::X86GpReg& da, 
      uint32_t bitb, const asmjit::X86GpReg& db) {
   auto shifta = 31 - bita;
   auto shiftb = 31 - bitb;
   a.mov(da, a.ppccr);
   a.mov(db, da);
   if (shifta > 0) {
      a.shr(da, shifta);
   }
   if (shiftb > 0) {
      a.shr(db, shiftb);
   }
   a.and_(da, 1);
   a.and_(db, 1);
}

void setCRB(PPCEmuAssembler& a, uint32_t bit, const asmjit::X86GpReg& value, const asmjit::X86GpReg& tmp, const asmjit::X86GpReg& tmp2) {
   auto shift = 31 - bit;
   a.mov(tmp, a.ppccr);
   a.and_(tmp, ~(1 << shift));
   a.mov(tmp2, value);
   a.and_(tmp2, 1);
   a.shl(tmp2, shift);
   a.or_(tmp, tmp2);
   a.mov(a.ppccr, tmp);
}

// Compare
enum CmpFlags
{
   CmpImmediate = 1 << 0, // b = imm
   CmpLogical = 1 << 1
};

template<typename Type, unsigned flags = 0>
static bool
cmpGeneric(PPCEmuAssembler& a, Instruction instr)
{
   uint32_t crshift = (7 - instr.crfD) * 4;

   // Load CRF
   a.mov(a.edx, a.ppccr);
   
   // Mask CRF
   uint32_t mask = 0xFFFFFFFF;
   mask &= ~(0xF << crshift);
   a.and_(a.edx, mask);

   // Summary Overflow
   a.mov(a.eax, a.ppcxer);
   a.and_(a.eax, XERegisterBits::StickyOV);
   a.shr(a.eax, XERegisterBits::StickyOVShift);
   a.shl(a.eax, ConditionRegisterFlag::SummaryOverflowShift << crshift);
   a.or_(a.edx, a.eax);

   // Perform Comparison
   a.mov(a.eax, a.ppcgpr[instr.rA]);

   if (flags & CmpImmediate) {
      if (std::is_signed<Type>::value) {
         a.mov(a.ecx, sign_extend<16>(instr.simm));
      } else {
         a.mov(a.ecx, instr.uimm);
      }
   } else {
      a.mov(a.ecx, a.ppcgpr[instr.rB]);
   }

   if (flags & CmpLogical) {
      a.cmp(a.zax, a.zcx);
   } else {
      a.cmp(a.eax, a.ecx);
   }

   asmjit::Label bbLessThan(a);
   asmjit::Label bbGreaterThan(a);
   asmjit::Label bbEnd(a);

   a.jl(bbLessThan);
   a.jg(bbGreaterThan);

   a.or_(a.edx, ConditionRegisterFlag::Equal << crshift);
   a.jmp(bbEnd);

   a.bind(bbLessThan);
   a.or_(a.edx, ConditionRegisterFlag::LessThan << crshift);
   a.jmp(bbEnd);

   a.bind(bbGreaterThan);
   a.or_(a.edx, ConditionRegisterFlag::GreaterThan << crshift);

   a.bind(bbEnd);

   a.mov(a.ppccr, a.edx);

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
   return cmpGeneric<uint32_t, CmpLogical>(a, instr);
}

static bool
cmpli(PPCEmuAssembler& a, Instruction instr)
{
   return cmpGeneric<uint32_t, CmpLogical | CmpImmediate>(a, instr);
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
   getTwoCRB(a, instr.crbA, a.eax, instr.crbB, a.ecx);
   a.and_(a.eax, a.ecx);
   setCRB(a, instr.crbD, a.eax, a.ecx, a.edx);
   return true;
}

// Condition Register AND with Complement
static bool
crandc(PPCEmuAssembler& a, Instruction instr)
{
   getTwoCRB(a, instr.crbA, a.eax, instr.crbB, a.ecx);
   a.not_(a.ecx);
   a.and_(a.eax, a.ecx);
   setCRB(a, instr.crbD, a.eax, a.ecx, a.edx);
   return true;
}

// Condition Register Equivalent
static bool
creqv(PPCEmuAssembler& a, Instruction instr)
{
   getTwoCRB(a, instr.crbA, a.eax, instr.crbB, a.ecx);
   a.xor_(a.eax, a.ecx);
   a.not_(a.eax);
   setCRB(a, instr.crbD, a.eax, a.ecx, a.edx);
   return true;
}

// Condition Register NAND
static bool
crnand(PPCEmuAssembler& a, Instruction instr)
{
   getTwoCRB(a, instr.crbA, a.eax, instr.crbB, a.ecx);
   a.and_(a.eax, a.ecx);
   a.not_(a.eax);
   setCRB(a, instr.crbD, a.eax, a.ecx, a.edx);
   return true;
}

// Condition Register NOR
static bool
crnor(PPCEmuAssembler& a, Instruction instr)
{
   getTwoCRB(a, instr.crbA, a.eax, instr.crbB, a.ecx);
   a.or_(a.eax, a.ecx);
   a.not_(a.eax);
   setCRB(a, instr.crbD, a.eax, a.ecx, a.edx);
   return true;
}

// Condition Register OR
static bool
cror(PPCEmuAssembler& a, Instruction instr)
{
   getTwoCRB(a, instr.crbA, a.eax, instr.crbB, a.ecx);
   a.or_(a.eax, a.ecx);
   setCRB(a, instr.crbD, a.eax, a.ecx, a.edx);
   return true;
}

// Condition Register OR with Complement
static bool
crorc(PPCEmuAssembler& a, Instruction instr)
{
   getTwoCRB(a, instr.crbA, a.eax, instr.crbB, a.ecx);
   a.not_(a.ecx);
   a.or_(a.eax, a.ecx);
   setCRB(a, instr.crbD, a.eax, a.ecx, a.edx);
   return true;
}

// Condition Register XOR
static bool
crxor(PPCEmuAssembler& a, Instruction instr)
{
   getTwoCRB(a, instr.crbA, a.eax, instr.crbB, a.ecx);
   a.xor_(a.eax, a.ecx);
   setCRB(a, instr.crbD, a.eax, a.ecx, a.edx);
   return true;
}

// Move Condition Register Field
static bool
mcrf(PPCEmuAssembler& a, Instruction instr)
{
   uint32_t crshifts = (7 - instr.crfS) * 4;
   uint32_t crshiftd = (7 - instr.crfD) * 4;

   a.mov(a.eax, a.ppccr);
   a.mov(a.ecx, a.eax);
   a.and_(a.ecx, ~(0xF << crshifts));
   a.shr(a.ecx, crshifts);
   a.shl(a.ecx, crshiftd);
   a.and_(a.eax, ~(0xF << crshiftd));
   a.or_(a.eax, a.ecx);
   a.mov(a.ppccr, a.eax);

   return true;
}

// Move to Condition Register from XER
static bool
mcrxr(PPCEmuAssembler& a, Instruction instr)
{
   uint32_t crshift = (7 - instr.crfD) * 4;

   a.mov(a.eax, a.ppcxer);
   a.mov(a.ecx, a.eax);

   // Grab CRXR
   a.shr(a.ecx, XERegisterBits::XRShift);
   a.and_(a.ecx, 0xF);

   // Clear XER CRXR
   a.and_(a.eax, ~XERegisterBits::XR);
   a.mov(a.ppcxer, a.eax);

   // Set CRF
   a.shl(a.ecx, crshift);
   a.mov(a.eax, a.ppccr);
   a.and_(a.eax, ~(0xF << crshift));
   a.or_(a.eax, a.ecx);
   a.mov(a.ppccr, a.eax);

   return true;
}


// Move from Condition Register
static bool
mfcr(PPCEmuAssembler& a, Instruction instr)
{
   a.mov(a.eax, a.ppccr);
   a.mov(a.ppcgpr[instr.rD], a.eax);
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

   a.mov(a.eax, a.ppcgpr[instr.rS]);
   a.and_(a.eax, mask);
   a.mov(a.ecx, a.ppccr);
   a.and_(a.ecx, ~mask);
   a.or_(a.eax, a.ecx);
   a.mov(a.ppccr, a.eax);
   return true;
}

void
JitManager::registerConditionInstructions()
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
