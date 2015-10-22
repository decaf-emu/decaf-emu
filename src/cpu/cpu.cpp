#include <vector>
#include "cpu.h"
#include "cpu_internal.h"
#include "interpreter/interpreter.h"
#include "jit/jit.h"
#include "instructiondata.h"

namespace cpu
{

interrupt_handler gInterruptHandler;

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

void set_interrupt_handler(interrupt_handler handler)
{
   gInterruptHandler = handler;
}

void interrupt(CoreState *core)
{
   core->interrupt.exchange(true);
}

bool hasInterrupt(CoreState *core)
{
   return core->interrupt.load();
}

void clearInterrupt(CoreState *core)
{
   core->interrupt.exchange(false);
}

static CoreState gDefaultCoreState;

void executeSub(CoreState *core, ThreadState *state)
{
   if (!core) {
      core = &gDefaultCoreState;
   }

   state->core = core;

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