#include <cfenv>
#include <condition_variable>
#include <vector>
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

interrupt_handler
gInterruptHandler;

jit_mode
gJitMode = jit_mode::disabled;

static Core
gCore[3];

static thread_local cpu::Core *
tCurrentCore = nullptr;

std::mutex
gInterruptMutex;

std::condition_variable
gInterruptCondition;

std::mutex
gTimerMutex;

std::condition_variable
gTimerCondition;

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

void
timerEntryPoint()
{
   while (true) {
      std::unique_lock<std::mutex> lock{ gTimerMutex };
      auto now = std::chrono::system_clock::now();
      auto next = std::chrono::time_point<std::chrono::system_clock>::max();
      bool timedWait = false;

      for (auto i = 0; i < 3; ++i) {
         auto core = &gCore[i];

         if (core->next_alarm <= now) {
            core->next_alarm = std::chrono::time_point<std::chrono::system_clock>::max();
            cpu::interrupt(i, ALARM_INTERRUPT);
         } else if (core->next_alarm < next) {
            next = core->next_alarm;
            timedWait = true;
         }
      }

      if (timedWait) {
         gTimerCondition.wait_until(lock, next);
      } else {
         gTimerCondition.wait(lock);
      }
   }
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

namespace this_core
{

cpu::Core * state()
{
   return tCurrentCore;
}

void resume()
{
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

bool isInterruptsEnabled()
{
   return tCurrentCore->interruptEnabled.load(std::memory_order_acquire);
}

bool enableInterrupts()
{
   return tCurrentCore->interruptEnabled.exchange(true);
}

bool disableInterrupts()
{
   return tCurrentCore->interruptEnabled.exchange(false);
}

void wait_for_interrupt()
{
   cpu::Core *core = tCurrentCore;
   std::unique_lock<std::mutex> lock{ gInterruptMutex };
   while (true) {
      if (!core->interruptEnabled.load()) {
         throw std::logic_error("We ended up in the wfi thread while interrupts were disabled");
      }

      if (core->interrupt.load()) {
         uint32_t flags = core->interrupt.exchange(0);
         lock.unlock();
         gInterruptHandler(flags);
         lock.lock();
      } else {
         gInterruptCondition.wait(lock);
      }
   }
}

void set_next_alarm(std::chrono::time_point<std::chrono::system_clock> alarm_time)
{
   std::unique_lock<std::mutex> lock{ gTimerMutex };
   tCurrentCore->next_alarm = alarm_time;
   gTimerCondition.notify_all();
}

}

void set_core_entrypoint_handler(entrypoint_handler handler)
{
   gCoreEntryPointHandler = handler;
}

void set_interrupt_handler(interrupt_handler handler)
{
   gInterruptHandler = handler;
}

void interrupt(int core_idx, uint32_t flags)
{
   std::unique_lock<std::mutex> lock{ gInterruptMutex };
   gCore[core_idx].interrupt.fetch_or(flags);
   gInterruptCondition.notify_all();
}

void
update_rounding_mode(Core *state)
{
   static const int modes[4] = {
      FE_TONEAREST, FE_TOWARDZERO, FE_UPWARD, FE_DOWNWARD
   };
   fesetround(modes[state->fpscr.rn]);
}

} // namespace cpu
