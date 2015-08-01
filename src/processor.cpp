#include <algorithm>
#include "interpreter.h"
#include "log.h"
#include "processor.h"
#include "ppc.h"
#include "modules/coreinit/coreinit_core.h"
#include "modules/coreinit/coreinit_thread.h"
#include "modules/coreinit/coreinit_scheduler.h"
#include "ppcinvoke.h"

Processor
gProcessor { CoreCount };

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

   mTimerThread = std::thread(std::bind(&Processor::timerEntryPoint, this));
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
Processor::timerEntryPoint()
{
   while (mRunning) {
      std::unique_lock<std::mutex> lock(mTimerMutex);
      auto now = std::chrono::system_clock::now();
      auto next = std::chrono::time_point<std::chrono::system_clock>::max();
      bool timedWait = false;

      for (auto core : mCores) {
         if (core->nextInterrupt <= now) {
            core->interrupt = true;
            core->nextInterrupt = std::chrono::time_point<std::chrono::system_clock>::max();
         } else if (core->nextInterrupt < next) {
            next = core->nextInterrupt;
            timedWait = true;
         }
      }

      if (timedWait) {
         mTimerCondition.wait_until(lock, next);
      } else {
         mTimerCondition.wait(lock);
      }
   }
}

void
Processor::waitFirstInterrupt()
{
   auto core = tCurrentCore;
   auto fiber = core->currentFiber;
   core->interruptHandlerFiber = fiber;
   SwitchToFiber(core->primaryFiber);
}

void
Processor::handleInterrupt()
{
   auto core = tCurrentCore;

   if (core->interrupt) {
      if (core->currentFiber) {
         core->interruptedFiber = core->currentFiber;
      } else {
         core->interruptedFiber = nullptr;
      }

      core->interrupt = false;
      core->currentFiber = core->interruptHandlerFiber;
      SwitchToFiber(core->currentFiber->handle);
   }
}

void
Processor::finishInterrupt()
{
   auto core = tCurrentCore;
   auto fiber = core->interruptedFiber;

   core->currentFiber = fiber;
   core->interruptedFiber = nullptr;
   gLog->trace("Exit interrupt core {}", core->id);

   if (!fiber) {
      SwitchToFiber(core->primaryFiber);
   } else {
      SwitchToFiber(fiber->handle);
   }
}

void
Processor::setInterrupt(uint32_t core)
{
   std::unique_lock<std::mutex> lock(mTimerMutex);
   mCores[core]->nextInterrupt = std::chrono::time_point<std::chrono::system_clock>::max();
   mCores[core]->interrupt = true;
}

void
Processor::setInterruptTimer(uint32_t core, std::chrono::time_point<std::chrono::system_clock> when)
{
   std::unique_lock<std::mutex> lock(mTimerMutex);

   if (when < mCores[core]->nextInterrupt) {
      mCores[core]->nextInterrupt = when;
   }

   mTimerCondition.notify_all();
}

void
Processor::coreEntryPoint(Core *core)
{
   tCurrentCore = core;
   core->primaryFiber = ConvertThreadToFiber(NULL);

   while (mRunning) {
      std::unique_lock<std::mutex> lock(mMutex);

      // Free any fibers which need to be deleted
      for (auto fiber : core->mFiberDeleteList) {
         delete fiber;
      }

      core->mFiberDeleteList.clear();

      if (auto fiber = peekNextFiber(core->id)) {
         // Remove fiber from schedule queue
         mFiberQueue.erase(std::remove(mFiberQueue.begin(), mFiberQueue.end(), fiber), mFiberQueue.end());

         // Switch to fiber
         core->currentFiber = fiber;
         fiber->coreID = core->id;
         fiber->parentFiber = core->primaryFiber;
         fiber->thread->state = OSThreadState::Running;
         lock.unlock();

         gLog->trace("Core {} enter thread {}", core->id, fiber->thread->id);
         SwitchToFiber(fiber->handle);
      } else if (core->interrupt) {
         // Switch to the interrupt thread for any waiting interrupts
         lock.unlock();
         handleInterrupt();
      } else {
         // Wait for a valid fiber
         gLog->trace("Core {} wait for thread", core->id);
         mCondition.wait(lock);
      }
   }
}

void
Processor::fiberEntryPoint(Fiber *fiber)
{
   gInterpreter.executeSub(&fiber->state);
   OSExitThread(ppctypes::getResult<int>(&fiber->state));
}

void
Processor::exit()
{
   auto core = tCurrentCore;
   auto fiber = core->currentFiber;
   auto parent = fiber->parentFiber;
   auto id = fiber->thread->id;

   // Destroy current fiber
   destroyFiber(fiber);
   core->currentFiber = nullptr;

   // Add to fiber free list
   core->mFiberDeleteList.push_back(fiber);

   // Return to parent fiber
   gLog->trace("Core {} exit thread {}", core->id, id);
   SwitchToFiber(parent);
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

   if (!core) {
      // Ran from host thread
      return;
   }

   auto fiber = core->currentFiber;
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

   // Change state to ready, only if this thread is running
   if (fiber->thread->state == OSThreadState::Running) {
      fiber->thread->state = OSThreadState::Ready;
   }

   // Add this fiber to queue
   queue(fiber);

   if (hasSchedulerLock) {
      OSUnlockScheduler();
   }

   gLog->trace("Core {} leave thread {}", core->id, fiber->thread->id);

   // Return to main scheduler fiber
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
   {
      std::lock_guard<std::mutex> lock(mMutex);
      mFiberList.push_back(fiber);
   }
   return fiber;
}

void
Processor::destroyFiber(Fiber *fiber)
{
   std::lock_guard<std::mutex> lock(mMutex);
   mFiberList.erase(std::remove(mFiberList.begin(), mFiberList.end(), fiber), mFiberList.end());
}

Fiber *
Processor::getCurrentFiber()
{
   return tCurrentCore ? tCurrentCore->currentFiber : nullptr;
}

OSContext *
Processor::getInterruptContext()
{
   if (!tCurrentCore || !tCurrentCore->currentFiber) {
      return nullptr;
   } else {
      return &tCurrentCore->currentFiber->thread->context;
   }
}

uint32_t
Processor::getCoreID()
{
   return tCurrentCore ? tCurrentCore->id : 4;
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
