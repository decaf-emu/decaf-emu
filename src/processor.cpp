#include <algorithm>
#include "cpu/cpu.h"
#include "cpu/state.h"
#include "debugcontrol.h"
#include "modules/coreinit/coreinit_core.h"
#include "modules/coreinit/coreinit_thread.h"
#include "modules/coreinit/coreinit_scheduler.h"
#include "platform/platform_fiber.h"
#include "platform/platform_thread.h"
#include "processor.h"
#include "ppcinvoke.h"
#include "utils/log.h"

Processor
gProcessor { CoreCount };

__declspec(thread) Core *
tCurrentCore = nullptr;

Processor::Processor(size_t cores)
{
   for (auto i = 0u; i < cores; ++i) {
      mCores.push_back(new Core { i });
   }
}

// Starts up the CPU threads and Timer thread
void
Processor::start()
{
   mRunning = true;

   cpu::set_interrupt_handler(handleInterrupt);

   for (auto core : mCores) {
      core->thread = std::thread(std::bind(&Processor::coreEntryPoint, this, core));

      static const std::string coreNames[] = { "Core #0", "Core #1", "Core #2" };
      platform::setThreadName(&core->thread, coreNames[core->id]);
   }

   mTimerThread = std::thread(std::bind(&Processor::timerEntryPoint, this));
   platform::setThreadName(&mTimerThread, "Timer Thread");
}

void
Processor::wakeAllCores()
{
   mCondition.notify_all();
}

// Wait for all threads to end
void
Processor::join()
{
   for (auto core : mCores) {
      core->thread.join();
   }

   mTimerThread.join();
}

// Entry point of new fibers
void
Processor::fiberEntryPoint(Fiber *fiber)
{
   auto core = tCurrentCore;

   if (!core) {
      gLog->error("fiberEntryPoint called from non-ppc thread.");
      return;
   }

   cpu::executeSub(&core->state, &fiber->state);
   OSExitThread(ppctypes::getResult<int>(&fiber->state));
}

void
Fiber::fiberEntryPoint(void *param)
{
   gProcessor.fiberEntryPoint(reinterpret_cast<Fiber *>(param));
}

// Entry point of CPU Core threads
void
Processor::coreEntryPoint(Core *core)
{
   tCurrentCore = core;
   core->primaryFiberHandle = platform::getThreadFiber();

   while (mRunning) {
      // Intentionally do this before the lock...
      gDebugControl.maybeBreak(0, nullptr, core->id);

      std::unique_lock<std::mutex> lock { mMutex };

      // Cleanup pending delete fibers
      for (auto fiber : core->fiberDeleteList) {
         mFiberList.erase(std::remove(mFiberList.begin(), mFiberList.end(), fiber), mFiberList.end());
         delete fiber;
      }

      core->fiberDeleteList.clear();

      // Queue any pending fibers
      for (auto fiber : core->fiberPendingList) {
         queueNoLock(fiber);
      }

      core->fiberPendingList.clear();

      // Check for any interrupts
      if (cpu::hasInterrupt(&core->state)) {
         cpu::clearInterrupt(&core->state);

         core->interruptHandlerFiber->thread->state = OSThreadState::Ready;
         queueNoLock(core->interruptHandlerFiber);
      }

      if (auto fiber = peekNextFiberNoLock(core->id)) {
         // Remove fiber from schedule queue
         mFiberQueue.erase(std::remove(mFiberQueue.begin(), mFiberQueue.end(), fiber), mFiberQueue.end());

         // Switch to fiber
         core->currentFiber = fiber;
         core->threadId = fiber->thread->id;
         fiber->coreID = core->id;
         fiber->thread->state = OSThreadState::Running;
         lock.unlock();

         gLog->trace("Core {} enter thread {}", core->id, fiber->thread->id);
         platform::swapToFiber(core->primaryFiberHandle, fiber->handle);
         core->threadId = 0;
      } else {
         // Wait for a valid fiber
         gLog->trace("Core {} wait for thread", core->id);
         mCondition.wait(lock);
      }
   }
}

void
Processor::reschedule(bool hasSchedulerLock, bool yield)
{
   std::unique_lock<std::mutex> lock { mMutex };
   auto core = tCurrentCore;

   if (!core) {
      // Ran from host thread
      return;
   }

   auto fiber = core->currentFiber;
   auto thread = fiber->thread;
   auto next = peekNextFiberNoLock(core->id);

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

   // Add this fiber to pending list
   core->fiberPendingList.push_back(fiber);

   if (hasSchedulerLock) {
      OSUnlockScheduler();
   }

   gLog->trace("Core {} leave thread {}", core->id, fiber->thread->id);

   // Return to main scheduler fiber
   lock.unlock();
   platform::swapToFiber(fiber->handle, core->primaryFiberHandle);

   // Reacquire scheduler lock if needed
   if (hasSchedulerLock) {
      OSLockScheduler();
   }
}

// Yield current thread to one of equal or higher priority
void
Processor::yield()
{
   reschedule(false, true);
}

// Exit current thread
void
Processor::exit()
{
   auto core = tCurrentCore;
   auto fiber = core->currentFiber;
   auto id = fiber->thread->id;

   // Put fiber on pending delete list
   core->fiberDeleteList.push_back(fiber);

   // Return to parent fiber
   gLog->trace("Core {} exit fiber {}", core->id, id);
   platform::swapToFiber(fiber->handle, core->primaryFiberHandle);
}

// Insert a fiber into the run queue
void
Processor::queue(Fiber *fiber)
{
   std::unique_lock<std::mutex> lock { mMutex };
   queueNoLock(fiber);
}

void
Processor::queueNoLock(Fiber *fiber)
{
   auto compare =
      [](Fiber *lhs, Fiber *rhs) {
      return lhs->thread->basePriority < rhs->thread->basePriority;
   };

   if (tCurrentCore) {
      gLog->trace("Core {} queued thread {}", tCurrentCore->id, fiber->thread->id);
   } else {
      gLog->trace("System queued thread {}", fiber->thread->id);
   }

   auto pos = std::upper_bound(mFiberQueue.begin(), mFiberQueue.end(), fiber, compare);
   mFiberQueue.insert(pos, fiber);
   mCondition.notify_all();
}

// Create a new fiber
Fiber *
Processor::createFiber()
{
   std::lock_guard<std::mutex> lock { mMutex };
   return createFiberNoLock();
}

Fiber *
Processor::createFiberNoLock()
{
   auto fiber = new Fiber();
   mFiberList.push_back(fiber);
   return fiber;
}

// Find the next suitable fiber to run on a core
Fiber *
Processor::peekNextFiberNoLock(uint32_t core)
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

Fiber *
Processor::getCurrentFiber()
{
   return tCurrentCore ? tCurrentCore->currentFiber : nullptr;
}

OSContext *
Processor::getInterruptContext()
{
   if (!tCurrentCore) {
      gLog->error("Called getInterruptContext on non-ppc thread.");
      return nullptr;
   }

   if (tCurrentCore->interruptedFiber) {
      return &tCurrentCore->interruptedFiber->thread->context;
   } else {
      return nullptr;
   }
}

// Entry point of interrupt thread
void
Processor::timerEntryPoint()
{
   while (mRunning) {
      std::unique_lock<std::mutex> lock { mTimerMutex };
      auto now = std::chrono::system_clock::now();
      auto next = std::chrono::time_point<std::chrono::system_clock>::max();
      bool timedWait = false;

      for (auto core : mCores) {
         if (core->nextInterrupt <= now) {
            std::unique_lock<std::mutex> cpuLock { mMutex };
            cpu::interrupt(&core->state);
            core->nextInterrupt = std::chrono::time_point<std::chrono::system_clock>::max();
            wakeAllCores();
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

// Sleep the interrupt thread until the first interrupt happens
void
Processor::waitFirstInterrupt()
{
   auto core = tCurrentCore;
   auto fiber = core->currentFiber;
   assert(fiber->thread->basePriority == -1);
   core->interruptHandlerFiber = fiber;
   platform::swapToFiber(fiber->handle, core->primaryFiberHandle);
}

void
Processor::handleInterrupt(cpu::CoreState *core, ThreadState *state)
{
   gProcessor.handleInterrupt();
}

// Yield to interrupt thread to handle any pending interrupt
void
Processor::handleInterrupt()
{
   auto core = tCurrentCore;

   if (!core) {
      gLog->error("handleInterrupt called from non-ppc thread");
      return;
   }

   // Add interrupted fiber to pending run list and return to primary fiber
   auto fiber = core->currentFiber;
   fiber->thread->state = OSThreadState::Ready;
   core->fiberPendingList.push_back(fiber);
   core->interruptedFiber = fiber;

   platform::swapToFiber(fiber->handle, core->primaryFiberHandle);
}

// Return to normal processing
void
Processor::finishInterrupt()
{
   auto core = tCurrentCore;

   if (!core) {
      gLog->error("finishInterrupt called from non-ppc thread");
      return;
   }

   gLog->trace("Exit interrupt core {}", core->id);

   // Clear interrupted fiber and return to primary fiber
   auto fiber = core->currentFiber;
   core->interruptedFiber = nullptr;
   platform::swapToFiber(fiber->handle, core->primaryFiberHandle);
}

// Set the time of the next interrupt, will not overwrite sooner times
void
Processor::setInterruptTimer(uint32_t core, std::chrono::time_point<std::chrono::system_clock> when)
{
   std::unique_lock<std::mutex> lock { mTimerMutex };

   if (when < mCores[core]->nextInterrupt) {
      mCores[core]->nextInterrupt = when;
   }

   mTimerCondition.notify_all();
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
      threadID = tCurrentCore->threadId;
   }

   return (coreID << 8) | threadID;
}
}
}
}
