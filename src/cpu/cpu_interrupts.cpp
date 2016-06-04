#include "cpu.h"
#include "cpu_internal.h"
#include <condition_variable>
#include <atomic>

namespace cpu
{

extern Core
gCore[3];

interrupt_handler
gInterruptHandler;

std::mutex
gInterruptMutex;

std::condition_variable
gInterruptCondition;

std::mutex
gTimerMutex;

std::condition_variable
gTimerCondition;

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

namespace this_core {

void clear_interrupt(uint32_t flags)
{
   state()->interrupt.fetch_and(~flags);
}

uint32_t interrupt_mask()
{
   return state()->interrupt_mask;
}

uint32_t set_interrupt_mask(uint32_t mask)
{
   Core *core = state();
   uint32_t old_mask = core->interrupt_mask;
   core->interrupt_mask = mask;
   return old_mask;
}

void check_interrupts()
{
   cpu::Core *core = state();

   uint32_t interrupt_mask = core->interrupt_mask | NONMASKABLE_INTERRUPTS;
   uint32_t interrupt_flags = core->interrupt.fetch_and(~interrupt_mask);

   // Check if we hit any breakpoints
   if (pop_breakpoint(core->nia)) {
      // Need to interrupt all the other cores
      for (auto i = 0; i < 3; ++i) {
         if (i != core->id) {
            interrupt(i, DBGBREAK_INTERRUPT);
         }
      }

      // Our core we can just add it to the local interrupt list
      interrupt_flags |= DBGBREAK_INTERRUPT;
   }

   if (interrupt_flags & interrupt_mask) {
      cpu::gInterruptHandler(interrupt_flags);
   }
}

void wait_for_interrupt()
{
   cpu::Core *core = this_core::state();
   std::unique_lock<std::mutex> lock{ gInterruptMutex };
   while (true) {
      if (!(core->interrupt_mask & ~NONMASKABLE_INTERRUPTS)) {
         throw std::logic_error("WFI thread found all maskable interrupts were disabled");
      }

      uint32_t interrupt_mask = core->interrupt_mask | NONMASKABLE_INTERRUPTS;
      uint32_t interrupt_flags = core->interrupt.fetch_and(~interrupt_mask);
      if (interrupt_flags & interrupt_mask) {
         lock.unlock();
         gInterruptHandler(interrupt_flags);
         lock.lock();
      } else {
         gInterruptCondition.wait(lock);
      }
   }
}

void set_next_alarm(std::chrono::time_point<std::chrono::system_clock> alarm_time)
{
   cpu::Core *core = this_core::state();
   std::unique_lock<std::mutex> lock{ gTimerMutex };
   core->next_alarm = alarm_time;
   gTimerCondition.notify_all();
}

} // namespace this_core

} // namespace cpu
