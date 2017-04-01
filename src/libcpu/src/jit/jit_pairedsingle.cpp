#include "jit_insreg.h"
#include "jit_float.h"
#include <common/bitutils.h>

namespace cpu
{

namespace jit
{

static void
roundToSingleSd(PPCEmuAssembler& a,
                const PPCEmuAssembler::XmmRegister& dst,
                const PPCEmuAssembler::XmmRegister& src)
{
   a.cvtsd2ss(dst, src);
   a.cvtss2sd(dst, dst);
}

static void
truncateToSingleSd(PPCEmuAssembler& a,
                   const PPCEmuAssembler::XmmRegister& dst,
                   const PPCEmuAssembler::XmmRegister& src)
{
   auto maskGp = a.allocGpTmp();
   a.mov(maskGp, UINT64_C(0xFFFFFFFFE0000000));
   if (&dst == &src) {
      auto tmp = a.allocXmmTmp();
      a.movq(tmp, maskGp);
      a.pand(dst, tmp);
   } else {
      a.movq(dst, maskGp);
      a.pand(dst, src);
   }
}

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

   auto tmpSrcA = a.allocXmmTmp();
   auto tmpSrcB = a.allocXmmTmp();
   {
      auto srcA = a.loadRegisterRead(a.fprps[instr.frA]);
      auto srcB = a.loadRegisterRead(a.fprps[instr.frB]);
      if (flags & MergeValue0) {
         a.movapd(tmpSrcA, srcA);
         a.shufpd(tmpSrcA, tmpSrcA, 1);
         roundToSingleSd(a, tmpSrcA, tmpSrcA);
      } else {
         roundToSingleSd(a, tmpSrcA, srcA);
      }
      if (flags & MergeValue1) {
         a.movapd(tmpSrcB, srcB);
         a.shufpd(tmpSrcB, tmpSrcB, 1);
         truncateToSingleSd(a, tmpSrcB, tmpSrcB);
      } else {
         truncateToSingleSd(a, tmpSrcB, srcB);
      }
   }

   auto dst = a.loadRegisterWrite(a.fprps[instr.frD]);
   a.movapd(dst, tmpSrcA);
   a.shufpd(dst, tmpSrcB, 0);

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
