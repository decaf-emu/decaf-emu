#include <cassert>
#include "jit_insreg.h"
#include "jit_float.h"
#include "utils/bitutils.h"

namespace cpu
{

namespace jit
{

void
updateFloatConditionRegister(PPCEmuAssembler& a, const asmjit::X86GpReg& tmp, const asmjit::X86GpReg& tmp2)
{
   //state->cr.cr1 = state->fpscr.cr1;
   assert(0);
}

static bool
fmuls(PPCEmuAssembler& a, Instruction instr)
{
   if (instr.rc) {
      return jit_fallback(a, instr);
   }

   // FPSCR, FPRF supposed to be updated here...

   a.movq(a.xmm0, a.ppcfpr[instr.frA]);
   a.movq(a.xmm1, a.ppcfpr[instr.frC]);

   a.mulsd(a.xmm0, a.xmm1);

   a.cvtsd2ss(a.xmm1, a.xmm0);
   a.cvtss2sd(a.xmm0, a.xmm1);

   a.movq(a.ppcfpr[instr.frD], a.xmm0);

   return true;
}

static bool
fsub(PPCEmuAssembler& a, Instruction instr)
{
   if (instr.rc) {
      return jit_fallback(a, instr);
   }

   // FPSCR, FPRF supposed to be updated here...

   a.movq(a.xmm0, a.ppcfpr[instr.frA]);
   a.movq(a.xmm1, a.ppcfpr[instr.frB]);

   a.subsd(a.xmm0, a.xmm1);

   a.movq(a.ppcfpr[instr.frD], a.xmm0);

   return true;
}

static bool
frsp(PPCEmuAssembler& a, Instruction instr)
{
   if (instr.rc) {
      return jit_fallback(a, instr);
   }

   // FPSCR, FPRF supposed to be updated here...

   a.movq(a.xmm0, a.ppcfpr[instr.frB]);
   a.cvtsd2ss(a.xmm1, a.xmm0);
   a.cvtss2sd(a.xmm0, a.xmm1);
   a.movq(a.ppcfpr[instr.frD], a.xmm0);

   return true;
}

void registerFloatInstructions()
{
   RegisterInstructionFallback(fadd);
   RegisterInstructionFallback(fadds);
   RegisterInstructionFallback(fdiv);
   RegisterInstructionFallback(fdivs);
   RegisterInstructionFallback(fmul);
   RegisterInstruction(fmuls);
   RegisterInstruction(fsub);
   RegisterInstructionFallback(fsubs);
   RegisterInstructionFallback(fres);
   RegisterInstructionFallback(frsqrte);
   RegisterInstructionFallback(fsel);
   RegisterInstructionFallback(fmadd);
   RegisterInstructionFallback(fmadds);
   RegisterInstructionFallback(fmsub);
   RegisterInstructionFallback(fmsubs);
   RegisterInstructionFallback(fnmadd);
   RegisterInstructionFallback(fnmadds);
   RegisterInstructionFallback(fnmsub);
   RegisterInstructionFallback(fnmsubs);
   RegisterInstructionFallback(fctiw);
   RegisterInstructionFallback(fctiwz);
   RegisterInstruction(frsp);
   RegisterInstructionFallback(fabs);
   RegisterInstructionFallback(fnabs);
   RegisterInstructionFallback(fmr);
   RegisterInstructionFallback(fabs);
   RegisterInstructionFallback(fneg);
}

} // namespace jit

} // namespace cpu
