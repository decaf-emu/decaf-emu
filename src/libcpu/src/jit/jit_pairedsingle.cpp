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
   if (instr.rc) {
      return jit_fallback(a, instr);
   }

   // We need to use a temporary here since frA/frB and frD may overlap
   auto srcA = a.loadRegisterRead(a.fprps[instr.frA]);
   auto tmpSrcB = a.allocXmmTmp(a.loadRegisterRead(a.fprps[instr.frB]));

   uint8_t shflFlags = 0;

   if (flags & MergeValue0) {
      shflFlags |= (1 << 0);
   }

   if (flags & MergeValue1) {
      shflFlags |= (1 << 1);
   }

   auto dst = a.loadRegisterWrite(a.fprps[instr.frD]);
   a.movapd(dst, srcA);
   a.shufpd(dst, tmpSrcB, shflFlags);

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
