#include "common/debuglog.h"
#include "jit_internal.h"
#include "espresso/espresso_instructionset.h"
#include "interpreter/interpreter_insreg.h"
#include <cassert>
#include <algorithm>

static const bool TRACK_FALLBACK_CALLS = true;

namespace cpu
{

namespace jit
{

static uint64_t sFallbackCalls[static_cast<size_t>(espresso::InstructionID::InstructionCount)] = { 0 };

bool jit_fallback(PPCEmuAssembler& a, espresso::Instruction instr)
{
   auto data = espresso::decodeInstruction(instr);
   auto fptr = cpu::interpreter::getInstructionHandler(data->id);
   if (!fptr) {
      throw;
   }

   if (TRACK_FALLBACK_CALLS) {
      a.mov(a.zax, asmjit::Ptr(reinterpret_cast<intptr_t>(&sFallbackCalls[static_cast<uint32_t>(data->id)])));
      a.lock();
      a.inc(asmjit::X86Mem(a.zax, 0));
   }

   a.mov(a.zcx, a.state);
   a.mov(a.edx, (uint32_t)instr);
   a.call(asmjit::Ptr(fptr));

   return true;
}

} // namespace jit

uint64_t *
getJitFallbackStats()
{
   return jit::sFallbackCalls;
}

} // namespace cpu
