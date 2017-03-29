#include "coreinit.h"
#include "coreinit_alarm.h"
#include "coreinit_core.h"
#include "coreinit_dynload.h"
#include "coreinit_fastmutex.h"
#include "coreinit_ghs.h"
#include "coreinit_interrupts.h"
#include "coreinit_memheap.h"
#include "coreinit_mutex.h"
#include "coreinit_scheduler.h"
#include "coreinit_systeminfo.h"
#include "coreinit_thread.h"
#include "kernel/kernel_loader.h"
#include "libcpu/mem.h"
#include "libcpu/cpu.h"
#include "ppcutils/stackobject.h"
#include "ppcutils/wfunc_call.h"
#include "usermodule.h"
#include <common/decaf_assert.h>
#include <array>
#include <limits>

namespace coreinit
{

const uint32_t
OSThread::Tag;

static std::array<OSThread *, CoreCount>
sDefaultThreads = { nullptr, nullptr, nullptr };

static uint32_t
sThreadId = 1;

static AlarmCallback
sSleepAlarmHandler = nullptr;

static OSThreadEntryPointFn
sDeallocatorThreadEntryPoint;

static std::array<OSThread *, CoreCount>
sDeallocatorThread;

static std::array<OSThreadQueue *, CoreCount>
sDeallocatorThreadQueue;

static std::array<OSThreadQueue *, CoreCount>
sDeallocationQueue;

void
__OSClearThreadStack32(OSThread *thread,
                       uint32_t value)
{
   be_val<uint32_t> *clearStart, *clearEnd;

   if (OSGetCurrentThread() == thread) {
      clearStart = thread->stackEnd + 4;
      clearEnd = mem::translate<be_val<uint32_t>>(OSGetStackPointer());
   } else {
      // We assume that the thread must be paused while this is happening...
      // This might be a bad assumption to make, but otherwise we run
      // into some sketchy race conditions.
      clearStart = thread->stackEnd + 4;
      clearEnd = mem::translate<be_val<uint32_t>>(thread->context.gpr[1]);
   }

   for (auto addr = clearStart; addr < clearEnd; addr += 4) {
      *addr = value;
   }
}


/**
 * Cancels a thread.
 *
 * This sets the threads requestFlag to OSThreadRequest::Cancel, the thread will
 * be terminated next time OSTestThreadCancel is called.
 */
void
OSCancelThread(OSThread *thread)
{
   bool reschedule = false;
   internal::lockScheduler();

   if (thread->requestFlag == OSThreadRequest::Suspend) {
      internal::wakeupThreadWaitForSuspensionNoLock(&thread->suspendQueue, -1);
      reschedule = true;
   }

   if (thread->suspendCounter != 0) {
      if (thread->cancelState == OSThreadCancelState::Enabled) {
         internal::resumeThreadNoLock(thread, thread->suspendCounter);
         reschedule = true;
      }
   }

   if (reschedule) {
      internal::rescheduleAllCoreNoLock();
   }

   thread->suspendCounter = 0;
   thread->needSuspend = 0;
   thread->requestFlag = OSThreadRequest::Cancel;
   internal::unlockScheduler();

   if (OSGetCurrentThread() == thread) {
      if (thread->cancelState == OSThreadCancelState::Enabled) {
         OSExitThread(-1);
      }
   }
}


/**
 * Returns the count of active threads.
 */
int32_t
OSCheckActiveThreads()
{
   internal::lockScheduler();
   OSThread *thread = OSGetCurrentThread();

   auto threadCount = internal::checkActiveThreadsNoLock();

   internal::unlockScheduler();
   return threadCount;
}


/**
 * Get the maximum amount of stack the thread has used.
 */
int32_t
OSCheckThreadStackUsage(OSThread *thread)
{
   be_val<uint32_t> *addr;
   internal::lockScheduler();

   for (addr = thread->stackEnd + 4; addr < thread->stackStart; addr += 4) {
      if (*addr != 0xfefefefe) {
         break;
      }
   }

   auto result = static_cast<int32_t>(thread->stackStart.getAddress()) - static_cast<int32_t>(mem::untranslate(addr));
   internal::unlockScheduler();
   return result;
}


/**
 * Disable tracking of thread stack usage
 */
void
OSClearThreadStackUsage(OSThread *thread)
{
   internal::lockScheduler();

   if (!thread) {
      thread = OSGetCurrentThread();
   }

   thread->attr &= ~OSThreadAttributes::StackUsage;
   internal::unlockScheduler();
}


/**
 * Clears a thread's suspend counter and resumes it.
 */
void
OSContinueThread(OSThread *thread)
{
   internal::lockScheduler();
   internal::resumeThreadNoLock(thread, thread->suspendCounter);
   internal::rescheduleAllCoreNoLock();
   internal::unlockScheduler();
}


/**
 * Setup thread run state, shared by OSRunThread and OSCreateThread
 */
static void
InitialiseThreadState(OSThread *thread,
                      uint32_t entry,
                      uint32_t argc,
                      void *argv)
{
   auto module = kernel::getUserModule();
   auto sdaBase = module ? module->sdaBase : 0u;
   auto sda2Base = module ? module->sda2Base : 0u;

   decaf_check(thread->context.fiber == nullptr);

   // Setup context
   thread->context.gpr[0] = 0;
   thread->context.gpr[1] = thread->stackStart.getAddress() - 8;
   thread->context.gpr[2] = sda2Base;
   thread->context.gpr[3] = argc;
   thread->context.gpr[4] = mem::untranslate(argv);
   thread->context.gpr[13] = sdaBase;
   thread->context.fpscr = 4;
   thread->context.gqr[2] = 0x40004;
   thread->context.gqr[3] = 0x50005;
   thread->context.gqr[4] = 0x60006;
   thread->context.gqr[5] = 0x70007;

   // Clear the backchain to 0
   auto stackPtr = mem::translate<uint32_t>(thread->context.gpr[1]);
   stackPtr[0] = 0;
   stackPtr[1] = 0;

   // Setup context Decaf special stuff
   thread->context.nia = entry;
   thread->context.cia = 0xFFFFFFFF;

   // Setup thread
   thread->entryPoint = entry;
   thread->state = entry ? OSThreadState::Ready : OSThreadState::None;
   thread->requestFlag = OSThreadRequest::None;
   thread->suspendCounter = 1;
   thread->needSuspend = 0;
}


/**
 * Create a new thread.
 *
 * \param thread Thread to initialise.
 * \param entry Thread entry point.
 * \param argc argc argument passed to entry point.
 * \param argv argv argument passed to entry point.
 * \param stack Top of stack (highest address).
 * \param stackSize Size of stack.
 * \param priority Thread priority, 0 is highest priorty, 31 is lowest.
 * \param attributes Thread attributes, see OSThreadAttributes.
 */
BOOL
OSCreateThread(OSThread *thread,
               OSThreadEntryPointFn entry,
               uint32_t argc,
               void *argv,
               be_val<uint32_t> *stack,
               uint32_t stackSize,
               int32_t priority,
               OSThreadAttributes attributes)
{
   // If no affinity is defined, we need to copy the affinity from the calling thread
   if ((attributes & OSThreadAttributes::AffinityAny) == 0) {
      auto curAttr = internal::getCurrentThread()->attr;
      uint32_t newAttr = attributes | (curAttr & OSThreadAttributes::AffinityAny);
      attributes = static_cast<OSThreadAttributes>(newAttr);
   }

   // Setup OSThread
   memset(thread, 0, sizeof(OSThread));
   thread->tag = OSThread::Tag;
   thread->userStackPointer = stack;
   thread->stackStart = stack;
   thread->stackEnd = reinterpret_cast<be_val<uint32_t>*>(reinterpret_cast<uint8_t*>(stack) - stackSize);
   thread->basePriority = priority;
   thread->priority = thread->basePriority;
   thread->attr = attributes;

   // Write magic stack ending!
   *thread->stackEnd = 0xDEADBABE;

   // Setup thread state
   internal::lockScheduler();
   InitialiseThreadState(thread, entry, argc, argv);
   thread->id = sThreadId++;

   if (entry) {
      internal::markThreadActiveNoLock(thread);
   }

   internal::unlockScheduler();

   gLog->info("Thread Created: ptr {:08x}, id {:x}, basePriority {}, attr {:08x}, entry {:08x}, stackStart {:08x}, stackEnd {:08x}",
      mem::untranslate(thread), static_cast<uint16_t>(thread->id),
      static_cast<int32_t>(thread->basePriority), static_cast<uint32_t>(thread->attr),
      entry, thread->stackStart.getAddress(), thread->stackEnd.getAddress());

   return TRUE;
}


/**
 * Detach thread.
 */
void
OSDetachThread(OSThread *thread)
{
   internal::lockScheduler();

   // HACK: Unfortunately this check is not valid in all games.  One Piece performs
   //  OSJoinThread on a thread, and then subsequently calls OSDetachThread on it
   //  for whetever reason.  Coreinit doesnt check this, so we can't do this check.
   //decaf_check(internal::isThreadActiveNoLock(thread));

   thread->attr |= OSThreadAttributes::Detached;

   if (thread->state == OSThreadState::Moribund) {
      // Thread has already ended so we can remove it from the active list
      internal::markThreadInactiveNoLock(thread);

      if (thread->deallocator) {
         internal::queueThreadDeallocation(thread);
      }

      thread->state = OSThreadState::None;
      // TODO: thread->id = 0x8000;
   }

   internal::wakeupThreadNoLock(&thread->joinQueue);
   internal::rescheduleAllCoreNoLock();
   internal::unlockScheduler();
}


/**
 * Exit the current thread with a exit code.
 *
 * This function is implicitly called when the thread entry point returns.
 */
void
OSExitThread(int value)
{
   auto thread = OSGetCurrentThread();

   // Call any thread cleanup callbacks
   if (thread->cleanupCallback) {
      thread->cancelState |= OSThreadCancelState::Disabled;
      thread->cleanupCallback(thread, thread->stackEnd.get());
   }

   // Cleanup the GHS exceptions we previously created
   internal::ghsCleanupExceptions();

   // Free any TLS data which was allocated to this thread
   if (thread->tlsSections) {
      internal::dynLoadTLSFree(thread->tlsSections);
      thread->tlsSectionCount = 0;
      thread->tlsSections = 0;
   }

   // Disable interrupts and lock the scheduler
   auto oldInterrupts = OSDisableInterrupts();
   internal::lockScheduler();

   // Actually proccess the thread exit
   internal::exitThreadNoLock(value);

   // We should never reach here.  The scheduler handles unlocking
   //  itself and restoring the interrupt state.
   decaf_abort("exitThreadNoLock returned");
}


/**
 * Get the next and previous thread in the thread's active queue.
 */
void
OSGetActiveThreadLink(OSThread *thread,
                      OSThreadLink *link)
{
   *link = thread->activeLink;
}


/**
 * Return pointer to OSThread object for the current thread.
 */
OSThread *
OSGetCurrentThread()
{
   return internal::getCurrentThread();
}


/**
 * Returns the default thread for a specific core.
 *
 * Each core has 1 default thread created before the game boots. The default
 * thread for core 1 calls the RPX entry point, the default threads for core 0
 * and 2 are suspended and can be used with OSRunThread.
 */
OSThread *
OSGetDefaultThread(uint32_t coreID)
{
   if (coreID >= CoreCount) {
      return nullptr;
   }

   return sDefaultThreads[coreID];
}


/**
 * Return current stack pointer, value of r1 register.
 */
uint32_t
OSGetStackPointer()
{
   return cpu::this_core::state()->gpr[1];
}


/**
 * Get a thread's affinity.
 */
uint32_t
OSGetThreadAffinity(OSThread *thread)
{
   return thread->attr & OSThreadAttributes::AffinityAny;
}


/**
 * Get a thread's name.
 */
const char *
OSGetThreadName(OSThread *thread)
{
   return thread->name;
}


/**
 * Get a thread's base priority.
 */
uint32_t
OSGetThreadPriority(OSThread *thread)
{
   return thread->basePriority;
}


/**
 * Get a thread's specific value set by OSSetThreadSpecific.
 */
uint32_t
OSGetThreadSpecific(uint32_t id)
{
   decaf_check(id >= 0 && id < 0x10);
   return OSGetCurrentThread()->specific[id];
}


/**
* Initialise a thread queue object.
*/
void
OSInitThreadQueue(OSThreadQueue *queue)
{
   OSInitThreadQueueEx(queue, nullptr);
}


/**
* Initialise a thread queue object with a parent.
*/
void
OSInitThreadQueueEx(OSThreadQueue *queue,
                    void *parent)
{
   queue->head = nullptr;
   queue->tail = nullptr;
   queue->parent = parent;
}


/**
 * Returns TRUE if a thread is suspended.
 */
BOOL
OSIsThreadSuspended(OSThread *thread)
{
   return thread->suspendCounter > 0;
}


/**
 * Returns TRUE if a thread is terminated.
 */
BOOL
OSIsThreadTerminated(OSThread *thread)
{
   return thread->state == OSThreadState::None
       || thread->state == OSThreadState::Moribund;
}


/**
 * Wait until thread is terminated.
 *
 * \param thread Thread to wait for
 * \param exitValue Pointer to store thread exit value in.
 * \returns Returns TRUE if thread has terminated, FALSE otherwise.
 */
BOOL
OSJoinThread(OSThread *thread,
             be_val<int> *exitValue)
{
   internal::lockScheduler();
   decaf_check(thread);
   decaf_check(internal::isThreadActiveNoLock(thread));

   // If the thread has not ended, let's wait for it
   //  note only one thread is allowed in the join queue
   if (thread->state != OSThreadState::Moribund && !thread->joinQueue.head) {
      internal::sleepThreadNoLock(&thread->joinQueue);
      internal::rescheduleSelfNoLock();

      if (!internal::isThreadActiveNoLock(thread)) {
         // This would only happen for detached threads.
         internal::unlockScheduler();
         return FALSE;
      }
   }

   if (thread->state != OSThreadState::Moribund) {
      internal::unlockScheduler();
      return FALSE;
   }

   if (exitValue) {
      *exitValue = thread->exitValue;
   }

   internal::markThreadInactiveNoLock(thread);
   thread->state = OSThreadState::None;

   if (thread->deallocator) {
      internal::queueThreadDeallocation(thread);
      internal::rescheduleSelfNoLock();
   }

   internal::unlockScheduler();
   return TRUE;
}


void
OSPrintCurrentThreadState()
{
   auto thread = OSGetCurrentThread();

   if (!thread) {
      return;
   }

   auto state = cpu::this_core::state();

   fmt::MemoryWriter out;
   out.write("id   = {}\n", thread->id);

   if (thread->name) {
      out.write("name  = {}\n", thread->name.get());
   }

   out.write("cia   = 0x{:08X}\n", state->cia);
   out.write("lr    = 0x{:08X}\n", state->lr);
   out.write("cr    = 0x{:08X}\n", state->cr.value);
   out.write("xer   = 0x{:08X}\n", state->xer.value);
   out.write("ctr   = 0x{:08X}\n", state->ctr);

   for (auto i = 0u; i < 32; ++i) {
      out.write("r{:<2}   = 0x{:08X}\n", i, state->gpr[i]);
   }

   out.write("fpscr = 0x{:08X}\n", state->fpscr.value);

   for (auto i = 0u; i < 32; ++i) {
      out.write("f{:<2}   = {}\n", i, state->fpr[i].value);
   }

   for (auto i = 0u; i < 32; ++i) {
      out.write("ps{:<2}   = {:<16} ps{:<2}   = {}\n", i, state->fpr[i].paired0, i, state->fpr[i].paired1);
   }

   gLog->info(out.str());
}


/**
 * Resumes a thread.
 *
 * Decrements the thread's suspend counter, if the counter reaches 0 the thread
 * is resumed.
 *
 * \returns Returns the previous value of the suspend counter.
 */
int32_t
OSResumeThread(OSThread *thread)
{
   internal::lockScheduler();
   auto oldSuspendCounter = internal::resumeThreadNoLock(thread, 1);

   if (oldSuspendCounter - 1 == 0) {
      internal::rescheduleAllCoreNoLock();
   }

   internal::unlockScheduler();
   return oldSuspendCounter;
}


/**
 * Run a function on an already created thread.
 *
 * Can only be used on idle threads.
 */
BOOL
OSRunThread(OSThread *thread,
            OSThreadEntryPointFn entry,
            uint32_t argc,
            void *argv)
{
   BOOL result = FALSE;
   internal::lockScheduler();

   if (OSIsThreadTerminated(thread)) {
      if (thread->state == OSThreadState::Moribund) {
         internal::markThreadInactiveNoLock(thread);
      }

      InitialiseThreadState(thread, entry, argc, argv);
      internal::markThreadActiveNoLock(thread);
      internal::resumeThreadNoLock(thread, 1);
      internal::rescheduleAllCoreNoLock();
      result = TRUE;
   }

   internal::unlockScheduler();
   return result;
}


/**
 * Set a thread's affinity.
 */
BOOL
OSSetThreadAffinity(OSThread *thread,
                    uint32_t affinity)
{
   internal::lockScheduler();
   internal::setThreadAffinityNoLock(thread, affinity);

   if (thread->state == OSThreadState::Ready && affinity != 0) {
      internal::rescheduleAllCoreNoLock();
   }

   internal::unlockScheduler();
   return TRUE;
}


/**
 * Set a thread's cancellation state.
 */
BOOL
OSSetThreadCancelState(BOOL cancelEnabled)
{
   auto thread = OSGetCurrentThread();
   auto oldCancelEnabled = TRUE;

   if (thread->cancelState & OSThreadCancelState::Disabled) {
      oldCancelEnabled = FALSE;
   }

   if (cancelEnabled) {
      thread->cancelState &= ~OSThreadCancelState::Disabled;
   } else {
      thread->cancelState |= OSThreadCancelState::Disabled;
   }

   return oldCancelEnabled;
}


/**
 * Set the callback to be called just before a thread is terminated.
 *
 * \return Returns the previous callback function.
 */
OSThreadCleanupCallbackFn
OSSetThreadCleanupCallback(OSThread *thread,
                           OSThreadCleanupCallbackFn callback)
{
   internal::lockScheduler();
   auto old = thread->cleanupCallback;
   thread->cleanupCallback = callback;
   internal::unlockScheduler();
   return old;
}


/**
 * Set the callback to be called just after a thread is terminated.
 */
OSThreadDeallocatorFn
OSSetThreadDeallocator(OSThread *thread,
                       OSThreadDeallocatorFn deallocator)
{
   internal::lockScheduler();
   auto old = thread->deallocator;
   thread->deallocator = deallocator;
   internal::unlockScheduler();
   return old;
}


/**
 * Set a thread's name.
 */
void
OSSetThreadName(OSThread *thread,
                const char *name)
{
   thread->name = name;
}


/**
 * Set a thread's priority.
 */
BOOL
OSSetThreadPriority(OSThread *thread,
                    uint32_t priority)
{
   if (priority > 31) {
      return FALSE;
   }

   internal::lockScheduler();
   thread->basePriority = priority;
   internal::updateThreadPriorityNoLock(thread);
   internal::rescheduleAllCoreNoLock();
   internal::unlockScheduler();
   return TRUE;
}


/**
 * Set a thread's run quantum.
 *
 * This is the maximum amount of time the thread can run for before being forced
 * to yield.
 */
BOOL
OSSetThreadRunQuantum(OSThread *thread,
                      uint32_t quantum)
{
   decaf_abort("Unsupported call to OSSetThreadRunQuantum");
   return FALSE;
}


/**
 * Set a thread specific value.
 *
 * Can be read with OSGetThreadSpecific.
 */
void
OSSetThreadSpecific(uint32_t id,
                    uint32_t value)
{
   OSGetCurrentThread()->specific[id] = value;
}


/**
 * Set thread stack usage tracking.
 */
BOOL
OSSetThreadStackUsage(OSThread *thread)
{
   internal::lockScheduler();

   if (!thread) {
      thread = OSGetCurrentThread();
   } else if (thread->state == OSThreadState::Running) {
      internal::unlockScheduler();
      return FALSE;
   }

   __OSClearThreadStack32(thread, 0xfefefefe);
   thread->attr |= OSThreadAttributes::StackUsage;
   internal::unlockScheduler();
   return TRUE;
}


/**
 * Sleep the current thread and add it to a thread queue.
 *
 * Will sleep until the thread queue is woken with OSWakeupThread.
 */
void
OSSleepThread(OSThreadQueue *queue)
{
   internal::lockScheduler();
   internal::sleepThreadNoLock(queue);
   internal::rescheduleSelfNoLock();
   internal::unlockScheduler();
}

static void
SleepAlarmHandler(OSAlarm *alarm, OSContext *context)
{
   // Wakeup the thread waiting on this alarm
   auto data = reinterpret_cast<OSThread*>(OSGetAlarmUserData(alarm));

   // System Alarm, we already have the scheduler lock
   internal::wakeupOneThreadNoLock(data);
}

/**
 * Sleep the current thread for a period of time.
 */
void
OSSleepTicks(OSTime ticks)
{
   // Create an alarm to trigger wakeup
   ppcutils::StackObject<OSAlarm> alarm;
   ppcutils::StackObject<OSThreadQueue> queue;

   OSCreateAlarm(alarm);
   OSInitThreadQueue(queue);

   internal::lockScheduler();
   internal::setAlarmInternal(alarm, ticks, sSleepAlarmHandler, OSGetCurrentThread());

   internal::sleepThreadNoLock(queue);
   internal::rescheduleSelfNoLock();

   internal::unlockScheduler();
}


/**
 * Suspend a thread.
 *
 * Increases a thread's suspend counter, if the counter is >0 then the thread is
 * suspended.
 *
 * \returns Returns the thread's previous suspend counter value
 */
uint32_t
OSSuspendThread(OSThread *thread)
{
   internal::lockScheduler();
   int32_t result = -1;

   if (thread->state == OSThreadState::Moribund || thread->state == OSThreadState::None) {
      internal::unlockScheduler();
      return -1;
   }

   if (thread->requestFlag == OSThreadRequest::Cancel) {
      internal::unlockScheduler();
      return -1;
   }

   auto curThread = OSGetCurrentThread();

   if (curThread == thread) {
      if (thread->cancelState == OSThreadCancelState::Enabled) {
         thread->needSuspend++;
         result = thread->suspendCounter;
         internal::suspendThreadNoLock(thread);
         internal::rescheduleAllCoreNoLock();
      }
   } else {
      if (thread->suspendCounter != 0) {
         result = thread->suspendCounter++;
      } else {
         thread->needSuspend++;
         thread->requestFlag = OSThreadRequest::Suspend;
         internal::sleepThreadNoLock(&thread->suspendQueue);
         internal::rescheduleSelfNoLock();
         result = thread->suspendResult;
      }
   }

   internal::unlockScheduler();
   return result;
}


/**
 * Check to see if the current thread should be cancelled or suspended.
 *
 * This is implicitly called in:
 * - OSLockMutex
 * - OSTryLockMutex
 * - OSUnlockMutex
 * - OSAcquireSpinLock
 * - OSTryAcquireSpinLock
 * - OSTryAcquireSpinLockWithTimeout
 * - OSReleaseSpinLock
 * - OSCancelThread
 */
void
OSTestThreadCancel()
{
   internal::lockScheduler();
   internal::testThreadCancelNoLock();
   internal::unlockScheduler();
}


/**
 * Wake up all threads in queue.
 *
 * Clears the thread queue.
 */
void
OSWakeupThread(OSThreadQueue *queue)
{
   internal::lockScheduler();
   internal::wakeupThreadNoLock(queue);
   internal::rescheduleAllCoreNoLock();
   internal::unlockScheduler();
}


/**
 * Yield execution to waiting threads with same priority.
 *
 * This will never switch to a thread with a lower priority than the current
 * thread.
 */
void
OSYieldThread()
{
   internal::lockScheduler();
   internal::checkRunningThreadNoLock(true);
   internal::unlockScheduler();
}


/**
 * Gets the TLS data for tls_index.
 */
void *
tls_get_addr(tls_index *index)
{
   auto thread = OSGetCurrentThread();
   auto module = kernel::getTLSModule(index->moduleIndex);
   decaf_check(module);

   if (thread->tlsSectionCount <= index->moduleIndex) {
      auto oldSections = thread->tlsSections;
      auto oldCount = thread->tlsSectionCount;
      thread->tlsSectionCount = index->moduleIndex + 1;

      // Allocate and zero new tlsSections
      internal::dynLoadTLSAlloc(thread->tlsSectionCount * sizeof(OSTLSSection), 4, reinterpret_cast<be_ptr<void> *>(&thread->tlsSections));
      std::memset(&thread->tlsSections[oldCount], 0, sizeof(OSTLSSection) * (thread->tlsSectionCount - oldCount));

      // Copy and free old tlsSections
      if (oldSections) {
         std::memcpy(thread->tlsSections, oldSections, oldCount * sizeof(OSTLSSection));
         internal::dynLoadTLSFree(oldSections);
      }
   }

   auto &section = thread->tlsSections[index->moduleIndex];

   if (!section.data) {
      // Allocate and copy TLS data
      internal::dynLoadTLSAlloc(module->tlsSize, 1u << module->tlsAlignShift, &section.data);
      std::memcpy(section.data, mem::translate<void>(module->tlsBase), module->tlsSize);
   }

   return mem::translate<void *>(section.data.getAddress() + index->offset);
}

uint32_t
DeallocatorThreadEntry(uint32_t coreId, void *arg2)
{
   auto &waitQueue = sDeallocatorThreadQueue[coreId];
   auto &queue = sDeallocationQueue[coreId];

   auto oldInterrupts = OSDisableInterrupts();

   while (true) {
      auto thread = internal::ThreadQueue::popFront(queue);

      if (!thread) {
         internal::lockScheduler();
         internal::sleepThreadNoLock(waitQueue);
         internal::rescheduleSelfNoLock();
         internal::unlockScheduler();
         continue;
      }

      if (thread->deallocator) {
         OSRestoreInterrupts(oldInterrupts);

         thread->deallocator(thread, thread->stackEnd);

         oldInterrupts = OSDisableInterrupts();
      }
   }

   OSRestoreInterrupts(oldInterrupts);
}

void
Module::registerThreadFunctions()
{
   RegisterKernelFunction(OSCancelThread);
   RegisterKernelFunction(OSCheckActiveThreads);
   RegisterKernelFunction(OSCheckThreadStackUsage);
   RegisterKernelFunction(OSClearThreadStackUsage);
   RegisterKernelFunction(OSContinueThread);
   RegisterKernelFunction(OSCreateThread);
   RegisterKernelFunction(OSDetachThread);
   RegisterKernelFunction(OSExitThread);
   RegisterKernelFunction(OSGetActiveThreadLink);
   RegisterKernelFunction(OSGetCurrentThread);
   RegisterKernelFunction(OSGetDefaultThread);
   RegisterKernelFunction(OSGetStackPointer);
   RegisterKernelFunction(OSGetThreadAffinity);
   RegisterKernelFunction(OSGetThreadName);
   RegisterKernelFunction(OSGetThreadPriority);
   RegisterKernelFunction(OSGetThreadSpecific);
   RegisterKernelFunction(OSInitThreadQueue);
   RegisterKernelFunction(OSInitThreadQueueEx);
   RegisterKernelFunction(OSIsThreadSuspended);
   RegisterKernelFunction(OSIsThreadTerminated);
   RegisterKernelFunction(OSJoinThread);
   RegisterKernelFunction(OSPrintCurrentThreadState);
   RegisterKernelFunction(OSResumeThread);
   RegisterKernelFunction(OSRunThread);
   RegisterKernelFunction(OSSetThreadAffinity);
   RegisterKernelFunction(OSSetThreadCancelState);
   RegisterKernelFunction(OSSetThreadCleanupCallback);
   RegisterKernelFunction(OSSetThreadDeallocator);
   RegisterKernelFunction(OSSetThreadName);
   RegisterKernelFunction(OSSetThreadPriority);
   RegisterKernelFunction(OSSetThreadRunQuantum);
   RegisterKernelFunction(OSSetThreadSpecific);
   RegisterKernelFunction(OSSetThreadStackUsage);
   RegisterKernelFunction(OSSleepThread);
   RegisterKernelFunction(OSSleepTicks);
   RegisterKernelFunction(OSSuspendThread);
   RegisterKernelFunction(OSTestThreadCancel);
   RegisterKernelFunction(OSWakeupThread);
   RegisterKernelFunction(OSYieldThread);
   RegisterKernelFunctionName("__tls_get_addr", tls_get_addr);

   RegisterInternalFunction(SleepAlarmHandler, sSleepAlarmHandler);
   RegisterInternalFunction(DeallocatorThreadEntry, sDeallocatorThreadEntryPoint);
   RegisterInternalData(sDeallocatorThreadQueue);
   RegisterInternalData(sDeallocationQueue);
   RegisterInternalData(sDeallocatorThread);
}

namespace internal
{


/**
 * Set the current thread to run only on the current core.
 *
 * \return
 * Returns the old thread affinity, to be restored with unpinThreadAffinity.
 */
uint32_t
pinThreadAffinity()
{
   auto core = OSGetCoreId();
   auto thread = OSGetCurrentThread();
   internal::lockScheduler();

   auto oldAffinity = thread->attr & OSThreadAttributes::AffinityAny;
   thread->attr &= ~OSThreadAttributes::AffinityAny;
   thread->attr |= 1 << core;

   internal::unlockScheduler();
   return oldAffinity;
}


/**
 * Restores the thread affinity.
 */
void
unpinThreadAffinity(uint32_t affinity)
{
   auto thread = OSGetCurrentThread();
   internal::lockScheduler();
   thread->attr &= ~OSThreadAttributes::AffinityAny;
   thread->attr |= affinity;
   internal::unlockScheduler();
}


void
exitThreadNoLock(int value)
{
   auto thread = OSGetCurrentThread();

   decaf_check(thread->state == OSThreadState::Running);
   decaf_check(internal::isThreadActiveNoLock(thread));

   // Clear the context associated with this thread

   if (thread->attr & OSThreadAttributes::Detached) {
      internal::markThreadInactiveNoLock(thread);
      thread->state = OSThreadState::None;
      // TODO: thread->id = 0x8000;

      if (thread->deallocator) {
         queueThreadDeallocation(thread);
      }
   } else {
      thread->exitValue = value;
      thread->state = OSThreadState::Moribund;
   }

   internal::disableScheduler();
   internal::unlockAllMutexNoLock(thread);
   internal::unlockAllFastMutexNoLock(thread);
   internal::wakeupThreadNoLock(&thread->joinQueue);
   internal::wakeupThreadWaitForSuspensionNoLock(&thread->suspendQueue, -1);
   internal::rescheduleAllCoreNoLock();
   internal::enableScheduler();

   kernel::exitThreadNoLock();
   internal::rescheduleSelfNoLock();

   // We do not need to unlockScheduler as OSExitThread never returns.
   decaf_abort("Exited thread was rcheduled...");
}

void
queueThreadDeallocation(OSThread *thread)
{
   auto coreId = cpu::this_core::id();
   ThreadQueue::insert(sDeallocationQueue[coreId], thread);
   wakeupThreadNoLock(sDeallocatorThreadQueue[coreId]);
}

void
startDeallocatorThreads()
{
   for (auto i = 0; i < CoreCount; ++i) {
      OSInitThreadQueue(sDeallocatorThreadQueue[i]);
      OSInitThreadQueue(sDeallocationQueue[i]);

      auto &thread = sDeallocatorThread[i];
      auto stackSize = 16 * 1024;
      auto stack = reinterpret_cast<uint8_t*>(coreinit::internal::sysAlloc(stackSize, 8));
      auto name = coreinit::internal::sysStrDup(fmt::format("Thread Deallocator Thread {}", i));

      OSCreateThread(thread, sDeallocatorThreadEntryPoint, i, nullptr,
         reinterpret_cast<be_val<uint32_t>*>(stack + stackSize), stackSize, -1,
         static_cast<OSThreadAttributes>(1 << i));
      OSSetThreadName(thread, name);
      OSResumeThread(thread);
   }
}

void
setDefaultThread(uint32_t core,
                 OSThread *thread)
{
   sDefaultThreads[core] = thread;
}

bool threadSortFunc(OSThread *lhs, OSThread *rhs)
{
   return lhs->priority <= rhs->priority;
}

} // namespace internal

} // namespace coreinit
