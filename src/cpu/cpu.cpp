#include <cfenv>
#include <condition_variable>
#include <vector>
#include <functional>
#include <memory>
#include <atomic>
#include "cpu.h"
#include "cpu_internal.h"
#include "interpreter/interpreter.h"
#include "jit/jit.h"
#include "espresso/espresso_instructionset.h"
#include "platform/platform_thread.h"

namespace cpu
{

entrypoint_handler
gCoreEntryPointHandler;

jit_mode
gJitMode = jit_mode::disabled;

Core
gCore[3];

static thread_local cpu::Core *
tCurrentCore = nullptr;

std::thread
gTimerThread;

void initialise()
{
   espresso::initialiseInstructionSet();
   cpu::interpreter::initialise();
   cpu::jit::initialise();
}

void set_jit_mode(jit_mode mode)
{
   gJitMode = mode;
}

void coreEntryPoint(Core *core)
{
   tCurrentCore = core;
   gCoreEntryPointHandler();
}

void start()
{
   for (auto i = 0; i < 3; ++i) {
      auto &core = gCore[i];
      core.id = i;
      core.thread = std::thread(std::bind(&coreEntryPoint, &core));
      core.next_alarm = std::chrono::time_point<std::chrono::system_clock>::max();

      static const std::string coreNames[] = { "Core #0", "Core #1", "Core #2" };
      platform::setThreadName(&core.thread, coreNames[core.id]);
   }

   gTimerThread = std::thread(std::bind(&timerEntryPoint));
   platform::setThreadName(&gTimerThread, "Timer Thread");
}

void halt()
{
   for (auto i = 0; i < 3; ++i) {
      interrupt(i, SRESET_INTERRUPT);
   }

   for (auto i = 0; i < 3; ++i) {
      gCore[i].thread.join();
   }
}

void set_core_entrypoint_handler(entrypoint_handler handler)
{
   gCoreEntryPointHandler = handler;
}

namespace this_core
{

cpu::Core * state()
{
   return tCurrentCore;
}

void resume()
{
   // If we have breakpoints set, we have to fall back to interpreter loop
   // This is because JIT wont check for breakpoints on each instruction.
   // TODO: Make JIT account for breakpoints by inserting breakpoint checks
   // in the appropriate places in the instruction stream.
   if (has_breakpoints()) {
      interpreter::resume(tCurrentCore);
   }

   // Use appropriate jit mode
   if (gJitMode == jit_mode::enabled) {
      jit::resume(tCurrentCore);
   } else {
      interpreter::resume(tCurrentCore);
   }
}

void execute_sub()
{
   auto lr = tCurrentCore->lr;
   tCurrentCore->lr = CALLBACK_ADDR;
   resume();
   tCurrentCore->lr = lr;
}

void
update_rounding_mode()
{
   Core *core = tCurrentCore;

   static const int modes[4] = {
      FE_TONEAREST, FE_TOWARDZERO, FE_UPWARD, FE_DOWNWARD
   };
   fesetround(modes[core->fpscr.rn]);
}

} // namespace this_core

} // namespace cpu
