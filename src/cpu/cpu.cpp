#include <vector>
#include "cpu.h"
#include "interpreter/interpreter.h"
#include "jit/jit.h"
#include "instructiondata.h"

namespace cpu
{

JitMode gJitMode = JitMode::Disabled;
static std::vector<KernelCallEntry> sKernelCalls;

void setJitMode(JitMode mode)
{
   gJitMode = mode;
}

void initialise()
{
   gInstructionTable.initialise();
   cpu::interpreter::initialise();
   cpu::jit::initialise();
}

void executeSub(ThreadState *state)
{
   if (gJitMode == JitMode::Enabled) {
      jit::executeSub(state);
   } else {
      interpreter::executeSub(state);
   }
}

uint32_t registerKernelCall(KernelCallEntry &entry)
{
   sKernelCalls.push_back(entry);
   return static_cast<uint32_t>(sKernelCalls.size() - 1);
}

KernelCallEntry * getKernelCall(uint32_t id)
{
   if (id >= sKernelCalls.size()) {
      return nullptr;
   }
   return &sKernelCalls[id];
}

}