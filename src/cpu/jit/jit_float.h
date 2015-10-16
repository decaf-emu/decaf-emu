#pragma once
#include "jit.h"

namespace cpu
{
namespace jit
{

   void
   updateFloatConditionRegister(PPCEmuAssembler& a, const asmjit::X86GpReg& tmp, const asmjit::X86GpReg& tmp2);

}
}