#include <array>
#include "coreinit.h"
#include "coreinit_alarm.h"
#include "coreinit_core.h"
#include "coreinit_scheduler.h"
#include "coreinit_interrupts.h"
#include "coreinit_event.h"
#include "coreinit_memheap.h"
#include "coreinit_mutex.h"
#include "coreinit_thread.h"
#include "coreinit_internal_queue.h"
#include "coreinit_internal_loader.h"
#include "debugger/debugger.h"
#include "kernel/kernel.h"
#include "libcpu/trace.h"
#include "ppcutils/wfunc_call.h"
#include "ppcutils/stackobject.h"
#include "common/emuassert.h"

namespace coreinit
{

static bool
sSchedulerEnabled[3];

static std::atomic<uint32_t>
sSchedulerLock { 0 };

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
getCoreRunningThread(uint32_t coreId)
{
   return sCurrentThread[coreId];
}

OSThread *
getFirstActiveThread()
{
   return sActiveThreads->head;
}

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

bool
isSchedulerEnabled()
{
   uint32_t coreId = cpu::this_core::id();
   return sSchedulerEnabled[coreId];
}

void
enableScheduler()
{
   emuassert(!OSIsInterruptEnabled());
   uint32_t coreId = cpu::this_core::id();
   sSchedulerEnabled[coreId] = true;
}

void
disableScheduler()
{
   emuassert(!OSIsInterruptEnabled());
   uint32_t coreId = cpu::this_core::id();
   sSchedulerEnabled[coreId] = false;
}

void markThreadActiveNoLock(OSThread *thread)
{
   ActiveQueue::append(sActiveThreads, thread);
}

void markThreadInactiveNoLock(OSThread *thread)
{
   ActiveQueue::erase(sActiveThreads, thread);
}

static void
queueThreadNoLock(OSThread *thread)
{
   emuassert(isSchedulerLocked());
   emuassert(!OSIsThreadSuspended(thread));
   emuassert(thread->state == OSThreadState::Ready);
   emuassert(thread->priority >= -1 && thread->priority <= 32);

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

void
setThreadAffinityNoLock(OSThread *thread, uint32_t affinity)
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

static OSThread *
peekNextThreadNoLock(uint32_t core)
{
   emuassert(isSchedulerLocked());
   auto thread = sCoreRunQueue[core]->head;

   if (thread) {
      emuassert(thread->state == OSThreadState::Ready);
      emuassert(thread->suspendCounter == 0);
      emuassert(thread->attr & (1 << core));
   }

   return thread;
}

void checkRunningThreadNoLock(bool yielding)
{
   emuassert(isSchedulerLocked());
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
         thread->priority = calculateThreadPriorityNoLock(thread);
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
   if (thread->state == OSThreadState::Running ||
      thread->state == OSThreadState::Ready) {
      // This thread is already running or ready
      return;
   }

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
      wakeupOneThreadNoLock(thread);
   }

   ThreadQueue::clear(queue);
}

int32_t
calculateThreadPriorityNoLock(OSThread *thread)
{
   emuassert(isSchedulerLocked());
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

   // TODO: Owned Fast Mutex queue
   return priority;
}

OSThread *
setThreadActualPriorityNoLock(OSThread *thread, int32_t priority)
{
   emuassert(isSchedulerLocked());
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
updateThreadPriorityNoLock(OSThread *thread)
{
   // Update the threads priority, and any thread chain of mutex owners
   while (thread) {
      auto priority = calculateThreadPriorityNoLock(thread);
      thread = setThreadActualPriorityNoLock(thread, priority);
   }
}

void
promoteThreadPriorityNoLock(OSThread *thread, int32_t priority)
{
   while (thread && priority < thread->priority) {
      thread = setThreadActualPriorityNoLock(thread, priority);
   }
}

} // namespace internal

void
GameThreadEntry(uint32_t argc, void *argv)
{
   auto appModule = internal::getUserModule();

   auto userPreinit = appModule->findFuncExport<void, be_ptr<CommonHeap>*, be_ptr<CommonHeap>*, be_ptr<CommonHeap>*>("__preinit_user");
   auto start = OSThreadEntryPointFn(appModule->entryPoint);

   debugger::handlePreLaunch();

   if (userPreinit) {
      ppcutils::StackObject<be_ptr<CommonHeap>> mem1HeapPtr;
      ppcutils::StackObject<be_ptr<CommonHeap>> fgHeapPtr;
      ppcutils::StackObject<be_ptr<CommonHeap>> mem2HeapPtr;

      *mem1HeapPtr = MEMGetBaseHeapHandle(MEMBaseHeapType::MEM1);
      *fgHeapPtr = MEMGetBaseHeapHandle(MEMBaseHeapType::FG);
      *mem2HeapPtr = MEMGetBaseHeapHandle(MEMBaseHeapType::MEM2);

      userPreinit(mem1HeapPtr, fgHeapPtr, mem2HeapPtr);

      MEMSetBaseHeapHandle(MEMBaseHeapType::MEM1, *mem1HeapPtr);
      MEMSetBaseHeapHandle(MEMBaseHeapType::FG, *fgHeapPtr);
      MEMSetBaseHeapHandle(MEMBaseHeapType::MEM2, *mem2HeapPtr);
   }

   start(argc, argv);
}

void
Module::registerSchedulerFunctions()
{
   RegisterKernelFunction(GameThreadEntry);
}

void
Module::initialiseSchedulerFunctions()
{
   sActiveThreads = coreinit::internal::sysAlloc<OSThreadQueue>();
   for (auto i = 0; i < 3; ++i) {
      sSchedulerEnabled[i] = true;
      sCurrentThread[i] = nullptr;
      sCoreRunQueue[i] = coreinit::internal::sysAlloc<OSThreadQueue>();
      OSInitThreadQueue(sCoreRunQueue[i]);
   }
}

} // namespace coreinit
