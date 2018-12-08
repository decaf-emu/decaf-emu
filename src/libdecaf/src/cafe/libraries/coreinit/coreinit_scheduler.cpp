#include "coreinit.h"
#include "coreinit_fastmutex.h"
#include "coreinit_interrupts.h"
#include "coreinit_internal_idlock.h"
#include "coreinit_internal_queue.h"
#include "coreinit_mutex.h"
#include "coreinit_scheduler.h"
#include "coreinit_thread.h"

#include "cafe/kernel/cafe_kernel_context.h"
#include "debugger/debugger.h"

#include <array>
#include <atomic>
#include <chrono>
#include <common/decaf_assert.h>
#include <common/log.h>
#include <fmt/format.h>

namespace cafe::coreinit
{

static constexpr uint32_t
SchedulerLockNonCpuCoreId = 1u << 31;

struct StaticSchedulerData
{
   struct PerCoreData
   {
      be2_val<bool> schedulerEnabled;
      be2_struct<OSThreadQueue> runQueue;
      be2_virt_ptr<OSThread> currentThread;
      std::chrono::time_point<std::chrono::high_resolution_clock> lastSwitchTime;
      std::chrono::time_point<std::chrono::high_resolution_clock> pauseTime;
   };

   internal::IdLock schedulerLock;
   be2_struct<OSThreadQueue> activeThreadQueue;
   be2_array<PerCoreData, 3> perCoreData;
};

static virt_ptr<StaticSchedulerData>
sSchedulerData = nullptr;

namespace internal
{

using ActiveQueue = Queue<OSThreadQueue, OSThreadLink, OSThread, &OSThread::activeLink>;
using CoreRunQueue0 = SortedQueue<OSThreadQueue, OSThreadLink, OSThread, &OSThread::coreRunQueueLink0, ThreadIsLess>;
using CoreRunQueue1 = SortedQueue<OSThreadQueue, OSThreadLink, OSThread, &OSThread::coreRunQueueLink1, ThreadIsLess>;
using CoreRunQueue2 = SortedQueue<OSThreadQueue, OSThreadLink, OSThread, &OSThread::coreRunQueueLink2, ThreadIsLess>;

virt_ptr<OSThread>
getCoreRunningThread(uint32_t coreId)
{
   if (!sSchedulerData) {
      return nullptr;
   }

   return sSchedulerData->perCoreData[coreId].currentThread;
}

uint64_t
getCoreThreadRunningTime(uint32_t coreId)
{
   auto &perCoreData = sSchedulerData->perCoreData[coreId];
   auto now = std::chrono::high_resolution_clock::now();

   if (perCoreData.pauseTime != std::chrono::time_point<std::chrono::high_resolution_clock>::max()) {
      now = perCoreData.pauseTime;
   }

   return (now - perCoreData.lastSwitchTime).count();
}

void
pauseCoreTime(bool isPaused)
{
   auto coreId = cpu::this_core::id();
   auto &perCoreData = sSchedulerData->perCoreData[coreId];
   auto now = std::chrono::high_resolution_clock::now();

   if (isPaused) {
      perCoreData.pauseTime = now;
   } else {
      perCoreData.lastSwitchTime += now - perCoreData.pauseTime;
      perCoreData.pauseTime = std::chrono::time_point<std::chrono::high_resolution_clock>::max();
   }
}

virt_ptr<OSThread>
getFirstActiveThread()
{
   if (!sSchedulerData) {
      return nullptr;
   }

   return sSchedulerData->activeThreadQueue.head;
}

virt_ptr<OSThread>
getCurrentThread()
{
   return getCoreRunningThread(cpu::this_core::id());
}

void
lockScheduler()
{
   internal::acquireIdLockWithCoreId(sSchedulerData->schedulerLock);
}

bool
isSchedulerLocked()
{
   return internal::isHoldingIdLockWithCoreId(sSchedulerData->schedulerLock);
}

void
unlockScheduler()
{
   internal::releaseIdLockWithCoreId(sSchedulerData->schedulerLock);
}

bool
isSchedulerEnabled()
{
   auto coreId = cpu::this_core::id();
   return sSchedulerData->perCoreData[coreId].schedulerEnabled;
}

void
enableScheduler()
{
   auto coreId = cpu::this_core::id();
   sSchedulerData->perCoreData[coreId].schedulerEnabled = true;
}

void
disableScheduler()
{
   auto coreId = cpu::this_core::id();
   sSchedulerData->perCoreData[coreId].schedulerEnabled = false;
}

void
markThreadActiveNoLock(virt_ptr<OSThread> thread)
{
   auto activeThreadQueue = virt_addrof(sSchedulerData->activeThreadQueue);
   decaf_check(!ActiveQueue::contains(activeThreadQueue, thread));
   ActiveQueue::append(activeThreadQueue, thread);
   checkActiveThreadsNoLock();
}

void
markThreadInactiveNoLock(virt_ptr<OSThread> thread)
{
   auto activeThreadQueue = virt_addrof(sSchedulerData->activeThreadQueue);
   decaf_check(ActiveQueue::contains(activeThreadQueue, thread));
   ActiveQueue::erase(activeThreadQueue, thread);
   checkActiveThreadsNoLock();
}

bool
isThreadActiveNoLock(virt_ptr<OSThread> thread)
{
   if (thread->state == OSThreadState::None) {
      return false;
   }

   auto activeThreadQueue = virt_addrof(sSchedulerData->activeThreadQueue);
   return ActiveQueue::contains(activeThreadQueue, thread);
}

static void
queueThreadNoLock(virt_ptr<OSThread> thread)
{
   decaf_check(isSchedulerLocked());
   decaf_check(!OSIsThreadSuspended(thread));
   decaf_check(thread->state == OSThreadState::Ready);

   // Schedule this thread on any cores which can run it!
   if (thread->attr & OSThreadAttributes::AffinityCPU0) {
      CoreRunQueue0::insert(virt_addrof(sSchedulerData->perCoreData[0].runQueue), thread);
   }

   if (thread->attr & OSThreadAttributes::AffinityCPU1) {
      CoreRunQueue1::insert(virt_addrof(sSchedulerData->perCoreData[1].runQueue), thread);
   }

   if (thread->attr & OSThreadAttributes::AffinityCPU2) {
      CoreRunQueue2::insert(virt_addrof(sSchedulerData->perCoreData[2].runQueue), thread);
   }
}

static void
unqueueThreadNoLock(virt_ptr<OSThread> thread)
{
   CoreRunQueue0::erase(virt_addrof(sSchedulerData->perCoreData[0].runQueue), thread);
   CoreRunQueue1::erase(virt_addrof(sSchedulerData->perCoreData[1].runQueue), thread);
   CoreRunQueue2::erase(virt_addrof(sSchedulerData->perCoreData[2].runQueue), thread);
}

void
setThreadAffinityNoLock(virt_ptr<OSThread> thread, uint32_t affinity)
{
   thread->attr &= ~OSThreadAttributes::AffinityAny;
   thread->attr |= affinity;

   if (thread->state == OSThreadState::Ready) {
      if (thread->suspendCounter == 0) {
         unqueueThreadNoLock(thread);
         queueThreadNoLock(thread);
      }
   }
}

static virt_ptr<OSThread>
peekNextThreadNoLock(uint32_t core)
{
   decaf_check(isSchedulerLocked());
   auto thread = sSchedulerData->perCoreData[core].runQueue.head;

   if (thread) {
      decaf_check(thread->state == OSThreadState::Ready);
      decaf_check(thread->suspendCounter == 0);
      decaf_check(thread->attr & static_cast<OSThreadAttributes>(1 << core));
   }

   return thread;
}

static void
validateThread(virt_ptr<OSThread> thread)
{
   decaf_check(*thread->stackEnd == 0xDEADBABE);
   decaf_check((thread->attr & OSThreadAttributes::AffinityAny) != 0);
}

int32_t
checkActiveThreadsNoLock()
{
   auto threadCount = 0;

   // Count threads before this one
   for (virt_ptr<OSThread> threadIter = sSchedulerData->activeThreadQueue.head; threadIter; threadIter = threadIter->activeLink.next) {
      validateThread(threadIter);
      threadCount++;
   }

   return threadCount;
}

void
checkRunningThreadNoLock(bool yielding)
{
   decaf_check(isSchedulerLocked());
   auto coreId = cpu::this_core::id();
   auto &perCoreData = sSchedulerData->perCoreData[coreId];
   checkActiveThreadsNoLock();

   if (!perCoreData.schedulerEnabled) {
      return;
   }

   auto currThread = perCoreData.currentThread;
   auto nextThread = peekNextThreadNoLock(coreId);
   if (!currThread && !nextThread) {
      // No coreinit threads running, we must be in the kernel WFI loop.
      return;
   }

   auto switchTime = std::chrono::high_resolution_clock::now();
   if (currThread) {
      if (currThread->state == OSThreadState::Running) {
         // If we're not ready to suspend check the priority vs next thread
         if (currThread->suspendCounter <= 0) {
            if (!nextThread) {
               // There is no other viable thread, keep running current.
               return;
            }

            if (currThread->priority < nextThread->priority) {
               // Next thread has lower priority, keep running current.
               return;
            } else if (!yielding && currThread->priority == nextThread->priority) {
               // Next thread has same priority, but we are not yielding.
               return;
            }
         }

         // Add thread back to run queue
         currThread->state = OSThreadState::Ready;
         queueThreadNoLock(currThread);
      }

      // Update thread run time
      auto diff = switchTime - perCoreData.lastSwitchTime;
      currThread->coreTimeConsumedNs += diff.count();
   }

   // Trace log the thread switch
   if (gLog->should_log(Logger::Level::trace)) {
      fmt::memory_buffer out;
      fmt::format_to(out, "Core {} leaving", coreId);

      if (currThread) {
         fmt::format_to(out, " thread {}", currThread->id);

         if (currThread->name) {
            fmt::format_to(out, " [{}]", currThread->name);
         }
      } else {
         fmt::format_to(out, " idle");
      }

      fmt::format_to(out, " to");

      if (nextThread) {
         fmt::format_to(out, " thread {}", nextThread->id);

         if (nextThread->name) {
            fmt::format_to(out, " [{}]", nextThread->name);
         }
      } else {
         fmt::format_to(out, " idle");
      }

      gLog->trace("{}", std::string_view { out.data(), out.size() });
   }

   if (nextThread) {
      // Remove next thread from Run Queue
      nextThread->state = OSThreadState::Running;
      nextThread->wakeCount++;
      unqueueThreadNoLock(nextThread);
   }

   // Switch thread
   perCoreData.currentThread = nextThread;
   perCoreData.lastSwitchTime = switchTime;

   // Make sure interrupts are enabled
   auto prevState = coreinit::OSEnableInterrupts();

   internal::unlockScheduler();
   kernel::switchContext(nextThread ? virt_addrof(nextThread->context) : nullptr);
   internal::lockScheduler();

   // Restore interrupts to whatever state they were in
   coreinit::OSRestoreInterrupts(prevState);
   checkActiveThreadsNoLock();
}

void
rescheduleSelfNoLock()
{
   checkRunningThreadNoLock(false);
}

void
rescheduleNoLock(uint32_t core)
{
   if (core == cpu::this_core::id()) {
      rescheduleSelfNoLock();
   } else {
      cpu::interrupt(core, cpu::GENERIC_INTERRUPT);
   }
}

void
rescheduleOtherCoreNoLock()
{
   auto core = cpu::this_core::id();

   for (auto i = 0u; i < 3; ++i) {
      if (i != core) {
         rescheduleNoLock(i);
      }
   }
}

void
rescheduleAllCoreNoLock()
{
   // Reschedule other cores first, or we might exit early!
   rescheduleOtherCoreNoLock();
   rescheduleSelfNoLock();
}

int32_t
resumeThreadNoLock(virt_ptr<OSThread> thread,
                   int32_t counter)
{
   decaf_check(isThreadActiveNoLock(thread));

   auto old = thread->suspendCounter;
   thread->suspendCounter -= counter;

   if (thread->suspendCounter < 0) {
      thread->suspendCounter = 0;
      return old;
   }

   if (thread->suspendCounter == 0) {
      if (thread->state == OSThreadState::Ready) {
         thread->priority = calculateThreadPriorityNoLock(thread);
         queueThreadNoLock(thread);
      }
   }

   return old;
}

void
setCoreRunningThread(uint32_t coreId,
                     virt_ptr<OSThread> thread)
{
   sSchedulerData->perCoreData[coreId].currentThread = thread;
}

bool
setThreadRunQuantumNoLock(virt_ptr<OSThread> thread,
                          OSTime ticks)
{
   decaf_check(isSchedulerLocked());
   decaf_abort("Unsupported call to setThreadRunQuantumNoLock");
}

void
sleepThreadNoLock(virt_ptr<OSThreadQueue> queue)
{
   auto thread = OSGetCurrentThread();
   decaf_check(thread->queue == nullptr);
   decaf_check(thread->state == OSThreadState::Running);

   thread->queue = queue;
   thread->state = OSThreadState::Waiting;

   if (queue) {
      ThreadQueue::insert(queue, thread);
   }
}

void
sleepThreadNoLock(virt_ptr<OSThreadSimpleQueue> queue)
{
   // This is super-strange, it is used by OSFastMutex, and after a few
   //  comparisons, I'm 99% sure they just cast...  I cast it here instead
   //  of inside OSFastMutex so that its closer to the use above to help
   //  ensure nobody mistakenly breaks it...
   sleepThreadNoLock(virt_cast<OSThreadQueue *>(queue));
}

void
suspendThreadNoLock(virt_ptr<OSThread> thread)
{
   thread->requestFlag = OSThreadRequest::None;
   thread->suspendCounter += thread->needSuspend;
   thread->needSuspend = 0;
   thread->state = OSThreadState::Ready;
   wakeupThreadNoLock(virt_addrof(thread->suspendQueue));
}

void
testThreadCancelNoLock()
{
   auto thread = OSGetCurrentThread();

   if (thread->cancelState == OSThreadCancelState::Enabled) {
      if (thread->requestFlag == OSThreadRequest::Suspend) {
         suspendThreadNoLock(thread);
         rescheduleAllCoreNoLock();
      }

      if (thread->requestFlag == OSThreadRequest::Cancel) {
         unlockScheduler();
         OSExitThread(-1);
      }
   }
}

void
wakeupOneThreadNoLock(virt_ptr<OSThread> thread)
{
   if (thread->state == OSThreadState::Running ||
       thread->state == OSThreadState::Ready) {
      // This thread is already running or ready
      return;
   }

   decaf_check(thread->queue);

   thread->state = OSThreadState::Ready;
   ThreadQueue::erase(thread->queue, thread);
   thread->queue = nullptr;
   queueThreadNoLock(thread);
}

void
wakeupThreadNoLock(virt_ptr<OSThreadQueue> queue)
{
   auto next = queue->head;

   for (auto thread = next; next; thread = next) {
      next = thread->link.next;
      wakeupOneThreadNoLock(thread);
   }
}

void
wakeupThreadNoLock(virt_ptr<OSThreadSimpleQueue> queue)
{
   // See sleepThreadNoLock(OSSimpleQueue*) for more details on this hack.
   wakeupThreadNoLock(virt_cast<OSThreadQueue *>(queue));
}

void
wakeupThreadWaitForSuspensionNoLock(virt_ptr<OSThreadQueue> queue, int32_t suspendResult)
{
   for (auto thread = queue->head; thread; thread = thread->link.next) {
      thread->suspendResult = suspendResult;
      wakeupOneThreadNoLock(thread);
   }

   ThreadQueue::clear(queue);
}

int32_t
calculateThreadPriorityNoLock(virt_ptr<OSThread> thread)
{
   decaf_check(isSchedulerLocked());
   auto priority = thread->basePriority;

   // If thread is holding a spinlock, it is always highest priority
   if (thread->context.spinLockCount > 0) {
      return 0;
   }

   // For all mutex we own, boost our priority over anyone waiting to own our mutex
   for (auto mutex = thread->mutexQueue.head; mutex; mutex = mutex->link.next) {
      // We only need to check the head of mutex thread queue as it is in priority order
      auto other = mutex->queue.head;

      if (other && other->priority < priority) {
         priority = other->priority;
      }
   }

   // For all fast mutex we own, boost our priority over anyone waiting to own our fast mutex
   for (auto fastMutex = thread->fastMutexQueue.head; fastMutex; fastMutex = fastMutex->link.next) {
      // We only need to check the head of mutex thread queue as it is in priority order
      auto other = fastMutex->queue.head;

      if (other && other->priority < priority) {
         priority = other->priority;
      }
   }

   return priority;
}

virt_ptr<OSThread>
setThreadActualPriorityNoLock(virt_ptr<OSThread> thread, int32_t priority)
{
   decaf_check(isSchedulerLocked());
   thread->priority = priority;

   if (thread->state == OSThreadState::Ready) {
      if (thread->suspendCounter == 0) {
         unqueueThreadNoLock(thread);
         queueThreadNoLock(thread);
      }
   } else if (thread->state == OSThreadState::Waiting) {
      // Move towards head of queue if needed
      while (thread->link.prev && priority < thread->link.prev->priority) {
         auto prev = thread->link.prev;
         auto next = thread->link.next;

         thread->link.prev = prev->link.prev;
         thread->link.next = prev;

         prev->link.prev = thread;
         prev->link.next = next;

         if (next) {
            next->link.prev = prev;
         }
      }

      // Move towards tail of queue if needed
      while (thread->link.next && thread->link.next->priority < priority) {
         auto prev = thread->link.prev;
         auto next = thread->link.next;

         thread->link.prev = next;
         thread->link.next = next->link.next;

         next->link.prev = prev;
         next->link.next = thread;

         if (prev) {
            prev->link.next = next;
         }
      }

      // If we are waiting for a mutex, return its owner
      if (thread->mutex) {
         return thread->mutex->owner;
      }
   }

   return nullptr;
}

void
updateThreadPriorityNoLock(virt_ptr<OSThread> thread)
{
   // Update the threads priority, and any thread chain of mutex owners
   while (thread) {
      auto priority = calculateThreadPriorityNoLock(thread);
      thread = setThreadActualPriorityNoLock(thread, priority);
   }
}

void
promoteThreadPriorityNoLock(virt_ptr<OSThread> thread,
                            int32_t priority)
{
   while (thread && priority < thread->priority) {
      thread = setThreadActualPriorityNoLock(thread, priority);
   }
}

void
initialiseScheduler()
{
   OSInitThreadQueue(virt_addrof(sSchedulerData->activeThreadQueue));

   for (auto i = 0u; i < sSchedulerData->perCoreData.size(); ++i) {
      auto &perCoreData = sSchedulerData->perCoreData[i];
      perCoreData.schedulerEnabled = true;
      perCoreData.currentThread = nullptr;

      OSInitThreadQueue(virt_addrof(perCoreData.runQueue));

      perCoreData.lastSwitchTime = std::chrono::high_resolution_clock::now();
      perCoreData.pauseTime = std::chrono::time_point<std::chrono::high_resolution_clock>::max();
   }
}

} // namespace internal

void
Library::registerSchedulerSymbols()
{
   RegisterDataInternal(sSchedulerData);
}

} // namespace cafe::coreinit
