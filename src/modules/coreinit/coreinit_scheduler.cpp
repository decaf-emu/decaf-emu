#include <array>
#include "coreinit.h"
#include "coreinit_alarm.h"
#include "coreinit_core.h"
#include "coreinit_scheduler.h"
#include "coreinit_thread.h"
#include "coreinit_queue.h"
#include "processor.h"
#include "cpu/trace.h"

namespace coreinit
{

static std::atomic_bool
gSchedulerLock { false };

static std::array<OSThread *, CoreCount>
sInterruptThreads;

ThreadEntryPoint
InterruptThreadEntryPoint;

// Setup a thread fiber, used by OSRunThread and OSResumeThread
static void
InitialiseThreadFiber(OSThread *thread)
{
   // Create a fiber for the thread
   auto fiber = gProcessor.createFiber();
   thread->fiber = fiber;
   fiber->thread = thread;

   // Setup thread state
   memset(&fiber->state, 0, sizeof(ThreadState));

   for (auto i = 0u; i < 32; ++i) {
      fiber->state.gpr[i] = thread->context.gpr[i];
   }

   // Setup entry point
   fiber->state.cia = 0;
   fiber->state.nia = thread->entryPoint;

   // Initialise tracer
   traceInit(&fiber->state, 1024);
}

namespace internal
{

void
lockScheduler()
{
   bool locked = false;

   while (!gSchedulerLock.compare_exchange_weak(locked, true, std::memory_order_acquire)) {
      locked = false;
   }
}

void
unlockScheduler()
{
   gSchedulerLock.store(false, std::memory_order_release);
}

void
rescheduleNoLock()
{
   gProcessor.reschedule(true);
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
         // Create a fiber for the thread to run on, if this is the first time!
         if (!thread->fiber) {
            InitialiseThreadFiber(thread);
         }

         gProcessor.queue(thread->fiber);
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
         rescheduleNoLock();
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
   gProcessor.queue(thread->fiber);
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
      gProcessor.queue(thread->fiber);
   }

   OSClearThreadQueue(queue);
}

OSThread *
setInterruptThread(uint32_t core, OSThread *thread)
{
   assert(core < CoreCount);
   auto old = sInterruptThreads[core];
   sInterruptThreads[core] = thread;
   return old;
}

} // namespace internal

void
InterruptThreadEntry(uint32_t core, void *arg2)
{
   // Initially keep thread dead until an interrupt wakes it
   gProcessor.waitFirstInterrupt();

   while (true) {
      auto context = gProcessor.getInterruptContext();

      // Check alarms for interrupts
      coreinit::internal::checkAlarms(core, context);

      // TODO: Process any other interrupts, e.g. async file system

      // Return to interrupted fiber
      gProcessor.finishInterrupt();
   }
}

void
Module::registerSchedulerFunctions()
{
   RegisterKernelFunction(InterruptThreadEntry);
}

void
Module::initialiseSchedulerFunctions()
{
   sInterruptThreads.fill(nullptr);
   InterruptThreadEntryPoint = findExportAddress("InterruptThreadEntry");
}

} // namespace coreinit
