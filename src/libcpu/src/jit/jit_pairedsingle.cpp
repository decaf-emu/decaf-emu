#include "jit_insreg.h"
#include "jit_float.h"
#include "common/bitutils.h"

namespace cpu
{

namespace jit
{

// Merge registers
enum MergeFlags
{
   MergeValue0 = 1 << 0,
   MergeValue1 = 1 << 1
};

template<unsigned flags = 0>
static bool
mergeGeneric(PPCEmuAssembler& a, Instruction instr)
{
   // We need to use a temporary here since frA/frB and frD may overlap
   auto src0Tmp = a.allocXmmTmp();

   if (flags & MergeValue0) {
      a.movq(src0Tmp, a.loadRegister(a.fprps[instr.frA][1]));
   } else {
      a.movq(src0Tmp, a.loadRegister(a.fprps[instr.frA][0]));
   }

   // Scope this to save us a register eviction
   {
      auto dst1 = a.allocRegister(a.fprps[instr.frD][1]);
      if (flags & MergeValue1) {
         a.movq(dst1, a.loadRegister(a.fprps[instr.frB][1]));
      } else {
         a.movq(dst1, a.loadRegister(a.fprps[instr.frB][0]));
      }
   }

   auto dst0 = a.allocRegister(a.fprps[instr.frD][0]);
   a.movq(dst0, src0Tmp);

   // Update the condition register
   if (instr.rc) {
      updateFloatConditionRegister(a);
   }

   return true;
}

static bool
ps_merge00(PPCEmuAssembler& a, Instruction instr)
{
   return mergeGeneric(a, instr);
}

static bool
ps_merge01(PPCEmuAssembler& a, Instruction instr)
{
   return mergeGeneric<MergeValue1>(a, instr);
}

static bool
ps_merge11(PPCEmuAssembler& a, Instruction instr)
{
   return mergeGeneric<MergeValue0 | MergeValue1>(a, instr);
}

static bool
ps_merge10(PPCEmuAssembler& a, Instruction instr)
{
   return mergeGeneric<MergeValue0>(a, instr);
}

void registerPairedInstructions()
{
   RegisterInstructionFallback(ps_add);
   RegisterInstructionFallback(ps_div);
   RegisterInstructionFallback(ps_mul);
   RegisterInstructionFallback(ps_sub);
   RegisterInstructionFallback(ps_abs);
   RegisterInstructionFallback(ps_nabs);
   RegisterInstructionFallback(ps_neg);
   RegisterInstructionFallback(ps_sel);
   RegisterInstructionFallback(ps_res);
   RegisterInstructionFallback(ps_rsqrte);
   RegisterInstructionFallback(ps_msub);
   RegisterInstructionFallback(ps_madd);
   RegisterInstructionFallback(ps_nmsub);
   RegisterInstructionFallback(ps_nmadd);
   RegisterInstructionFallback(ps_mr);
   RegisterInstructionFallback(ps_sum0);
   RegisterInstructionFallback(ps_sum1);
   RegisterInstructionFallback(ps_muls0);
   RegisterInstructionFallback(ps_muls1);
   RegisterInstructionFallback(ps_madds0);
   RegisterInstructionFallback(ps_madds1);
   RegisterInstruction(ps_merge00);
   RegisterInstruction(ps_merge01);
   RegisterInstruction(ps_merge10);
   RegisterInstruction(ps_merge11);
}

} // namespace jit

} // namespace cpu
