#include <array>
#include "coreinit.h"
#include "coreinit_alarm.h"
#include "coreinit_core.h"
#include "coreinit_scheduler.h"
#include "coreinit_event.h"
#include "coreinit_memheap.h"
#include "coreinit_thread.h"
#include "coreinit_internal_queue.h"
#include "kernel/kernel.h"
#include "cpu/trace.h"

namespace coreinit
{

static bool
sSchedulerEnabled[3];

static std::atomic<uint32_t>
sSchedulerLock { 0 };

// TODO: Use sActiveThreads with OSThread::activeLink
static OSThreadQueue *
sActiveThreads;

static OSThreadQueue *
sCoreRunQueue[3];

static OSThread *
sCurrentThread[3];

namespace internal
{

using ActiveQueue = Queue<OSThreadQueue, OSThreadLink, OSThread, &OSThread::activeLink>;
using CoreRunQueue0 = SortedQueue<OSThreadQueue, OSThreadLink, OSThread, &OSThread::coreRunQueueLink0, threadSortFunc>;
using CoreRunQueue1 = SortedQueue<OSThreadQueue, OSThreadLink, OSThread, &OSThread::coreRunQueueLink1, threadSortFunc>;
using CoreRunQueue2 = SortedQueue<OSThreadQueue, OSThreadLink, OSThread, &OSThread::coreRunQueueLink2, threadSortFunc>;

OSThread *
getCurrentThread()
{
   return sCurrentThread[cpu::this_core::id()];
}

void
lockScheduler()
{
   uint32_t expected = 0;
   auto core = 1 << cpu::this_core::id();

   while (!sSchedulerLock.compare_exchange_weak(expected, core, std::memory_order_acquire)) {
      expected = 0;
   }
}

bool
isSchedulerLocked()
{
   auto core = 1 << cpu::this_core::id();
   return sSchedulerLock.load(std::memory_order_acquire) == core;
}

void
unlockScheduler()
{
   sSchedulerLock.store(0, std::memory_order_release);
}

void
enableScheduler()
{
   uint32_t coreId = cpu::this_core::id();
   sSchedulerEnabled[coreId] = true;
}

void
disableScheduler()
{
   uint32_t coreId = cpu::this_core::id();
   sSchedulerEnabled[coreId] = false;
}

static void
queueThreadNoLock(OSThread *thread)
{
   assert(isSchedulerLocked());
   //assert(!ThreadIsSuspended(thread));
   //assert(thread->state == OSThreadState::Ready);
   //assert(thread->priority >= 0 && thread->priority <= 32);

   // Schedule this thread on any cores which can run it!
   if (thread->attr & OSThreadAttributes::AffinityCPU0) {
      CoreRunQueue0::insert(sCoreRunQueue[0], thread);
   }

   if (thread->attr & OSThreadAttributes::AffinityCPU1) {
      CoreRunQueue1::insert(sCoreRunQueue[1], thread);
   }

   if (thread->attr & OSThreadAttributes::AffinityCPU2) {
      CoreRunQueue2::insert(sCoreRunQueue[2], thread);
   }
}

static void
unqueueThreadNoLock(OSThread *thread)
{
   CoreRunQueue0::erase(sCoreRunQueue[0], thread);
   CoreRunQueue1::erase(sCoreRunQueue[1], thread);
   CoreRunQueue2::erase(sCoreRunQueue[2], thread);
}

static OSThread *
peekNextThreadNoLock(uint32_t core)
{
   assert(isSchedulerLocked());
   auto thread = sCoreRunQueue[core]->head;
   auto bit = 1 << core;

   while (thread) {
      if (thread->state != OSThreadState::Ready) {
         continue;
      }

      if (thread->suspendCounter > 0) {
         continue;
      }

      if (thread->attr & bit) {
         return thread;
      }

      if (core == 0) {
         thread = thread->coreRunQueueLink0.next;
      } else if (core == 1) {
         thread = thread->coreRunQueueLink1.next;
      } else {
         thread = thread->coreRunQueueLink2.next;
      }
   }

   return nullptr;
}

void checkRunningThreadNoLock(bool yielding)
{
   assert(isSchedulerLocked());
   auto coreId = cpu::this_core::id();

   if (!sSchedulerEnabled[coreId]) {
      return;
   }

   auto thread = sCurrentThread[coreId];
   auto next = peekNextThreadNoLock(coreId);

   if (thread
    && thread->suspendCounter <= 0
    && thread->state == OSThreadState::Running) {
      if (!next) {
         // There is no other viable thread, keep running current.
         return;
      }

      if (thread->priority < next->priority) {
         // Next thread has lower priority, keep running current.
         return;
      } else if (!yielding && thread->priority == next->priority) {
         // Next thread has same priority, but we are not yielding.
         return;
      }
   }

   // If thread is in running state then leave it in Ready to run state
   if (thread && thread->state == OSThreadState::Running) {
      thread->state = OSThreadState::Ready;
      queueThreadNoLock(thread);
   }

   // *snip* log thread switch *snip* ...

   const char *threadName = "?";
   const char *nextName = "?";
   if (thread && thread->name) {
      threadName = thread->name;
   }
   if (next && next->name) {
      nextName = next->name;
   }
   if (thread) {
      if (next) {
         gLog->trace("Core {} leaving thread {}[{}] to thread {}[{}]", coreId, thread->id, threadName, next->id, nextName);
      } else {
         gLog->trace("Core {} leaving thread {}[{}] to idle", coreId, thread->id, threadName);
      }
   } else {
      if (next) {
         gLog->trace("Core {} leaving idle to thread {}[{}]", coreId, next->id, nextName);
      } else {
         gLog->trace("Core {} leaving idle to idle", coreId);
      }
   }

   // Remove next thread from Run Queue
   if (next) {
      next->state = OSThreadState::Running;
      unqueueThreadNoLock(next);
   }

   // Switch thread
   sCurrentThread[coreId] = next;
   kernel::switchThread(thread, next);
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
resumeThreadNoLock(OSThread *thread, int32_t counter)
{
   auto old = thread->suspendCounter;
   thread->suspendCounter -= counter;

   if (thread->suspendCounter < 0) {
      thread->suspendCounter = 0;
      return old;
   }

   if (thread->suspendCounter == 0) {
      if (thread->state == OSThreadState::Ready) {
         queueThreadNoLock(thread);
      }
   }

   return old;
}

void
sleepThreadNoLock(OSThreadQueue *queue)
{
   auto thread = OSGetCurrentThread();
   thread->queue = queue;
   thread->state = OSThreadState::Waiting;

   if (queue) {
      ThreadQueue::insert(queue, thread);
   }
}

void
suspendThreadNoLock(OSThread *thread)
{
   thread->requestFlag = OSThreadRequest::None;
   thread->suspendCounter += thread->needSuspend;
   thread->needSuspend = 0;
   thread->state = OSThreadState::Ready;
   wakeupThreadNoLock(&thread->suspendQueue);
}

void
testThreadCancelNoLock()
{
   auto thread = OSGetCurrentThread();

   if (thread->cancelState) {
      if (thread->requestFlag == OSThreadRequest::Suspend) {
         suspendThreadNoLock(thread);
         rescheduleAllCoreNoLock();
      } else if (thread->requestFlag == OSThreadRequest::Cancel) {
         unlockScheduler();
         OSExitThread(-1);
      }
   }
}

void
wakeupOneThreadNoLock(OSThread *thread)
{
   thread->state = OSThreadState::Ready;
   queueThreadNoLock(thread);
}

void
wakeupThreadNoLock(OSThreadQueue *queue)
{
   for (auto thread = queue->head; thread; thread = thread->link.next) {
      wakeupOneThreadNoLock(thread);
   }

   ThreadQueue::clear(queue);
}

void
wakeupThreadWaitForSuspensionNoLock(OSThreadQueue *queue, int32_t suspendResult)
{
   for (auto thread = queue->head; thread; thread = thread->link.next) {
      thread->suspendResult = suspendResult;
      queueThreadNoLock(thread);
   }

   ThreadQueue::clear(queue);
}

} // namespace internal

void
Module::registerSchedulerFunctions()
{
}

void
Module::initialiseSchedulerFunctions()
{
   for (auto i = 0; i < 3; ++i) {
      sSchedulerEnabled[i] = true;
      sCurrentThread[i] = nullptr;
      sCoreRunQueue[i] = coreinit::internal::sysAlloc<OSThreadQueue>();
      OSInitThreadQueue(sCoreRunQueue[i]);
   }
}

} // namespace coreinit
