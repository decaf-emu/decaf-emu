#include <cassert>
#include "jit_internal.h"
#include "cpu/instructiondata.h"
#include "cpu/interpreter/interpreter_insreg.h"

namespace cpu
{

namespace jit
{

bool jit_fallback(PPCEmuAssembler& a, Instruction instr)
{
   auto data = gInstructionTable.decode(instr);
   auto fptr = ::cpu::interpreter::getInstructionHandler(data->id);
   if (!fptr) {
      assert(0);
   }

   //printf("JIT Fallback for `%s`\n", data->name);

   a.mov(a.zcx, a.state);
   a.mov(a.edx, (uint32_t)instr);
   a.call(asmjit::Ptr(fptr));

   return true;
}

} // namespace jit

} // namespace cpu
