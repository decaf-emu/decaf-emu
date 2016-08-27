#include <array>
#include <chrono>
#include "coreinit.h"
#include "coreinit_alarm.h"
#include "coreinit_core.h"
#include "coreinit_scheduler.h"
#include "coreinit_interrupts.h"
#include "coreinit_event.h"
#include "coreinit_fastmutex.h"
#include "coreinit_memexpheap.h"
#include "coreinit_memheap.h"
#include "coreinit_mutex.h"
#include "coreinit_thread.h"
#include "coreinit_internal_queue.h"
#include "debugger/debugger.h"
#include "kernel/kernel.h"
#include "kernel/kernel_loader.h"
#include "libcpu/trace.h"
#include "ppcutils/wfunc_call.h"
#include "ppcutils/stackobject.h"
#include "common/decaf_assert.h"

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

static std::chrono::time_point<std::chrono::high_resolution_clock>
sLastSwitchTime[3];

static std::chrono::time_point<std::chrono::high_resolution_clock>
sCorePauseTime[3];

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

uint64_t
getCoreThreadRunningTime(uint32_t coreId) {
   auto now = std::chrono::high_resolution_clock::now();
   if (sCorePauseTime[coreId] != std::chrono::time_point<std::chrono::high_resolution_clock>::max()) {
      now = sCorePauseTime[coreId];
   }
   return (now - sLastSwitchTime[coreId]).count();
}

void
pauseCoreTime(bool isPaused) {
   auto coreId = cpu::this_core::id();
   auto now = std::chrono::high_resolution_clock::now();
   if (isPaused) {
      sCorePauseTime[coreId] = now;
   } else {
      sLastSwitchTime[coreId] += now - sCorePauseTime[coreId];
      sCorePauseTime[coreId] = std::chrono::time_point<std::chrono::high_resolution_clock>::max();
   }
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
   auto core = 1 << cpu::this_core::id();
   auto oldCore = sSchedulerLock.exchange(0, std::memory_order_release);
   decaf_check(oldCore == core);
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
   uint32_t coreId = cpu::this_core::id();
   sSchedulerEnabled[coreId] = true;
}

void
disableScheduler()
{
   uint32_t coreId = cpu::this_core::id();
   sSchedulerEnabled[coreId] = false;
}

void
markThreadActiveNoLock(OSThread *thread)
{
   decaf_check(!ActiveQueue::contains(sActiveThreads, thread));
   ActiveQueue::append(sActiveThreads, thread);
   checkActiveThreadsNoLock();
}

void
markThreadInactiveNoLock(OSThread *thread)
{
   decaf_check(ActiveQueue::contains(sActiveThreads, thread));
   ActiveQueue::erase(sActiveThreads, thread);
   checkActiveThreadsNoLock();
}

bool
isThreadActiveNoLock(OSThread *thread)
{
   if (thread->state == OSThreadState::None) {
      return false;
   }

   return ActiveQueue::contains(sActiveThreads, thread);
}

static void
queueThreadNoLock(OSThread *thread)
{
   decaf_check(isSchedulerLocked());
   decaf_check(!OSIsThreadSuspended(thread));
   decaf_check(thread->state == OSThreadState::Ready);
   decaf_check(thread->priority >= -1 && thread->priority <= 32);

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
   decaf_check(isSchedulerLocked());
   auto thread = sCoreRunQueue[core]->head;

   if (thread) {
      decaf_check(thread->state == OSThreadState::Ready);
      decaf_check(thread->suspendCounter == 0);
      decaf_check(thread->attr & (1 << core));
   }

   return thread;
}

static void
validateThread(OSThread *thread)
{
   decaf_check(*thread->stackEnd == 0xDEADBABE);
   decaf_check((thread->attr & OSThreadAttributes::AffinityAny) != 0);
}

int32_t
checkActiveThreadsNoLock()
{
   // Counter for the number of threads, 1 for the current thread
   int32_t threadCount = 0;

   // Count threads before this one
   for (OSThread *threadIter = sActiveThreads->head; threadIter != nullptr; threadIter = threadIter->activeLink.next) {
      validateThread(threadIter);
      threadCount++;
   }

   return threadCount;
}

void checkRunningThreadNoLock(bool yielding)
{
   decaf_check(isSchedulerLocked());
   auto coreId = cpu::this_core::id();
   auto thread = sCurrentThread[coreId];

   // Do a check to see if anything has become corrupted...
   if (thread) {
      checkActiveThreadsNoLock();
   }

   if (!sSchedulerEnabled[coreId]) {
      return;
   }

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

   // Update thread core time tracking stuff
   auto now = std::chrono::high_resolution_clock::now();
   if (thread) {
      auto diff = now - sLastSwitchTime[coreId];
      thread->coreTimeConsumedNs += diff.count();
   }
   sLastSwitchTime[coreId] = now;
   if (next) {
      next->wakeCount++;
   }

   // Make sure interrupts are enabled
   BOOL prevState = coreinit::OSEnableInterrupts();

   // Switch thread
   sCurrentThread[coreId] = next;

   internal::unlockScheduler();
   kernel::setContext(&next->context);
   internal::lockScheduler();

   // Restore interrupts to whatever state they were in
   coreinit::OSRestoreInterrupts(prevState);

   if (thread) {
      checkActiveThreadsNoLock();
   }
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
sleepThreadNoLock(OSThreadQueue *queue)
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
sleepThreadNoLock(OSThreadSimpleQueue *queue)
{
   // This is super-strange, it is used by OSFastMutex, and after a few
   //  comparisons, I'm 99% sure they just cast...  I cast it here instead
   //  of inside OSFastMutex so that its closer to the use above to help
   //  ensure nobody mistakenly breaks it...
   sleepThreadNoLock(reinterpret_cast<OSThreadQueue*>(queue));
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
wakeupOneThreadNoLock(OSThread *thread)
{
   if (thread->state == OSThreadState::Running ||
      thread->state == OSThreadState::Ready) {
      // This thread is already running or ready
      return;
   }

   decaf_check(thread->queue != nullptr);

   thread->state = OSThreadState::Ready;
   ThreadQueue::erase(thread->queue, thread);
   thread->queue = nullptr;
   queueThreadNoLock(thread);
}

void
wakeupThreadNoLock(OSThreadQueue *queue)
{
   auto next = queue->head;

   for (auto thread = next; next; thread = next) {
      next = thread->link.next;
      wakeupOneThreadNoLock(thread);
   }
}

void
wakeupThreadNoLock(OSThreadSimpleQueue *queue)
{
   // See sleepThreadNoLock(OSSimpleQueue*) for more details on this hack.
   wakeupThreadNoLock(reinterpret_cast<OSThreadQueue*>(queue));
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

OSThread *
setThreadActualPriorityNoLock(OSThread *thread, int32_t priority)
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

void startDefaultCoreThreads()
{
   auto appModule = kernel::getUserModule();
   auto stackSize = appModule->defaultStackSize;

   for (auto i = 0u; i < coreinit::CoreCount; ++i) {
      auto thread = coreinit::internal::sysAlloc<coreinit::OSThread>();
      auto stack = reinterpret_cast<uint8_t*>(coreinit::internal::sysAlloc(stackSize, 8));
      auto name = coreinit::internal::sysStrDup(fmt::format("Default Thread {}", i));

      coreinit::OSCreateThread(thread, 0u, 0, nullptr,
         reinterpret_cast<be_val<uint32_t>*>(stack + stackSize), stackSize, 16,
         static_cast<coreinit::OSThreadAttributes>(1 << i));
      coreinit::internal::setDefaultThread(i, thread);
      coreinit::OSSetThreadName(thread, name);
   }
}

} // namespace internal

void
GameThreadEntry(uint32_t argc, void *argv)
{
   // Allocate some memory as if there were system libraries which were loaded...
   auto mem2Heap = MEMGetBaseHeapHandle(MEMBaseHeapType::MEM2);
   MEMAllocFromExpHeapEx(reinterpret_cast<MEMExpHeap*>(mem2Heap), 0x01000000, 4);

   // Start up the game!
   auto appModule = kernel::getUserModule();

   auto userPreinit = appModule->findFuncExport<void, be_ptr<MEMHeapHeader>*, be_ptr<MEMHeapHeader>*, be_ptr<MEMHeapHeader>*>("__preinit_user");
   auto startFn = kernel::loader::AppEntryPoint(appModule->entryPoint);

   debugger::handlePreLaunch();

   if (userPreinit) {
      ppcutils::StackObject<be_ptr<MEMHeapHeader>> mem1HeapPtr;
      ppcutils::StackObject<be_ptr<MEMHeapHeader>> fgHeapPtr;
      ppcutils::StackObject<be_ptr<MEMHeapHeader>> mem2HeapPtr;

      *mem1HeapPtr = MEMGetBaseHeapHandle(MEMBaseHeapType::MEM1);
      *fgHeapPtr = MEMGetBaseHeapHandle(MEMBaseHeapType::FG);
      *mem2HeapPtr = MEMGetBaseHeapHandle(MEMBaseHeapType::MEM2);

      gLog->info("Executing application user pre-init");
      userPreinit(mem1HeapPtr, fgHeapPtr, mem2HeapPtr);

      MEMSetBaseHeapHandle(MEMBaseHeapType::MEM1, *mem1HeapPtr);
      MEMSetBaseHeapHandle(MEMBaseHeapType::FG, *fgHeapPtr);
      MEMSetBaseHeapHandle(MEMBaseHeapType::MEM2, *mem2HeapPtr);
   }

   auto loadedModules = kernel::loader::getLoadedModules();
   for (auto i : loadedModules) {
      auto loadedModule = i.second;
      if (loadedModule == appModule) {
         // The app entrypoint is called last, below.
         continue;
      }

      if (loadedModule->entryPoint && !loadedModule->entryCalled) {
         gLog->info("Executing module {} rpl_entry", loadedModule->name);

         loadedModule->entryCalled = true;

         auto moduleStart = kernel::loader::RplEntryPoint(loadedModule->entryPoint);
         moduleStart(loadedModule->handle, kernel::loader::RplEntryReasonLoad);
      }
   }

   gLog->info("Executing application start");
   auto result = startFn(argc, argv);

   // Try call coreinit::exit with return code
   auto coreinitModule = kernel::loader::findModule("coreinit");

   if (coreinitModule) {
      auto exitFn = coreinitModule->findFuncExport<void, int>("exit");

      if (exitFn) {
         exitFn(result);
      }
   }
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
   OSInitThreadQueue(sActiveThreads);

   for (auto i = 0; i < 3; ++i) {
      sSchedulerEnabled[i] = true;
      sCurrentThread[i] = nullptr;
      sCoreRunQueue[i] = coreinit::internal::sysAlloc<OSThreadQueue>();
      OSInitThreadQueue(sCoreRunQueue[i]);
      sLastSwitchTime[i] = std::chrono::high_resolution_clock::now();
      sCorePauseTime[i] = std::chrono::time_point<std::chrono::high_resolution_clock>::max();
   }
}

} // namespace coreinit
