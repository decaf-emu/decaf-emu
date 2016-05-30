#include "kernel.h"
#include <algorithm>
#include <cfenv>
#include "platform/platform_fiber.h"
#include "platform/platform_thread.h"
#include "modules/coreinit/coreinit.h"
#include "modules/coreinit/coreinit_core.h"
#include "modules/coreinit/coreinit_memheap.h"
#include "modules/coreinit/coreinit_scheduler.h"
#include "modules/coreinit/coreinit_systeminfo.h"

namespace kernel
{

static coreinit::OSThread *tCurrentThread[3];
static coreinit::OSThread *tDeadThread[3];
static platform::Fiber *tIdleFiber[3];

static std::vector<Fiber *> gFiberQueue;

struct Fiber
{
   platform::Fiber *handle = nullptr;
   coreinit::OSThread *thread = nullptr;
};

coreinit::OSThread * getCurrentThread() {
   return tCurrentThread[cpu::this_core::id()];
}

// This must be called under the same scheduler lock
// that added the thread to tDeadThread, we simply use
// the thread_local to pass it between fibers.
void checkDeadThread()
{
   auto core_id = cpu::this_core::id();
   auto deadThread = tDeadThread[core_id];
   if (deadThread) {
      tDeadThread[core_id] = nullptr;

      // Something is broken if we have no fiber
      assert(deadThread->fiber);

      // Destroy the fiber
      auto fiber = deadThread->fiber;
      platform::destroyFiber(fiber->handle);
   }
}

void fiberEntryPoint(void*)
{
   checkDeadThread();

   // Our scheduler will have been locked by whoever
   // scheduled this fiber.
   coreinit::internal::unlockScheduler();

   auto core = cpu::this_core::state();
   auto thread = getCurrentThread();
   core->state.cia = 0;
   core->state.nia = thread->entryPoint.getAddress();

   cpu::this_core::execute_sub();

   coreinit::OSExitThread(ppctypes::getResult<int>(&core->state));
}

void init_core_fiber()
{
   // Grab the currently running core state.
   auto core_id = cpu::this_core::id();

   // Grab the system fiber
   auto fiber = platform::getThreadFiber();

   // Save some needed information about the fiber run states.
   tIdleFiber[core_id] = fiber;
   tCurrentThread[core_id] = nullptr;
   tDeadThread[core_id] = nullptr;
}

// TODO: this should probably go away once coreinit handles schedulering
void
exitThreadNoLock()
{
   coreinit::OSThread *thread = getCurrentThread();
   
   // Remove the threads fiber from the queue
   gFiberQueue.erase(std::remove(gFiberQueue.begin(), gFiberQueue.end(), thread->fiber), gFiberQueue.end());

   // Mark this fiber to be cleaned up
   tDeadThread[cpu::this_core::id()] = thread;

   // Reschedule to another thread, this will never return.
   rescheduleNoLock(false);
}

Fiber *
peekNextFiberNoLock(uint32_t core)
{
   auto bit = 1 << core;

   for (auto fiber : gFiberQueue) {
      if (fiber->thread->state != OSThreadState::Ready) {
         continue;
      }

      if (fiber->thread->suspendCounter > 0) {
         continue;
      }

      if (fiber->thread->attr & bit) {
         return fiber;
      }
   }

   return nullptr;
}

void
queueThreadNoLock(coreinit::OSThread *thread)
{
   auto fiber = thread->fiber;
   
   // Initialise this if its the first time!
   if (fiber == nullptr) {
      fiber = new Fiber();
      fiber->handle = platform::createFiber(fiberEntryPoint, nullptr);
      fiber->thread = thread;
      thread->fiber = fiber;
   }

   auto compare =
      [](Fiber *lhs, Fiber *rhs) {
      return lhs->thread->basePriority < rhs->thread->basePriority;
   };

   gLog->trace("Core {} queued thread {}", cpu::this_core::id(), fiber->thread->id);

   // Add this fiber to the fiber queue
   auto pos = std::upper_bound(gFiberQueue.begin(), gFiberQueue.end(), fiber, compare);
   gFiberQueue.insert(pos, fiber);

   // Check if a core is idling and has something to run, if so, interrupt it.
   for (auto i = 0; i < 3; ++i) {
      if (!tCurrentThread[i] && peekNextFiberNoLock(i)) {
         cpu::interrupt(i, cpu::GENERIC_INTERRUPT);
      }
   }
}

void saveContext(coreinit::OSContext *context)
{
   auto &state = cpu::this_core::state()->state;
   for (auto i = 0; i < 32; ++i) {
      context->gpr[i] = state.gpr[i];
   }
   for (auto i = 0; i < 32; ++i) {
      context->fpr[i] = state.fpr[i].value;
      context->psf[i] = state.fpr[i].paired1;
   }
   for (auto i = 0; i < 8; ++i) {
      context->gqr[i] = state.gqr[i].value;
   }
   context->cr = state.cr.value;
   context->lr = state.lr;
   context->ctr = state.ctr;
   context->xer = state.xer.value;
   //context->srr0 = state.sr[0];
   //context->srr1 = state.sr[1];
   context->fpscr = state.fpscr.value;
}

void restoreContext(coreinit::OSContext *context)
{
   auto &state = cpu::this_core::state()->state;
   for (auto i = 0; i < 32; ++i) {
      state.gpr[i] = context->gpr[i];
   }
   for (auto i = 0; i < 32; ++i) {
      state.fpr[i].value = context->fpr[i];
      state.fpr[i].paired1 = context->psf[i];
   }
   for (auto i = 0; i < 8; ++i) {
      state.gqr[i].value = context->gqr[i];
   }
   state.cr.value = context->cr;
   state.lr = context->lr;
   state.ctr = context->ctr;
   state.xer.value = context->xer;
   //state.sr[0] = context->srr0;
   //state.sr[1] = context->srr1;
   state.fpscr.value = context->fpscr;
}

void rescheduleNoLock(bool yielding)
{
   auto core_id = cpu::this_core::id();
   auto thread = getCurrentThread();

   auto next = peekNextFiberNoLock(core_id);

   // Priority is 0 = highest, 31 = lowest
   if (thread && thread->suspendCounter <= 0 && thread->state == OSThreadState::Running) {
      if (!next) {
         // There is no thread to reschedule to
         return;
      }

      if (yielding) {
         // Yield will transfer control to threads with equal or better priority
         if (thread->basePriority < next->thread->basePriority) {
            return;
         }
      }
      else {
         // Only reschedule to more important threads
         if (thread->basePriority <= next->thread->basePriority) {
            return;
         }
      }
   }

   // Change state to ready, only if this thread is running
   if (thread && thread->state == OSThreadState::Running) {
      thread->state = OSThreadState::Ready;
   }

   const char *threadName = "?";
   const char *nextName = "?";
   if (thread && thread->name) {
      threadName = thread->name;
   }
   if (next && next->thread->name) {
      nextName = next->thread->name;
   }
   if (thread) {
      if (next) {
         gLog->trace("Core {} leaving thread {}[{}] to thread {}[{}]", core_id, thread->id, threadName, next->thread->id, nextName);
      } else {
         gLog->trace("Core {} leaving thread {}[{}] to idle", core_id, thread->id, threadName);
      }
   } else {
      if (next) {
         gLog->trace("Core {} leaving idle to thread {}[{}]", core_id, next->thread->id, nextName);
      } else {
         gLog->trace("Core {} leaving idle to idle", core_id);
      }
   }

   // Save our CIA for when we come back.
   auto core = cpu::this_core::state();
   auto cia = core->state.cia;
   auto nia = core->state.nia;

   // If we have a current context, save it
   if (thread) {
      saveContext(&thread->context);
   }

   // Switch to the new thread
   if (next) {
      next->thread->state = OSThreadState::Running;

      tCurrentThread[core_id] = next->thread;
      restoreContext(&next->thread->context);
      platform::swapToFiber(nullptr, next->handle);
   } else {
      tCurrentThread[core_id] = nullptr;
      platform::swapToFiber(nullptr, tIdleFiber[core_id]);
   }

   checkDeadThread();

   core->state.cia = cia;
   core->state.nia = nia;
}

}

/**
* Our cheeky hack to get user thread ID into spdlog output
*/
namespace spdlog
{
namespace details
{
namespace os
{
size_t thread_id()
{
   size_t coreID = 0xFF, threadID = 0XFF;

   auto core = cpu::this_core::state();
   if (core) {
      coreID = core->id;
      auto thread = kernel::getCurrentThread();
      if (thread) {
         threadID = thread->id;
      }
   }

   return (coreID << 16) | threadID;
}
}
}
}
