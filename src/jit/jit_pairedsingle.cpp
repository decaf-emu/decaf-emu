#include "bitutils.h"
#include "jit.h"
#include "jit_float.h"

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
   if (flags & MergeValue0) {
      a.mov(a.eax, a.ppcfprps[instr.frA][1]);
   } else {
      a.mov(a.eax, a.ppcfprps[instr.frA][0]);
   }

   if (flags & MergeValue1) {
      a.mov(a.ecx, a.ppcfprps[instr.frB][1]);
   } else {
      a.mov(a.ecx, a.ppcfprps[instr.frB][0]);
   }

   a.mov(a.ppcfprps[instr.frD][0], a.eax);
   a.mov(a.ppcfprps[instr.frD][1], a.ecx);

   if (instr.rc) {
      updateFloatConditionRegister(a, a.eax, a.ecx);
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

void
JitManager::registerPairedInstructions()
{
   RegisterInstruction(ps_merge00);
   RegisterInstruction(ps_merge01);
   RegisterInstruction(ps_merge10);
   RegisterInstruction(ps_merge11);
}