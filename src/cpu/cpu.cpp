#include <vector>
#include <cfenv>
#include "cpu.h"
#include "cpu_internal.h"
#include "interpreter/interpreter.h"
#include "jit/jit.h"
#include "espresso/espresso_instructionset.h"

namespace cpu
{

interrupt_handler
gInterruptHandler;

JitMode
gJitMode = JitMode::Disabled;

static CoreState
gDefaultCoreState;

void setJitMode(JitMode mode)
{
   gJitMode = mode;
}

void initialise()
{
   espresso::initialiseInstructionSet();
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

void
setRoundingMode(ThreadState *state)
{
   static const int modes[4] = {
      FE_TONEAREST, FE_TOWARDZERO, FE_UPWARD, FE_DOWNWARD
   };
   fesetround(modes[state->fpscr.rn]);
}

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

} // namespace cpu
