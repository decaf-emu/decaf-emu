#include <algorithm>
#include "interpreter.h"
#include "processor.h"
#include "ppc.h"
#include "modules/coreinit/coreinit_thread.h"
#include "modules/coreinit/coreinit_scheduler.h"

Processor
gProcessor { 3 };

__declspec(thread) Core *
tCurrentCore = nullptr;

void
Fiber::fiberEntryPoint(void *param)
{
   gProcessor.fiberEntryPoint(reinterpret_cast<Fiber*>(param));
}

Processor::Processor(size_t cores)
{
   for (auto i = 0u; i < cores; ++i) {
      mCores.push_back(new Core { i });
   }
}

void
Processor::start()
{
   mRunning = true;

   for (auto core : mCores) {
      core->thread = std::thread(std::bind(&Processor::coreEntryPoint, this, core));
   }
}

void
Processor::join()
{
   for (auto core : mCores) {
      core->thread.join();
   }
}

Fiber *
Processor::peekNextFiber(uint32_t core)
{
   auto bit = 1 << core;

   for (auto fiber : mFiberQueue) {
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
Processor::coreEntryPoint(Core *core)
{
   tCurrentCore = core;
   core->primaryFiber = ConvertThreadToFiber(NULL);

   while (mRunning) {
      std::unique_lock<std::mutex> lock(mMutex);

      if (auto fiber = peekNextFiber(core->id)) {
         // Remove fiber from schedule queue
         mFiberQueue.erase(std::remove(mFiberQueue.begin(), mFiberQueue.end(), fiber), mFiberQueue.end());

         // Switch to fiber
         core->currentFiber = fiber;
         fiber->coreID = core->id;
         fiber->parentFiber = core->primaryFiber;
         fiber->thread->state = OSThreadState::Running;
         lock.unlock();
         SwitchToFiber(fiber->handle);
      } else {
         // Wait for a valid fiber
         mCondition.wait(lock);
      }
   }
}

void
Processor::fiberEntryPoint(Fiber *fiber)
{
   gInterpreter.execute(&fiber->state, fiber->state.cia);
   OSExitThread(fiber->state.gpr[3]);
}

void
Processor::exit()
{
   // Return to parent fiber
   auto fiber = tCurrentCore->currentFiber;
   SwitchToFiber(fiber->parentFiber);
}

void
Processor::queue(Fiber *fiber)
{
   std::lock_guard<std::mutex> lock(mMutex);

   auto compare =
      [](Fiber *lhs, Fiber *rhs) {
         return lhs->thread->basePriority < rhs->thread->basePriority;
      };

   auto pos = std::upper_bound(mFiberQueue.begin(), mFiberQueue.end(), fiber, compare);
   mFiberQueue.insert(pos, fiber);
   mCondition.notify_all();
}

void
Processor::reschedule(bool hasSchedulerLock, bool yield)
{
   auto core = tCurrentCore;

   if (!tCurrentCore) {
      // Run from host thread
      return;
   }

   auto fiber = tCurrentCore->currentFiber;
   auto next = peekNextFiber(core->id);
   auto thread = fiber->thread;
   bool reschedule = true;

   // Priority is 0 = highest, 31 = lowest
   if (thread->suspendCounter <= 0 && thread->state == OSThreadState::Running) {
      if (!next) {
         // There is no thread to reschedule to
         return;
      }

      if (yield) {
         // Yield will transfer control to threads with equal or better priority
         if (thread->basePriority < next->thread->basePriority) {
            return;
         }
      } else {
         // Only reschedule to more important threads
         if (thread->basePriority <= next->thread->basePriority) {
            return;
         }
      }
   }

   // Return to main scheduler fiber
   queue(fiber);

   if (hasSchedulerLock) {
      OSUnlockScheduler();
   }

   SwitchToFiber(core->primaryFiber);

   if (hasSchedulerLock) {
      OSLockScheduler();
   }
}

void
Processor::yield()
{
   reschedule(false, true);
}

Fiber *
Processor::createFiber()
{
   auto fiber = new Fiber();
   mFiberList.push_back(fiber);
   return fiber;
}

Fiber *
Processor::getCurrentFiber()
{
   return tCurrentCore->currentFiber;
}

uint32_t
Processor::getCoreID()
{
   return tCurrentCore->id;
}

uint32_t
Processor::getCoreCount()
{
   return static_cast<uint32_t>(mCores.size());
}

namespace spdlog
{
namespace details
{
namespace os
{
size_t thread_id()
{
   size_t coreID = 0, threadID = 0;

   if (tCurrentCore) {
      coreID = tCurrentCore->id;

      if (tCurrentCore->currentFiber) {
         threadID = tCurrentCore->currentFiber->thread->id;
      }
   }

   return (coreID << 8) | threadID;
}
}
}
}
