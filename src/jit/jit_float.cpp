#include "bitutils.h"
#include "jit.h"
#include "jit_float.h"

void
updateFloatConditionRegister(PPCEmuAssembler& a, const asmjit::X86GpReg& tmp, const asmjit::X86GpReg& tmp2)
{
   //state->cr.cr1 = state->fpscr.cr1;
   assert(0);
}

void
JitManager::registerFloatInstructions()
{
}