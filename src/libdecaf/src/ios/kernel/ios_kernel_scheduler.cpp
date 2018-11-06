#include "ios_kernel_hardware.h"
#include "ios_kernel_scheduler.h"
#include "ios_kernel_thread.h"
#include "ios_kernel_threadqueue.h"
#include "ios_kernel_process.h"

#include <common/log.h>
#include <common/platform_fiber.h>

namespace ios::kernel::internal
{

struct StaticSchedulerData
{
   be2_struct<ThreadQueue> runQueue;
};

static phys_ptr<StaticSchedulerData>
sData = nullptr;

static thread_local phys_ptr<Thread>
sCurrentThreadContext = nullptr;

static platform::Fiber *
sIdleFiber = nullptr;

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
sleepThread(phys_ptr<ThreadQueue> queue)
{
   auto thread = sCurrentThreadContext;
   thread->state = ThreadState::Waiting;
   ThreadQueue_PushThread(queue, thread);
}

void
wakeupOneThread(phys_ptr<ThreadQueue> queue,
                Error waitResult)
{
   if (auto thread = ThreadQueue_PopThread(queue)) {
      thread->state = ThreadState::Ready;
      thread->context.queueWaitResult = waitResult;
      queueThread(thread);
   }
}

void
wakeupAllThreads(phys_ptr<ThreadQueue> queue,
                 Error waitResult)
{
   while (queue->first) {
      wakeupOneThread(queue, waitResult);
   }
}

void
queueThread(phys_ptr<Thread> thread)
{
   ThreadQueue_PushThread(phys_addrof(sData->runQueue), thread);
}

bool
isThreadInRunQueue(phys_ptr<Thread> thread)
{
   return thread->threadQueue == phys_addrof(sData->runQueue);
}

void
reschedule(bool yielding)
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

      decaf_check(ThreadQueue_PopThread(phys_addrof(sData->runQueue)) == nextThread);
      queueThread(currentThread);
   } else {
      decaf_check(ThreadQueue_PopThread(phys_addrof(sData->runQueue)) == nextThread);
   }

   // Trace log the thread switch
   if (gLog->should_log(spdlog::level::trace)) {
      fmt::memory_buffer out;
      fmt::format_to(out, "IOS leaving");

      if (currentThread) {
         fmt::format_to(out, " thread {}", currentThread->id);

         if (currentThread->context.threadName) {
            fmt::format_to(out, " [{}]", currentThread->context.threadName);
         }
      } else {
         fmt::format_to(out, " idle");
      }

      fmt::format_to(out, " to");

      if (nextThread) {
         fmt::format_to(out, " thread {}", nextThread->id);

         if (nextThread->context.threadName) {
            fmt::format_to(out, " [{}]", nextThread->context.threadName);
         }
      } else {
         fmt::format_to(out, " idle");
      }

      gLog->trace("{}", std::string_view { out.data(), out.size() });
   }

   sCurrentThreadContext = nextThread;

   if (nextThread) {
      nextThread->state = ThreadState::Running;
   }

   auto fiberSrc = currentThread ? currentThread->context.fiber : sIdleFiber;
   auto fiberDst = nextThread ? nextThread->context.fiber : sIdleFiber;

   if (fiberSrc != fiberDst) {
      platform::swapToFiber(fiberSrc, fiberDst);
   }
}

void
setIdleFiber()
{
   sIdleFiber = platform::getThreadFiber();
}

void
initialiseStaticSchedulerData()
{
   sData = allocProcessStatic<StaticSchedulerData>();
}

} // namespace ios::kernel::internal
