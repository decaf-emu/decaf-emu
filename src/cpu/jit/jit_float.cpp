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
fadd(PPCEmuAssembler& a, Instruction instr)
{
   if (instr.rc) {
      return jit_fallback(a, instr);
   }

   // FPSCR, FPRF supposed to be updated here...

   a.movq(a.xmm0, a.ppcfpr[instr.frA]);
   a.movq(a.xmm1, a.ppcfpr[instr.frB]);

   a.addsd(a.xmm0, a.xmm1);

   a.movq(a.ppcfpr[instr.frD], a.xmm0);

   return true;
}

static bool
fadds(PPCEmuAssembler& a, Instruction instr)
{
   if (instr.rc) {
      return jit_fallback(a, instr);
   }

   // FPSCR, FPRF supposed to be updated here...

   a.movq(a.xmm0, a.ppcfpr[instr.frA]);
   a.movq(a.xmm1, a.ppcfpr[instr.frB]);

   a.addsd(a.xmm0, a.xmm1);

   a.cvtsd2ss(a.xmm1, a.xmm0);
   a.cvtss2sd(a.xmm0, a.xmm1);

   a.movq(a.ppcfpr[instr.frD], a.xmm0);

   return true;
}

static bool
fdiv(PPCEmuAssembler& a, Instruction instr)
{
   if (instr.rc) {
      return jit_fallback(a, instr);
   }

   // FPSCR, FPRF supposed to be updated here...

   a.movq(a.xmm0, a.ppcfpr[instr.frA]);
   a.movq(a.xmm1, a.ppcfpr[instr.frB]);

   a.divsd(a.xmm0, a.xmm1);

   a.movq(a.ppcfpr[instr.frD], a.xmm0);

   return true;
}

static bool
fdivs(PPCEmuAssembler& a, Instruction instr)
{
   if (instr.rc) {
      return jit_fallback(a, instr);
   }

   // FPSCR, FPRF supposed to be updated here...

   a.movq(a.xmm0, a.ppcfpr[instr.frA]);
   a.movq(a.xmm1, a.ppcfpr[instr.frB]);

   a.divsd(a.xmm0, a.xmm1);

   a.cvtsd2ss(a.xmm1, a.xmm0);
   a.cvtss2sd(a.xmm0, a.xmm1);

   a.movq(a.ppcfpr[instr.frD], a.xmm0);

   return true;
}

static bool
fmul(PPCEmuAssembler& a, Instruction instr)
{
   if (instr.rc) {
      return jit_fallback(a, instr);
   }

   // FPSCR, FPRF supposed to be updated here...

   a.movq(a.xmm0, a.ppcfpr[instr.frA]);
   a.movq(a.xmm1, a.ppcfpr[instr.frC]);

   a.mulsd(a.xmm0, a.xmm1);

   a.movq(a.ppcfpr[instr.frD], a.xmm0);

   return true;
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
fsubs(PPCEmuAssembler& a, Instruction instr)
{
   if (instr.rc) {
      return jit_fallback(a, instr);
   }

   // FPSCR, FPRF supposed to be updated here...

   a.movq(a.xmm0, a.ppcfpr[instr.frA]);
   a.movq(a.xmm1, a.ppcfpr[instr.frB]);

   a.subsd(a.xmm0, a.xmm1);

   a.cvtsd2ss(a.xmm1, a.xmm0);
   a.cvtss2sd(a.xmm0, a.xmm1);

   a.movq(a.ppcfpr[instr.frD], a.xmm0);

   return true;
}

static bool
fmadd(PPCEmuAssembler& a, Instruction instr)
{
   if (instr.rc) {
      return jit_fallback(a, instr);
   }

   // FPSCR, FPRF supposed to be updated here...
   a.movq(a.xmm0, a.ppcfpr[instr.frA]);
   a.movq(a.xmm1, a.ppcfpr[instr.frC]);
   a.mulsd(a.xmm0, a.xmm1);

   a.movq(a.xmm1, a.ppcfpr[instr.frB]);
   a.addsd(a.xmm0, a.xmm1);

   a.movq(a.ppcfpr[instr.frD], a.xmm0);

   return true;
}

static bool
fmadds(PPCEmuAssembler& a, Instruction instr)
{
   if (instr.rc) {
      return jit_fallback(a, instr);
   }

   // FPSCR, FPRF supposed to be updated here...

   a.movq(a.xmm0, a.ppcfpr[instr.frA]);
   a.movq(a.xmm1, a.ppcfpr[instr.frC]);
   a.mulsd(a.xmm0, a.xmm1);

   a.movq(a.xmm1, a.ppcfpr[instr.frB]);
   a.addsd(a.xmm0, a.xmm1);

   a.cvtsd2ss(a.xmm1, a.xmm0);
   a.cvtss2sd(a.xmm0, a.xmm1);

   a.movq(a.ppcfpr[instr.frD], a.xmm0);

   return true;
}

static bool
fmsub(PPCEmuAssembler& a, Instruction instr)
{
   if (instr.rc) {
      return jit_fallback(a, instr);
   }

   // FPSCR, FPRF supposed to be updated here...
   a.movq(a.xmm0, a.ppcfpr[instr.frA]);
   a.movq(a.xmm1, a.ppcfpr[instr.frC]);
   a.mulsd(a.xmm0, a.xmm1);

   a.movq(a.xmm1, a.ppcfpr[instr.frB]);
   a.subsd(a.xmm0, a.xmm1);

   a.movq(a.ppcfpr[instr.frD], a.xmm0);

   return true;
}

static bool
fmsubs(PPCEmuAssembler& a, Instruction instr)
{
   if (instr.rc) {
      return jit_fallback(a, instr);
   }

   // FPSCR, FPRF supposed to be updated here...

   a.movq(a.xmm0, a.ppcfpr[instr.frA]);
   a.movq(a.xmm1, a.ppcfpr[instr.frC]);
   a.mulsd(a.xmm0, a.xmm1);

   a.movq(a.xmm1, a.ppcfpr[instr.frB]);
   a.subsd(a.xmm0, a.xmm1);

   a.cvtsd2ss(a.xmm1, a.xmm0);
   a.cvtss2sd(a.xmm0, a.xmm1);

   a.movq(a.ppcfpr[instr.frD], a.xmm0);

   return true;
}

static bool
fnmadd(PPCEmuAssembler& a, Instruction instr)
{
   if (instr.rc) {
      return jit_fallback(a, instr);
   }

   // FPSCR, FPRF supposed to be updated here...
   a.movq(a.xmm0, a.ppcfpr[instr.frA]);
   a.movq(a.xmm1, a.ppcfpr[instr.frC]);
   a.mulsd(a.xmm0, a.xmm1);

   a.movq(a.xmm1, a.ppcfpr[instr.frB]);
   a.addsd(a.xmm0, a.xmm1);

   a.mov(a.zax, 0x8000000000000000);
   a.movq(a.xmm1, a.zax);
   a.pxor(a.xmm0, a.xmm1);

   a.movq(a.ppcfpr[instr.frD], a.xmm0);

   return true;
}

static bool
fnmadds(PPCEmuAssembler& a, Instruction instr)
{
   if (instr.rc) {
      return jit_fallback(a, instr);
   }

   // FPSCR, FPRF supposed to be updated here...

   a.movq(a.xmm0, a.ppcfpr[instr.frA]);
   a.movq(a.xmm1, a.ppcfpr[instr.frC]);
   a.mulsd(a.xmm0, a.xmm1);

   a.movq(a.xmm1, a.ppcfpr[instr.frB]);
   a.addsd(a.xmm0, a.xmm1);

   a.mov(a.zax, 0x8000000000000000);
   a.movq(a.xmm1, a.zax);
   a.pxor(a.xmm0, a.xmm1);

   a.cvtsd2ss(a.xmm1, a.xmm0);
   a.cvtss2sd(a.xmm0, a.xmm1);

   a.movq(a.ppcfpr[instr.frD], a.xmm0);

   return true;
}

static bool
fnmsub(PPCEmuAssembler& a, Instruction instr)
{
   if (instr.rc) {
      return jit_fallback(a, instr);
   }

   // FPSCR, FPRF supposed to be updated here...
   a.movq(a.xmm0, a.ppcfpr[instr.frA]);
   a.movq(a.xmm1, a.ppcfpr[instr.frC]);
   a.mulsd(a.xmm0, a.xmm1);

   a.movq(a.xmm1, a.ppcfpr[instr.frB]);
   a.subsd(a.xmm0, a.xmm1);

   a.mov(a.zax, 0x8000000000000000);
   a.movq(a.xmm1, a.zax);
   a.pxor(a.xmm0, a.xmm1);

   a.movq(a.ppcfpr[instr.frD], a.xmm0);

   return true;
}

static bool
fnmsubs(PPCEmuAssembler& a, Instruction instr)
{
   if (instr.rc) {
      return jit_fallback(a, instr);
   }

   // FPSCR, FPRF supposed to be updated here...

   a.movq(a.xmm0, a.ppcfpr[instr.frA]);
   a.movq(a.xmm1, a.ppcfpr[instr.frC]);
   a.mulsd(a.xmm0, a.xmm1);

   a.movq(a.xmm1, a.ppcfpr[instr.frB]);
   a.subsd(a.xmm0, a.xmm1);

   a.mov(a.zax, 0x8000000000000000);
   a.movq(a.xmm1, a.zax);
   a.pxor(a.xmm0, a.xmm1);

   a.cvtsd2ss(a.xmm1, a.xmm0);
   a.cvtss2sd(a.xmm0, a.xmm1);

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

static bool
fabs(PPCEmuAssembler& a, Instruction instr)
{
   if (instr.rc) {
      return jit_fallback(a, instr);
   }

   // FPSCR, FPRF supposed to be updated here...

   a.movq(a.xmm0, a.ppcfpr[instr.frB]);

   a.mov(a.zax, 0x7FFFFFFFFFFFFFFF);
   a.movq(a.xmm1, a.zax);
   a.pand(a.xmm0, a.xmm1);

   a.movq(a.ppcfpr[instr.frD], a.xmm0);

   return true;
}

static bool
fnabs(PPCEmuAssembler& a, Instruction instr)
{
   if (instr.rc) {
      return jit_fallback(a, instr);
   }

   // FPSCR, FPRF supposed to be updated here...

   a.movq(a.xmm0, a.ppcfpr[instr.frB]);

   a.mov(a.zax, 0x7FFFFFFFFFFFFFFF);
   a.movq(a.xmm1, a.zax);
   a.pand(a.xmm0, a.xmm1);

   a.mov(a.zax, 0x8000000000000000);
   a.movq(a.xmm1, a.zax);
   a.pxor(a.xmm0, a.xmm1);

   a.movq(a.ppcfpr[instr.frD], a.xmm0);

   return true;
}

static bool
fmr(PPCEmuAssembler& a, Instruction instr)
{
   if (instr.rc) {
      return jit_fallback(a, instr);
   }

   // FPSCR, FPRF supposed to be updated here...

   a.movq(a.xmm0, a.ppcfpr[instr.frB]);
   a.movq(a.ppcfpr[instr.frD], a.xmm0);

   return true;
}

static bool
fneg(PPCEmuAssembler& a, Instruction instr)
{
   if (instr.rc) {
      return jit_fallback(a, instr);
   }

   // FPSCR, FPRF supposed to be updated here...

   a.movq(a.xmm0, a.ppcfpr[instr.frB]);

   a.mov(a.zax, 0x8000000000000000);
   a.movq(a.xmm1, a.zax);
   a.pxor(a.xmm0, a.xmm1);

   a.movq(a.ppcfpr[instr.frD], a.xmm0);

   return true;
}

void registerFloatInstructions()
{
   // TODO: fmXXX instructions are CLOSE, but not perfectly
   //   accurate...

   RegisterInstruction(fadd);
   RegisterInstruction(fadds);
   RegisterInstruction(fdiv);
   RegisterInstruction(fdivs);
   RegisterInstruction(fmul);
   RegisterInstruction(fmuls);
   RegisterInstruction(fsub);
   RegisterInstruction(fsubs);
   RegisterInstructionFallback(fres);
   RegisterInstructionFallback(frsqrte);
   RegisterInstructionFallback(fsel);
   RegisterInstruction(fmadd);
   RegisterInstruction(fmadds);
   RegisterInstruction(fmsub);
   RegisterInstruction(fmsubs);
   RegisterInstruction(fnmadd);
   RegisterInstruction(fnmadds);
   RegisterInstruction(fnmsub);
   RegisterInstruction(fnmsubs);
   RegisterInstructionFallback(fctiw);
   RegisterInstructionFallback(fctiwz);
   RegisterInstruction(frsp);
   RegisterInstruction(fabs);
   RegisterInstruction(fnabs);
   RegisterInstruction(fmr);
   RegisterInstruction(fneg);
}

} // namespace jit

} // namespace cpu
