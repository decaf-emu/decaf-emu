#include "bitutils.h"
#include "jit.h"

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
   RegisterInstruction(mfcr);
   RegisterInstruction(mtcrf);
}
