#include "common/decaf_assert.h"
#include "jit_internal.h"
#include "espresso/espresso_instructionset.h"
#include "interpreter/interpreter_insreg.h"
#include <cassert>
#include <algorithm>
#include <spdlog/fmt/fmt.h>

static const bool
TRACK_FALLBACK_CALLS = true;

namespace cpu
{

namespace jit
{

static uint64_t sFallbackCalls[static_cast<size_t>(espresso::InstructionID::InstructionCount)] = { 0 };

bool jit_fallback(PPCEmuAssembler& a, espresso::Instruction instr)
{
   auto data = espresso::decodeInstruction(instr);
   decaf_assert(data, fmt::format("Failed to decode instruction {:08X}", instr.value));

   auto fptr = cpu::interpreter::getInstructionHandler(data->id);
   decaf_assert(fptr, fmt::format("Unimplemented instruction {}", static_cast<int>(data->id)));

   a.evictAll();

   if (TRACK_FALLBACK_CALLS) {
      auto fallbackAddr = reinterpret_cast<intptr_t>(&sFallbackCalls[static_cast<uint32_t>(data->id)]);
      a.mov(asmjit::x86::rax, asmjit::Ptr(fallbackAddr));
      a.lock().inc(asmjit::X86Mem(asmjit::x86::rax, 0));
   }

   a.mov(a.sysArgReg[0], a.stateReg);
   a.mov(a.sysArgReg[1], (uint32_t)instr);
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
