#include "coreinit.h"
#include "coreinit_scheduler.h"
#include "coreinit_thread.h"
#include "processor.h"

static std::atomic_bool
gSchedulerLock = false;

void
OSLockScheduler()
{
   bool locked = false;

   while (!gSchedulerLock.compare_exchange_weak(locked, true, std::memory_order_acquire)) {
      locked = false;
   }
}

void
OSUnlockScheduler()
{
   gSchedulerLock.store(false, std::memory_order_release);
}

void
OSRescheduleNoLock()
{
   gProcessor.reschedule(true);
}

int32_t
OSResumeThreadNoLock(OSThread *thread, int32_t counter)
{
   auto old = thread->suspendCounter;
   thread->suspendCounter -= counter;

   if (thread->suspendCounter < 0) {
      thread->suspendCounter = 0;
      return old;
   }

   if (thread->suspendCounter == 0) {
      if (thread->state == OSThreadState::Ready) {
         gProcessor.queue(thread->fiber);
      }
   }

   return old;
}

void
OSSleepThreadNoLock(OSThreadQueue *queue)
{
   auto thread = OSGetCurrentThread();
   thread->queue = queue;
   thread->state = OSThreadState::Waiting;

   if (queue) {
      OSInsertThreadQueue(queue, thread);
   }
}

void
OSSuspendThreadNoLock(OSThread *thread)
{
   thread->requestFlag = OSThreadRequest::None;
   thread->suspendCounter += thread->needSuspend;
   thread->needSuspend = 0;
   thread->state = OSThreadState::Ready;
   OSWakeupThreadNoLock(&thread->suspendQueue);
}

void
OSTestThreadCancelNoLock()
{
   auto thread = OSGetCurrentThread();

   if (thread->cancelState) {
      if (thread->requestFlag == OSThreadRequest::Suspend) {
         OSSuspendThreadNoLock(thread);
         OSRescheduleNoLock();
      } else if (thread->requestFlag == OSThreadRequest::Cancel) {
         OSUnlockScheduler();
         OSExitThread(-1);
      }
   }
}

void
OSWakeupOneThreadNoLock(OSThread *thread)
{
   gProcessor.queue(thread->fiber);
}

void
OSWakeupThreadNoLock(OSThreadQueue *queue)
{
   for (auto thread = queue->head; thread; thread = thread->link.next) {
      gProcessor.queue(thread->fiber);
   }

   OSClearThreadQueue(queue);
}

void
OSWakeupThreadWaitForSuspensionNoLock(OSThreadQueue *queue, int32_t suspendResult)
{
   for (auto thread = queue->head; thread; thread = thread->link.next) {
      thread->suspendResult = suspendResult;
      gProcessor.queue(thread->fiber);
   }

   OSClearThreadQueue(queue);
}
