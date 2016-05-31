#include <cassert>
#include <algorithm>
#include "jit_internal.h"
#include "cpu/interpreter/interpreter_insreg.h"
#include "utils/debuglog.h"

#include "cpu/espresso/espresso_instructionset.h"

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

} // namespace cpu

void fallbacksPrint()
{
   typedef std::pair<uint32_t, uint64_t> FallbackItem;
   std::vector<FallbackItem> callList;
   for (auto i = 0u; i < static_cast<size_t>(espresso::InstructionID::InstructionCount); ++i) {
      callList.emplace_back(i, cpu::jit::sFallbackCalls[i]);
   }

   std::sort(callList.begin(), callList.end(), [](const FallbackItem &a, const FallbackItem &b) {
      return b.second < a.second;
   });

   fmt::MemoryWriter out;
   out.write("Fallback Call Numbers:\n");

   for (auto i : callList) {
      auto data = espresso::findInstructionInfo(static_cast<espresso::InstructionID>(i.first));
      out.write("  [{}] {}\n", data->name, i.second);
   }

   debugPrint(out.str());
}
