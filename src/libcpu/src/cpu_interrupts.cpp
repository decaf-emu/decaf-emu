#include "cpu.h"
#include "cpu_breakpoints.h"
#include "cpu_internal.h"

#include <common/decaf_assert.h>
#include <condition_variable>
#include <atomic>

namespace cpu
{

static void defaultInterruptHandler(Core *core, uint32_t interrupt_flags) { }

static InterruptHandler sUserInterruptHandler = &defaultInterruptHandler;
static std::mutex sInterruptMutex;
static std::condition_variable sInterruptCondition;

void
interrupt(int coreIndex, uint32_t flags)
{
   std::unique_lock<std::mutex> lock { sInterruptMutex };
   auto core = getCore(coreIndex);
   if (core) {
      core->interrupt.fetch_or(flags);
   }
   sInterruptCondition.notify_all();
}

void
setInterruptHandler(InterruptHandler handler)
{
   sUserInterruptHandler = handler;
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
      sUserInterruptHandler(core, flags);
   }
}

void
waitForInterrupt()
{
   auto core = this_core::state();
   std::unique_lock<std::mutex> lock { sInterruptMutex };

   while (true) {
      if (!(core->interrupt_mask & ~NONMASKABLE_INTERRUPTS)) {
         decaf_abort("WFI thread found all maskable interrupts were disabled");
      }

      auto mask = core->interrupt_mask | NONMASKABLE_INTERRUPTS;
      auto flags = core->interrupt.fetch_and(~mask);

      if (flags & mask) {
         lock.unlock();
         sUserInterruptHandler(core, flags);
         lock.lock();
      } else {
         sInterruptCondition.wait(lock);
      }
   }
}

void
waitNextInterrupt(std::chrono::steady_clock::time_point until)
{
   auto core = this_core::state();
   std::unique_lock<std::mutex> lock { sInterruptMutex };

   if (!(core->interrupt_mask & ~NONMASKABLE_INTERRUPTS)) {
      decaf_abort("WFI thread found all maskable interrupts were disabled");
   }

   auto mask = core->interrupt_mask | NONMASKABLE_INTERRUPTS;
   auto flags = core->interrupt.fetch_and(~mask);

   if (!(flags & mask)) {
      if (until == std::chrono::steady_clock::time_point { }) {
         sInterruptCondition.wait(lock);
      } else {
         sInterruptCondition.wait_until(lock, until);
      }

      mask = core->interrupt_mask | NONMASKABLE_INTERRUPTS;
      flags = core->interrupt.fetch_and(~mask);
   }

   lock.unlock();

   if (flags & mask) {
      sUserInterruptHandler(core, flags);
   }
}

} // namespace this_core

} // namespace cpu
