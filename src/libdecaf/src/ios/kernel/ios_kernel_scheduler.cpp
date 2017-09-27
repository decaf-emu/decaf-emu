#include "ios_kernel_scheduler.h"
#include "ios_kernel_thread.h"
#include "ios_kernel_threadqueue.h"
#include "ios/ios_core.h"
#include <mutex>

namespace ios::kernel::internal
{

struct SchedulerData
{
   be2_struct<ThreadQueue> runQueue;
};

static phys_ptr<SchedulerData>
sData = nullptr;

static std::mutex
sSchedulerLock;

static thread_local phys_ptr<Thread>
sCurrentThreadContext = nullptr;

phys_ptr<Thread>
getCurrentThread()
{
   return sCurrentThreadContext;
}

ThreadId
getCurrentThreadId()
{
   return sCurrentThreadContext->id;
}

void
lockScheduler()
{
   sSchedulerLock.lock();
}

void
unlockScheduler()
{
   sSchedulerLock.unlock();
}

void
sleepThreadNoLock(phys_ptr<ThreadQueue> queue)
{
   auto thread = sCurrentThreadContext;
   thread->state = ThreadState::Waiting;
   ThreadQueue_PushThread(queue, thread);
}

void
wakeupOneThreadNoLock(phys_ptr<ThreadQueue> queue,
                      Error waitResult)
{
   auto thread = ThreadQueue_PopThread(queue);
   thread->state = ThreadState::Ready;
   thread->context.queueWaitResult = waitResult;
}

void
wakeupAllThreadsNoLock(phys_ptr<ThreadQueue> queue,
                       Error waitResult)
{
   while (queue->first) {
      wakeupOneThreadNoLock(queue, waitResult);
   }
}

void
queueThreadNoLock(phys_ptr<Thread> thread)
{
   ThreadQueue_PushThread(phys_addrof(sData->runQueue), thread);
}

bool
isThreadInRunQueue(phys_ptr<Thread> thread)
{
   return thread->threadQueue == phys_addrof(sData->runQueue);
}

void
rescheduleOtherNoLock()
{
   auto curCore = ios::internal::getCurrentCoreId();
   for (auto i = 0u; i < ios::internal::getCoreCount(); ++i) {
      if (i == curCore) {
         continue;
      }

      ios::internal::interruptCore(i, CoreInterruptFlags::Scheduler);
   }
}

void
rescheduleAllNoLock()
{
   rescheduleOtherNoLock();
   rescheduleSelfNoLock(false);
}

void
rescheduleSelfNoLock(bool yielding)
{
   auto currentThread = sCurrentThreadContext;
   auto nextThread = ThreadQueue_PeekThread(phys_addrof(sData->runQueue));

   if (currentThread && currentThread->state == ThreadState::Running) {
      if (!nextThread) {
         // No other threads to run, we're stuck with this one!
         return;
      }

      if (currentThread->priority > nextThread->priority) {
         // Next thread has lower priority, keep running current.
         return;
      }

      if (!yielding && currentThread->priority == nextThread->priority) {
         // Next thread has same priority, but we are not yielding.
         return;
      }

      currentThread->state = ThreadState::Ready;
      queueThreadNoLock(currentThread);
   }

   decaf_check(ThreadQueue_PopThread(phys_addrof(sData->runQueue)) == nextThread);
   unlockScheduler();

   auto fiberSrc = currentThread ? currentThread->context.fiber : ios::internal::getCurrentCore()->fiber;
   auto fiberDst = nextThread ? nextThread->context.fiber : ios::internal::getCurrentCore()->fiber;

   if (fiberSrc != fiberDst) {
      platform::swapToFiber(fiberSrc, fiberDst);
   }

   lockScheduler();
}

void
handleSchedulerInterrupt()
{
   internal::lockScheduler();
   rescheduleSelfNoLock(false);
   internal::unlockScheduler();
}

} // namespace ios::kernel::internal
