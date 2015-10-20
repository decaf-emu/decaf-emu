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

void registerFloatInstructions()
{
   RegisterInstructionFallback(fadd);
   RegisterInstructionFallback(fadds);
   RegisterInstructionFallback(fdiv);
   RegisterInstructionFallback(fdivs);
   RegisterInstructionFallback(fmul);
   RegisterInstructionFallback(fmuls);
   RegisterInstructionFallback(fsub);
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
   RegisterInstructionFallback(frsp);
   RegisterInstructionFallback(fabs);
   RegisterInstructionFallback(fnabs);
   RegisterInstructionFallback(fmr);
   RegisterInstructionFallback(fabs);
   RegisterInstructionFallback(fneg);
}

} // namespace jit

} // namespace cpu
