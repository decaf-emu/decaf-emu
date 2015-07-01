#include "bitutils.h"
#include "jit.h"

// Compare
enum CmpFlags
{
   CmpImmediate = 1 << 0, // b = imm
};

template<typename Type, unsigned flags = 0>
static bool
cmpGeneric(PPCEmuAssembler& a, Instruction instr)
{
   uint32_t crshift = instr.crfD * 4;

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
         a.cmp(a.eax, sign_extend<16>(instr.simm));
      } else {
         a.cmp(a.eax, instr.uimm);
      }
   } else {
      a.mov(a.ecx, a.ppcgpr[instr.rB]);
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
   return cmpGeneric<uint32_t>(a, instr);
}

static bool
cmpli(PPCEmuAssembler& a, Instruction instr)
{
   return cmpGeneric<uint32_t, CmpImmediate>(a, instr);
}

void
JitManager::registerConditionInstructions()
{
   RegisterInstruction(cmp);
   RegisterInstruction(cmpi);
   RegisterInstruction(cmpl);
   RegisterInstruction(cmpli);
}
