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

namespace internal
{

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
}

} // namespace coreinit
