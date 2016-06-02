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

static std::atomic_bool
sSchedulerLock { false };

OSThreadQueue *
sActiveThreads;

OSThreadQueue *
sCoreRunQueue[3];

namespace internal
{

using ActiveThreadQueueFuncs = QueueFuncs < OSThreadQueue, OSThreadLink, OSThread, &OSThread::activeLink >;
using Core0RunThreadQueueFuncs = SortedQueueFuncs < OSThreadQueue, OSThreadLink, OSThread, &OSThread::core0RunQueueLink, ThreadQueueSortFunc>;
using Core1RunThreadQueueFuncs = SortedQueueFuncs < OSThreadQueue, OSThreadLink, OSThread, &OSThread::core1RunQueueLink, ThreadQueueSortFunc>;
using Core2RunThreadQueueFuncs = SortedQueueFuncs < OSThreadQueue, OSThreadLink, OSThread, &OSThread::core2RunQueueLink, ThreadQueueSortFunc>;

void
lockScheduler()
{
   bool locked = false;

   while (!sSchedulerLock.compare_exchange_weak(locked, true, std::memory_order_acquire)) {
      locked = false;
   }
}

void
unlockScheduler()
{
   sSchedulerLock.store(false, std::memory_order_release);
}

void
queueThreadNoLock(OSThread *thread)
{
   //assert(ThisCoreIsHoldingSchedulerLock())
   //assert(!ThreadIsSuspended(thread));
   //assert(thread->state == OSThreadState::Ready);
   //assert(thread->priority >= 0 && thread->priority <= 32);

   // Schedule this thread on any cores which can run it!
   for (auto i = 0; i < 3; ++i) {
      auto core_attr_bit = 1 << i;
      if (!(thread->attr & core_attr_bit)) {
         // The threads affinity doesn't allow it to run on this core.
         continue;
      }

      switch (i) {
      case 0: Core0RunThreadQueueFuncs::insert(sCoreRunQueue[0], thread); break;
      case 1: Core1RunThreadQueueFuncs::insert(sCoreRunQueue[1], thread); break;
      case 2: Core2RunThreadQueueFuncs::insert(sCoreRunQueue[2], thread); break;
      }
   }
}

void
unqueueThreadNoLock(OSThread *thread)
{
   //assert(ThisCoreIsHoldingSchedulerLock())
   //assert(thread->state != OSThreadState::Running);

   Core0RunThreadQueueFuncs::erase(sCoreRunQueue[0], thread);
   Core1RunThreadQueueFuncs::erase(sCoreRunQueue[1], thread);
   Core2RunThreadQueueFuncs::erase(sCoreRunQueue[2], thread);
}

void checkRunningThread(bool yield)
{
   // Check my run queue for something new
   // If nothing better, return 
  
   // queueThreadNoLock(thisThread);
   // unqueueThreadNoLock(newThread);
   // kernel::switchContext(newThread->context);
}

void
rescheduleNoLock(uint32_t core)
{
   if (core == cpu::this_core::id()) {
      kernel::checkActiveThread(false);
   } else {
      cpu::interrupt(core, cpu::GENERIC_INTERRUPT);
   }
}

void
rescheduleSelfNoLock()
{
   rescheduleNoLock(cpu::this_core::id());
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
         kernel::queueThreadNoLock(thread);
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
      OSInsertThreadQueue(queue, thread);
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
   kernel::queueThreadNoLock(thread);
}

void
wakeupThreadNoLock(OSThreadQueue *queue)
{
   for (auto thread = queue->head; thread; thread = thread->link.next) {
      wakeupOneThreadNoLock(thread);
   }

   OSClearThreadQueue(queue);
}

void
wakeupThreadWaitForSuspensionNoLock(OSThreadQueue *queue, int32_t suspendResult)
{
   for (auto thread = queue->head; thread; thread = thread->link.next) {
      thread->suspendResult = suspendResult;
      kernel::queueThreadNoLock(thread);
   }

   OSClearThreadQueue(queue);
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
      sCoreRunQueue[i] = coreinit::internal::sysAlloc<OSThreadQueue>();
   }
}

} // namespace coreinit
