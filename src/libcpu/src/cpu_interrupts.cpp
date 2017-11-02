#include "cpu.h"
#include "cpu_breakpoints.h"
#include "cpu_internal.h"

#include <common/decaf_assert.h>
#include <condition_variable>
#include <atomic>

namespace cpu
{

InterruptHandler
gInterruptHandler;

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

void
setInterruptHandler(InterruptHandler handler)
{
   gInterruptHandler = handler;
}

void
interrupt(int core_idx, uint32_t flags)
{
   std::unique_lock<std::mutex> lock { gInterruptMutex };
   gCore[core_idx]->interrupt.fetch_or(flags);
   gInterruptCondition.notify_all();
}

void
timerEntryPoint()
{
   while (gRunning.load()) {
      std::unique_lock<std::mutex> lock{ gTimerMutex };
      auto now = std::chrono::steady_clock::now();
      auto next = std::chrono::steady_clock::time_point::max();
      bool timedWait = false;

      for (auto i = 0; i < 3; ++i) {
         auto core = gCore[i];

         if (core->next_alarm <= now) {
            core->next_alarm = std::chrono::steady_clock::time_point::max();
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

namespace this_core
{

void
clearInterrupt(uint32_t flags)
{
   state()->interrupt.fetch_and(~flags);
}

uint32_t
interruptMask()
{
   return state()->interrupt_mask;
}

uint32_t
setInterruptMask(uint32_t mask)
{
   auto core = state();
   auto old_mask = core->interrupt_mask;
   core->interrupt_mask = mask;
   return old_mask;
}

void
checkInterrupts()
{
   auto core = state();
   auto mask = core->interrupt_mask | NONMASKABLE_INTERRUPTS;
   auto flags = core->interrupt.fetch_and(~mask);

   if (flags & mask) {
      cpu::gInterruptHandler(core, flags);
   }
}

void
waitForInterrupt()
{
   auto core = this_core::state();
   std::unique_lock<std::mutex> lock { gInterruptMutex };

   while (true) {
      if (!(core->interrupt_mask & ~NONMASKABLE_INTERRUPTS)) {
         decaf_abort("WFI thread found all maskable interrupts were disabled");
      }

      auto mask = core->interrupt_mask | NONMASKABLE_INTERRUPTS;
      auto flags = core->interrupt.fetch_and(~mask);

      if (flags & mask) {
         lock.unlock();
         gInterruptHandler(core, flags);
         lock.lock();
      } else {
         gInterruptCondition.wait(lock);
      }
   }
}

void
waitNextInterrupt(std::chrono::steady_clock::time_point until)
{
   auto core = this_core::state();
   std::unique_lock<std::mutex> lock { gInterruptMutex };

   if (!(core->interrupt_mask & ~NONMASKABLE_INTERRUPTS)) {
      decaf_abort("WFI thread found all maskable interrupts were disabled");
   }

   auto mask = core->interrupt_mask | NONMASKABLE_INTERRUPTS;
   auto flags = core->interrupt.fetch_and(~mask);

   if (!(flags & mask)) {
      gInterruptCondition.wait_until(lock, until);

      mask = core->interrupt_mask | NONMASKABLE_INTERRUPTS;
      flags = core->interrupt.fetch_and(~mask);
   }

   lock.unlock();

   if (flags & mask) {
      gInterruptHandler(core, flags);
   }
}

void
setNextAlarm(std::chrono::steady_clock::time_point time)
{
   auto core = this_core::state();
   std::unique_lock<std::mutex> lock { gTimerMutex };
   core->next_alarm = time;
   gTimerCondition.notify_all();
}

} // namespace this_core

} // namespace cpu
