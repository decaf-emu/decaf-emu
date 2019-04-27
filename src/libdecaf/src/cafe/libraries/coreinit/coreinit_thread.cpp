#include "coreinit.h"
#include "coreinit_alarm.h"
#include "coreinit_core.h"
#include "coreinit_context.h"
#include "coreinit_cosreport.h"
#include "coreinit_dynload.h"
#include "coreinit_enum_string.h"
#include "coreinit_fastmutex.h"
#include "coreinit_ghs.h"
#include "coreinit_lockedcache.h"
#include "coreinit_ipcdriver.h"
#include "coreinit_interrupts.h"
#include "coreinit_memheap.h"
#include "coreinit_memory.h"
#include "coreinit_mutex.h"
#include "coreinit_rendezvous.h"
#include "coreinit_scheduler.h"
#include "coreinit_systeminfo.h"
#include "coreinit_thread.h"

#include "cafe/cafe_stackobject.h"
#include "cafe/kernel/cafe_kernel.h"
#include "cafe/kernel/cafe_kernel_context.h"
#include "cafe/libraries/cafe_hle.h"

#include <array>
#include <common/decaf_assert.h>
#include <common/log.h>
#include <fmt/format.h>
#include <libcpu/cpu.h>
#include <limits>

namespace cafe::coreinit
{

static uint16_t
sThreadId = 1;

static AlarmCallbackFn
sSleepAlarmHandler = nullptr;

static OSThreadEntryPointFn
sThreadEntryPoint = nullptr;

static OSThreadEntryPointFn
sDeallocatorThreadEntryPoint = nullptr;

static OSThreadEntryPointFn
sDefaultThreadEntryPoint = nullptr;

constexpr auto DeallocatorThreadSize = 0x2000u;

struct StaticThreadData
{
   struct PerCoreData
   {
      be2_struct<OSThread> defaultThread;
      be2_array<char, 32> defaultThreadName;
      be2_struct<OSAlarm> timeSliceAlarm;
      be2_struct<OSThreadQueue> deallocationQueue;
      be2_struct<OSThreadQueue> deallocationThreadQueue;
      be2_struct<OSThread> deallocatorThread;
      be2_array<uint8_t, DeallocatorThreadSize> deallocatorThreadStack;
      be2_array<char, 40> deallocatorThreadName;
   };

   be2_array<PerCoreData, OSGetCoreCount()> perCoreData;
   be2_struct<OSRendezvous> defaultThreadInitRendezvous;
   be2_val<uint32_t> defaultThreadInitRendezvousWaitMask;
};

static virt_ptr<StaticThreadData>
sThreadData = nullptr;

static void
clearThreadStackWithValue(virt_ptr<OSThread> thread,
                          uint32_t value)
{
   auto clearStart = virt_ptr<uint32_t> { nullptr };
   auto clearEnd = virt_ptr<uint32_t> { nullptr };

   if (OSGetCurrentThread() == thread) {
      clearStart = thread->stackEnd + 4;
      clearEnd = virt_cast<uint32_t *>(OSGetStackPointer());
   } else {
      // We assume that the thread must be paused while this is happening...
      // This might be a bad assumption to make, but otherwise we run
      // into some sketchy race conditions.
      clearStart = thread->stackEnd + 4;
      clearEnd = virt_cast<uint32_t *>(virt_addr { thread->context.gpr[1].value() });
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
OSCancelThread(virt_ptr<OSThread> thread)
{
   bool reschedule = false;
   internal::lockScheduler();

   if (thread->requestFlag == OSThreadRequest::Suspend) {
      internal::wakeupThreadWaitForSuspensionNoLock(
         virt_addrof(thread->suspendQueue),
         -1);
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
   virt_ptr<OSThread> thread = OSGetCurrentThread();

   auto threadCount = internal::checkActiveThreadsNoLock();

   internal::unlockScheduler();
   return threadCount;
}


/**
 * Get the maximum amount of stack the thread has used.
 */
int32_t
OSCheckThreadStackUsage(virt_ptr<OSThread> thread)
{
   auto addr = virt_ptr<uint32_t> { nullptr };
   internal::lockScheduler();

   for (addr = thread->stackEnd + 4; addr < thread->stackStart; addr += 4) {
      if (*addr != 0xFEFEFEFE) {
         break;
      }
   }

   auto result = virt_cast<virt_addr>(thread->stackStart) - virt_cast<virt_addr>(addr);
   internal::unlockScheduler();
   return static_cast<int32_t>(result);
}


/**
 * Clear current stack with a value.
 */
void
OSClearStack(uint32_t value)
{
   auto thread = OSGetCurrentThread();
   auto stackTop = OSGetStackPointer();
   for (auto ptr = thread->stackEnd + 1; ptr < stackTop; ++ptr) {
      *ptr = value;
   }
}


/**
 * Disable tracking of thread stack usage
 */
void
OSClearThreadStackUsage(virt_ptr<OSThread> thread)
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
OSContinueThread(virt_ptr<OSThread> thread)
{
   internal::lockScheduler();
   internal::resumeThreadNoLock(thread, thread->suspendCounter);
   internal::rescheduleAllCoreNoLock();
   internal::unlockScheduler();
}


/**
 * Thread entry.
 */
static uint32_t
threadEntry(uint32_t argc,
            virt_ptr<void> argv)
{
   auto thread = OSGetCurrentThread();
   auto interruptsState = OSDisableInterrupts();
   internal::ghsExceptionInit(thread);
   OSRestoreInterrupts(interruptsState);
   return cafe::invoke(cpu::this_core::state(),
                       thread->entryPoint,
                       argc,
                       argv);
}


/**
 * Setup thread run state, shared by OSRunThread and OSCreateThread
 */
static void
initialiseThreadState(virt_ptr<OSThread> thread,
                      OSThreadEntryPointFn entry,
                      uint32_t argc,
                      virt_ptr<void> argv,
                      virt_ptr<void> stack,
                      uint32_t stackSize,
                      int32_t priority,
                      uint32_t pir,
                      OSThreadType type)
{
   // Setup thread state
   thread->priority = priority;
   thread->basePriority = priority;
   thread->tag = OSThread::Tag;
   thread->suspendResult = -1;
   thread->needSuspend = 0;
   thread->exitValue = -1;
   thread->type = type;
   thread->state = entry ? OSThreadState::Ready : OSThreadState::None;
   thread->mutex = nullptr;
   thread->deallocator = nullptr;
   thread->coreTimeConsumedNs = 0ull;
   thread->cleanupCallback = nullptr;
   thread->requestFlag = OSThreadRequest::None;
   thread->fastMutex = nullptr;
   thread->waitEventTimeoutAlarm = nullptr;
   thread->runQuantumTicks = 0ll;
   thread->cancelState = OSThreadCancelState::Enabled;
   thread->entryPoint = entry;
   thread->suspendCounter = entry ? 1 : 0;
   thread->eh_globals = nullptr;
   thread->eh_mem_manage.fill(nullptr);
   thread->eh_store_globals.fill(nullptr);
   thread->eh_store_globals_tdeh.fill(nullptr);
   thread->tlsSectionCount = uint16_t { 0u };
   thread->tlsSections = nullptr;
   thread->contendedFastMutexes.head = nullptr;
   thread->contendedFastMutexes.tail = nullptr;
   thread->mutexQueue.head = nullptr;
   thread->mutexQueue.tail = nullptr;
   thread->mutexQueue.parent = thread;
   thread->alarmCancelled = 0;
   thread->specific.fill(0u);
   thread->wakeCount = 0ull;
   thread->unk0x610 = 0ll;
   thread->unk0x618 = 0ll;
   thread->unk0x620 = 0x7FFFFFFFFFFFFFFFll;
   thread->unk0x628 = 0ll;
   OSInitThreadQueueEx(virt_addrof(thread->joinQueue), thread);
   OSInitThreadQueueEx(virt_addrof(thread->suspendQueue), thread);

   // Setup thread stack
   auto stackInit = virt_cast<uint32_t *>(align_down(virt_cast<virt_addr>(stack), 8));
   *(stackInit - 1) = 0u;
   *(stackInit - 2) = 0u;

   thread->stackStart = virt_cast<uint32_t *>(stack);
   thread->stackEnd = virt_cast<uint32_t *>(virt_cast<virt_addr>(stack) - stackSize);
   *thread->stackEnd = 0xDEADBABE;

   // Setup thread context
   OSInitContext(virt_addrof(thread->context),
                 virt_func_cast<virt_addr>(sThreadEntryPoint),
                 align_down(virt_cast<virt_addr>(stack), 8) - 8);

   thread->context.pir = pir;
   thread->context.lr = hle::getLibrary(hle::LibraryId::coreinit)->findSymbolAddress("OSExitThread");
   thread->context.gpr[3] = argc;
   thread->context.gpr[4] = static_cast<uint32_t>(virt_cast<virt_addr>(argv));
   thread->context.fpscr = 4u;
   thread->context.psf.fill(0.0);
   thread->context.fpr.fill(0.0);
   thread->context.gqr[2] = 0x40004u;
   thread->context.gqr[3] = 0x50005u;
   thread->context.gqr[4] = 0x60006u;
   thread->context.gqr[5] = 0x70007u;
   thread->context.coretime.fill(0);
}

static BOOL
createThread(virt_ptr<OSThread> thread,
             OSThreadEntryPointFn entry,
             uint32_t argc,
             virt_ptr<void> argv,
             virt_ptr<uint32_t> stack,
             uint32_t stackSize,
             int32_t priority,
             OSThreadAttributes attributes,
             OSThreadType type)
{
   auto currentThread = internal::getCurrentThread();

   // If no affinity is defined, we need to copy the affinity from the calling thread
   if ((attributes & OSThreadAttributes::AffinityAny) == 0) {
      auto curAttr = currentThread->attr;
      attributes = attributes | (curAttr & OSThreadAttributes::AffinityAny);
   }

   auto realPriority = priority;
   if (type == OSThreadType::Driver) {
      if (priority < 0 || priority >= 32) {
         decaf_abort("Thread priority was out of range");
      }

      realPriority = priority;
   } else if (type == OSThreadType::AppIo) {
      if (priority < 0 || priority >= 32) {
         decaf_abort("Thread priority was out of range");
      }

      realPriority = priority + 32;
   } else if (type == OSThreadType::App) {
      if (priority < 0 || priority >= 32) {
         decaf_abort("Thread priority was out of range");
      }

      realPriority = priority + 64;
   } else {
      return FALSE;
   }

   // Setup thread state
   internal::lockScheduler();
   std::memset(thread.get(), 0, sizeof(OSThread));
   initialiseThreadState(thread, entry, argc, argv, stack, stackSize, realPriority,
                         OSGetCoreId(), type);
   thread->name = nullptr;
   thread->context.attr = attributes & OSThreadAttributes::AffinityAny;
   thread->attr = attributes;
   thread->id = sThreadId++;
   thread->dsiCallback = currentThread->dsiCallback;
   thread->isiCallback = currentThread->isiCallback;
   thread->programCallback = currentThread->programCallback;
   thread->perfMonCallback = currentThread->perfMonCallback;
   thread->alignCallback = currentThread->alignCallback;

   // Copy FPU exception status
   thread->context.fpscr |= currentThread->context.fpscr & 0xF8;

   if (entry) {
      internal::markThreadActiveNoLock(thread);
   }

   internal::unlockScheduler();

   gLog->info("Thread Created: ptr {}, id 0x{:X}, basePriority {}, attr 0x{:02X}, entry {}, stackStart {}, stackEnd {}",
      thread, thread->id, thread->basePriority, thread->attr,
      virt_func_cast<virt_addr>(entry), thread->stackStart, thread->stackEnd);

   return TRUE;
}

BOOL
OSCreateThread(virt_ptr<OSThread> thread,
               OSThreadEntryPointFn entry,
               uint32_t argc,
               virt_ptr<void> argv,
               virt_ptr<uint32_t> stack,
               uint32_t stackSize,
               int32_t priority,
               OSThreadAttributes attributes)
{
   return createThread(thread, entry, argc, argv, stack, stackSize, priority, attributes, OSThreadType::App);
}

BOOL
OSCreateThreadType(virt_ptr<OSThread> thread,
                   OSThreadEntryPointFn entry,
                   uint32_t argc,
                   virt_ptr<void> argv,
                   virt_ptr<uint32_t> stack,
                   uint32_t stackSize,
                   int32_t priority,
                   OSThreadAttributes attributes,
                   OSThreadType type)
{
   if (type != OSThreadType::AppIo && type != OSThreadType::App) {
      return FALSE;
   }

   return createThread(thread, entry, argc, argv, stack, stackSize, priority, attributes, type);
}

BOOL
coreinit__OSCreateThreadType(virt_ptr<OSThread> thread,
                             OSThreadEntryPointFn entry,
                             uint32_t argc,
                             virt_ptr<void> argv,
                             virt_ptr<uint32_t> stack,
                             uint32_t stackSize,
                             int32_t priority,
                             OSThreadAttributes attributes,
                             OSThreadType type)
{
   return createThread(thread, entry, argc, argv, stack, stackSize, priority, attributes, type);
}

/**
 * Detach thread.
 */
void
OSDetachThread(virt_ptr<OSThread> thread)
{
   internal::lockScheduler();

   // HACK: Unfortunately this check is not valid in all games.  One Piece performs
   //  OSJoinThread on a thread, and then subsequently calls OSDetachThread on it
   //  for whatever reason.  Coreinit doesnt check this, so we can't do this check.
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

   internal::wakeupThreadNoLock(virt_addrof(thread->joinQueue));
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
      cafe::invoke(cpu::this_core::state(),
                   thread->cleanupCallback,
                   thread,
                   virt_cast<void *>(thread->stackEnd));
   }

   // Cleanup the GHS exceptions we previously created
   internal::ghsExceptionCleanup(thread);

   // Free any TLS data which was allocated to this thread
   if (thread->tlsSections) {
      internal::dynLoadTlsFree(thread);
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
OSGetActiveThreadLink(virt_ptr<OSThread> thread,
                      virt_ptr<OSThreadLink> link)
{
   *link = thread->activeLink;
}


/**
 * Return pointer to OSThread object for the current thread.
 */
virt_ptr<OSThread>
OSGetCurrentThread()
{
   return internal::getCurrentThread();
}


/**
 * Returns the default thread for a specific core.
 */
virt_ptr<OSThread>
OSGetDefaultThread(uint32_t coreID)
{
   if (coreID >= CoreCount) {
      return nullptr;
   }

   return virt_addrof(sThreadData->perCoreData[coreID].defaultThread);
}


/**
 * Return current stack pointer, value of r1 register.
 */
virt_ptr<uint32_t>
OSGetStackPointer()
{
   return virt_cast<uint32_t *>(virt_addr { cpu::this_core::state()->gpr[1] });
}


/**
 * Return user stack pointer.
 */
virt_ptr<uint32_t>
OSGetUserStackPointer(virt_ptr<OSThread> thread)
{
   auto stack = virt_ptr<uint32_t> { nullptr };
   internal::lockScheduler();

   if (OSIsThreadSuspended(thread)) {
      stack = thread->userStackPointer;

      if (!stack) {
         stack = virt_cast<uint32_t *>(virt_addr { thread->context.gpr[1].value() });
      }
   }

   internal::unlockScheduler();
   return stack;
}


/**
 * Get a thread's affinity.
 */
uint32_t
OSGetThreadAffinity(virt_ptr<OSThread> thread)
{
   return thread->attr & OSThreadAttributes::AffinityAny;
}


/**
 * Get a thread's name.
 */
virt_ptr<const char>
OSGetThreadName(virt_ptr<OSThread> thread)
{
   return thread->name;
}


/**
 * Get a thread's base priority.
 */
int32_t
OSGetThreadPriority(virt_ptr<OSThread> thread)
{
   if (thread->type == OSThreadType::Driver) {
      return thread->basePriority;
   } else if(thread->type == OSThreadType::AppIo) {
      return thread->basePriority - 32;
   } else if (thread->type == OSThreadType::App) {
      return thread->basePriority - 64;
   }

   decaf_abort("Unexpected thread type in OSGetThreadPriority");
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
OSInitThreadQueue(virt_ptr<OSThreadQueue> queue)
{
   OSInitThreadQueueEx(queue, nullptr);
}


/**
* Initialise a thread queue object with a parent.
*/
void
OSInitThreadQueueEx(virt_ptr<OSThreadQueue> queue,
                    virt_ptr<void> parent)
{
   queue->head = nullptr;
   queue->tail = nullptr;
   queue->parent = parent;
}


/**
 * Returns TRUE if a thread is suspended.
 */
BOOL
OSIsThreadSuspended(virt_ptr<OSThread> thread)
{
   return thread->suspendCounter > 0;
}


/**
 * Returns TRUE if a thread is terminated.
 */
BOOL
OSIsThreadTerminated(virt_ptr<OSThread> thread)
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
OSJoinThread(virt_ptr<OSThread> thread,
             virt_ptr<int32_t> outExitValue)
{
   internal::lockScheduler();

   // If the thread has not ended, let's wait for it
   //  note only one thread is allowed in the join queue
   if (!(thread->attr & OSThreadAttributes::Detached) &&
       thread->state != OSThreadState::Moribund &&
       !thread->joinQueue.head) {
      internal::sleepThreadNoLock(virt_addrof(thread->joinQueue));
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

   if (outExitValue) {
      *outExitValue = thread->exitValue;
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

   fmt::memory_buffer out;
   fmt::format_to(out, "id   = {}\n", thread->id);

   if (thread->name) {
      fmt::format_to(out, "name  = {}\n", thread->name);
   }

   fmt::format_to(out, "cia   = 0x{:08X}\n", state->cia);
   fmt::format_to(out, "lr    = 0x{:08X}\n", state->lr);
   fmt::format_to(out, "cr    = 0x{:08X}\n", state->cr.value);
   fmt::format_to(out, "xer   = 0x{:08X}\n", state->xer.value);
   fmt::format_to(out, "ctr   = 0x{:08X}\n", state->ctr);

   for (auto i = 0u; i < 32; ++i) {
      fmt::format_to(out, "r{:<2}   = 0x{:08X}\n", i, state->gpr[i]);
   }

   fmt::format_to(out, "fpscr = 0x{:08X}\n", state->fpscr.value);

   for (auto i = 0u; i < 32; ++i) {
      fmt::format_to(out, "f{:<2}   = {}\n", i, state->fpr[i].value);
   }

   for (auto i = 0u; i < 32; ++i) {
      fmt::format_to(out, "ps{:<2}   = {:<16} ps{:<2}   = {}\n", i, state->fpr[i].paired0, i, state->fpr[i].paired1);
   }

   gLog->info(std::string_view { out.data(), out.size() });
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
OSResumeThread(virt_ptr<OSThread> thread)
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
OSRunThread(virt_ptr<OSThread> thread,
            OSThreadEntryPointFn entry,
            uint32_t argc,
            virt_ptr<void> argv)
{
   BOOL result = FALSE;
   internal::lockScheduler();

   if (OSIsThreadTerminated(thread)) {
      if (thread->state == OSThreadState::Moribund) {
         internal::markThreadInactiveNoLock(thread);
      }

      auto stackSize =
         virt_cast<virt_addr>(thread->stackStart) -
         virt_cast<virt_addr>(thread->stackEnd);

      initialiseThreadState(thread,
                            entry,
                            argc,
                            argv,
                            thread->stackStart,
                            static_cast<uint32_t>(stackSize),
                            thread->basePriority,
                            thread->context.pir,
                            thread->type);
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
OSSetThreadAffinity(virt_ptr<OSThread> thread,
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
OSSetThreadCleanupCallback(virt_ptr<OSThread> thread,
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
OSSetThreadDeallocator(virt_ptr<OSThread> thread,
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
OSSetThreadName(virt_ptr<OSThread> thread,
                virt_ptr<const char> name)
{
   thread->name = name;
}


/**
 * Set a thread's priority.
 */
BOOL
OSSetThreadPriority(virt_ptr<OSThread> thread,
                    int32_t priority)
{
   auto realPriority = priority;
   if (thread->type == OSThreadType::Driver) {
      realPriority = priority;
   } else if (thread->type == OSThreadType::AppIo) {
      realPriority = priority + 32;
   } else if (thread->type == OSThreadType::App) {
      realPriority = priority + 64;
   } else {
      return FALSE;
   }

   internal::lockScheduler();
   thread->basePriority = realPriority;
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
OSSetThreadRunQuantum(virt_ptr<OSThread> thread,
                      uint32_t quantumUS)
{
   if (quantumUS != OSThreadQuantum::Infinite) {
      if (quantumUS < OSThreadQuantum::MinMicroseconds) {
         return FALSE;
      }

      if (quantumUS > OSThreadQuantum::MaxMicroseconds) {
         return FALSE;
      }
   }

   auto ticks = internal::usToTicks(quantumUS);
   auto result = FALSE;

   internal::lockScheduler();
   result = internal::setThreadRunQuantumNoLock(thread, ticks);
   internal::unlockScheduler();
   return result;
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
OSSetThreadStackUsage(virt_ptr<OSThread> thread)
{
   internal::lockScheduler();

   if (!thread) {
      thread = OSGetCurrentThread();
   } else if (thread->state == OSThreadState::Running) {
      internal::unlockScheduler();
      return FALSE;
   }

   clearThreadStackWithValue(thread, 0xfefefefe);
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
OSSleepThread(virt_ptr<OSThreadQueue> queue)
{
   internal::lockScheduler();
   internal::sleepThreadNoLock(queue);
   internal::rescheduleSelfNoLock();
   internal::unlockScheduler();
}

static void
sleepAlarmHandler(virt_ptr<OSAlarm> alarm,
                  virt_ptr<OSContext> context)
{
   // Wakeup the thread waiting on this alarm
   auto data = virt_cast<OSThread *>(OSGetAlarmUserData(alarm));

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
   auto alarm = StackObject<OSAlarm> { };
   auto queue = StackObject<OSThreadQueue> { };

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
OSSuspendThread(virt_ptr<OSThread> thread)
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
         internal::sleepThreadNoLock(virt_addrof(thread->suspendQueue));
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
OSWakeupThread(virt_ptr<OSThreadQueue> queue)
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

namespace internal
{

/**
 * Set a user stack pointer for the current thread.
 */
void
setUserStackPointer(virt_ptr<uint32_t> stack)
{
   auto thread = OSGetCurrentThread();

   if (stack >= thread->stackEnd && stack < thread->stackStart) {
      // Cannot modify stack to within current stack frame.
      return;
   }

   auto current = OSGetStackPointer();

   if (current < thread->stackEnd || current >= thread->stackStart) {
      // If current stack is outside stack frame, then we must already have
      // a user stack pointer, and we shouldn't overwrite it.
      return;
   }

   thread->userStackPointer = stack;
   OSTestThreadCancel();
   thread->cancelState |= OSThreadCancelState::DisabledByUserStackPointer;
}


/**
 * Remove the user stack pointer for the current thread.
 */
void
removeUserStackPointer(virt_ptr<uint32_t> stack)
{
   auto thread = OSGetCurrentThread();

   if (stack < thread->stackEnd || stack >= thread->stackStart) {
      // If restore stack pointer is outside stack frame, then it is not
      // really restoring the original stack.
      return;
   }

   thread->cancelState &= ~OSThreadCancelState::DisabledByUserStackPointer;
   thread->userStackPointer = nullptr;
   OSTestThreadCancel();
}


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
exitThreadNoLock(int32_t value)
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
   internal::wakeupThreadNoLock(virt_addrof(thread->joinQueue));
   internal::wakeupThreadWaitForSuspensionNoLock(virt_addrof(thread->suspendQueue), -1);
   internal::rescheduleAllCoreNoLock();
   internal::enableScheduler();

   cafe::kernel::exitThreadNoLock();
   internal::rescheduleSelfNoLock();

   // We do not need to unlockScheduler as OSExitThread never returns.
   decaf_abort("Exited thread was rescheduled...");
}

void
queueThreadDeallocation(virt_ptr<OSThread> thread)
{
   auto &perCoreData = sThreadData->perCoreData[cpu::this_core::id()];
   ThreadQueue::insert(virt_addrof(perCoreData.deallocationQueue), thread);
   wakeupThreadNoLock(virt_addrof(perCoreData.deallocationThreadQueue));
}

static uint32_t
deallocatorThreadEntry(uint32_t coreId,
                       virt_ptr<void>)
{
   auto &perCoreData = sThreadData->perCoreData[cpu::this_core::id()];
   auto waitQueue = virt_addrof(perCoreData.deallocationThreadQueue);
   auto queue = virt_addrof(perCoreData.deallocationQueue);

   auto oldInterrupts = OSDisableInterrupts();

   while (true) {
      auto thread = ThreadQueue::popFront(queue);

      if (!thread) {
         lockScheduler();
         sleepThreadNoLock(waitQueue);
         rescheduleSelfNoLock();
         unlockScheduler();
         continue;
      }

      if (thread->deallocator) {
         OSRestoreInterrupts(oldInterrupts);

         cafe::invoke(cpu::this_core::state(),
                      thread->deallocator,
                      thread,
                      virt_cast<void *>(thread->stackEnd));
         oldInterrupts = OSDisableInterrupts();
      }
   }

   OSRestoreInterrupts(oldInterrupts);
}

static void
initialiseDeallocatorThread()
{
   auto coreId = OSGetCoreId();
   auto &perCoreData = sThreadData->perCoreData[coreId];
   OSInitThreadQueue(virt_addrof(perCoreData.deallocationThreadQueue));
   OSInitThreadQueue(virt_addrof(perCoreData.deallocationQueue));

   auto thread = virt_addrof(perCoreData.deallocatorThread);
   auto stack = virt_addrof(perCoreData.deallocatorThreadStack);
   auto stackSize = perCoreData.deallocatorThreadStack.size();
   perCoreData.deallocatorThreadName = fmt::format("{{SYS Thread Terminator Core {}}}", coreId);

   coreinit__OSCreateThreadType(thread, sDeallocatorThreadEntryPoint, coreId, nullptr,
                                virt_cast<uint32_t *>(stack + stackSize),
                                stackSize,
                                1,
                                static_cast<OSThreadAttributes>(1 << coreId),
                                OSThreadType::AppIo);
   OSSetThreadName(thread, virt_addrof(perCoreData.deallocatorThreadName));
   OSResumeThread(thread);
}

static uint32_t
defaultThreadEntry(uint32_t coreId,
                   virt_ptr<void> /* unused */)
{
   auto thread = OSGetDefaultThread(coreId);

   lockScheduler();
   setCoreRunningThread(coreId, thread);
   OSSetCurrentContext(virt_addrof(thread->context));
   OSSetCurrentFPUContext(0);
   unlockScheduler();

   initialiseIci();
   initialiseExceptionHandlers();
   initialiseAlarmThread();
   initialiseLockedCache(coreId);
   IPCDriverInit();
   IPCDriverOpen();

   internal::COSWarn(COSReportModule::Unknown1,
      fmt::format("  Core {} Complete, MSR 0x{:08X}, Default Thread {}",
                  coreId, 0, thread));

   OSWaitRendezvous(virt_addrof(sThreadData->defaultThreadInitRendezvous),
                    sThreadData->defaultThreadInitRendezvousWaitMask);
   initialiseDeallocatorThread();
   OSExitThread(0);
   return 0;
}

static void
initialiseThreadForCore(uint32_t coreId)
{
   auto currentCoreId = OSGetCoreId();
   auto thread = virt_addrof(sThreadData->perCoreData[coreId].defaultThread);

   sThreadData->perCoreData[coreId].defaultThreadName = fmt::format("Default Thread {}", coreId);
   thread->name = virt_addrof(sThreadData->perCoreData[coreId].defaultThreadName);
   thread->tag = OSThread::Tag;
   thread->exitValue = -1;
   thread->type = OSThreadType::App;
   thread->attr = OSThreadAttributes::Detached;
   thread->state = OSThreadState::Running;
   thread->priority = 80;
   thread->basePriority = 80;
   thread->id = sThreadId++;

   OSInitThreadQueueEx(virt_addrof(thread->joinQueue), thread);
   OSInitThreadQueueEx(virt_addrof(thread->suspendQueue), thread);
   thread->mutexQueue.parent = thread;

   thread->stackStart = virt_cast<uint32_t *>(internal::getDefaultThreadStackBase(coreId));
   thread->stackEnd = virt_cast<uint32_t *>(internal::getDefaultThreadStackEnd(coreId));

   if (currentCoreId == coreId) {
      // Save and restore our host context because OSInitContext nulls it
      auto hostContext = thread->context.hostContext;
      OSInitContext(virt_addrof(thread->context),
                    virt_addr { 0 },
                    virt_cast<virt_addr>(thread->stackStart));
      thread->context.hostContext = hostContext;
   } else {
      OSInitContext(virt_addrof(thread->context),
                    virt_func_cast<virt_addr>(sDefaultThreadEntryPoint),
                    virt_cast<virt_addr>(thread->stackStart));
   }

   thread->context.pir = coreId;
   thread->context.starttime = OSGetSystemTime();
   thread->context.gqr[2] = 0x40004u;
   thread->context.gqr[3] = 0x50005u;
   thread->context.gqr[4] = 0x60006u;
   thread->context.gqr[5] = 0x70007u;

   if (coreId == 0) {
      thread->attr |= OSThreadAttributes::AffinityCPU0;
      thread->context.attr |= OSThreadAttributes::AffinityCPU0;
   } else if (coreId == 1) {
      thread->attr |= OSThreadAttributes::AffinityCPU1;
      thread->context.attr |= OSThreadAttributes::AffinityCPU1;
   } else if (coreId == 2) {
      thread->attr |= OSThreadAttributes::AffinityCPU2;
      thread->context.attr |= OSThreadAttributes::AffinityCPU2;
   }

   clearThreadStackWithValue(thread, 0);
   *thread->stackEnd = 0xDEADBABE;

   if (currentCoreId == coreId) {
      setCoreRunningThread(coreId, thread);
   }

   markThreadActiveNoLock(thread);
}

void
initialiseThreads()
{
   auto mainCoreId = OSGetMainCoreId();
   auto mainThread = OSGetDefaultThread(mainCoreId);
   internal::lockScheduler();

   for (auto i = 0u; i < sThreadData->perCoreData.size(); ++i) {
      auto &perCoreData = sThreadData->perCoreData[i];
      OSCreateAlarm(virt_addrof(perCoreData.timeSliceAlarm));
      initialiseThreadForCore(i);
   }

   // Hijack the kernel context fiber into our own.
   kernel::hijackCurrentHostContext(virt_addrof(mainThread->context));
   OSSetCurrentContext(virt_addrof(mainThread->context));
   OSSetCurrentFPUContext(0);

   internal::unlockScheduler();

   internal::COSWarn(COSReportModule::Unknown1,
      fmt::format("UserMode Core & Thread Initialization ({} Cores)",
                  OSGetCoreCount()));

   internal::COSWarn(COSReportModule::Unknown1,
      fmt::format("  Core {} Complete, Default Thread {}",
                  mainCoreId, mainThread));
   sThreadData->defaultThreadInitRendezvousWaitMask = 1u << 1;

   // Run default thread initilisation on Core 0
   if (auto thread = virt_addrof(sThreadData->perCoreData[0].defaultThread)) {
      OSInitRendezvous(virt_addrof(sThreadData->defaultThreadInitRendezvous));

      thread->context.gpr[3] = 0u;
      kernel::setSubCoreEntryContext(0, virt_addrof(thread->context));

      OSWaitRendezvous(virt_addrof(sThreadData->defaultThreadInitRendezvous),
                       1 << 0);
   }

   // Run default thread initilisation on Core 2
   if (auto thread = virt_addrof(sThreadData->perCoreData[2].defaultThread)) {
      OSInitRendezvous(virt_addrof(sThreadData->defaultThreadInitRendezvous));

      thread->context.gpr[3] = 2u;
      kernel::setSubCoreEntryContext(2, virt_addrof(thread->context));

      OSWaitRendezvous(virt_addrof(sThreadData->defaultThreadInitRendezvous),
                       1 << 2);
   }

   OSWaitRendezvous(virt_addrof(sThreadData->defaultThreadInitRendezvous),
                    1 << 2);

   initialiseDeallocatorThread();
}

} // namespace internal

void
Library::registerThreadSymbols()
{
   RegisterFunctionExport(OSCancelThread);
   RegisterFunctionExport(OSCheckActiveThreads);
   RegisterFunctionExport(OSCheckThreadStackUsage);
   RegisterFunctionExport(OSClearStack);
   RegisterFunctionExport(OSClearThreadStackUsage);
   RegisterFunctionExport(OSContinueThread);
   RegisterFunctionExport(OSCreateThread);
   RegisterFunctionExport(OSCreateThreadType);
   RegisterFunctionExportName("__OSCreateThreadType", coreinit__OSCreateThreadType);
   RegisterFunctionExport(OSDetachThread);
   RegisterFunctionExport(OSExitThread);
   RegisterFunctionExport(OSGetActiveThreadLink);
   RegisterFunctionExport(OSGetCurrentThread);
   RegisterFunctionExport(OSGetDefaultThread);
   RegisterFunctionExport(OSGetStackPointer);
   RegisterFunctionExport(OSGetUserStackPointer);
   RegisterFunctionExport(OSGetThreadAffinity);
   RegisterFunctionExport(OSGetThreadName);
   RegisterFunctionExport(OSGetThreadPriority);
   RegisterFunctionExport(OSGetThreadSpecific);
   RegisterFunctionExport(OSInitThreadQueue);
   RegisterFunctionExport(OSInitThreadQueueEx);
   RegisterFunctionExport(OSIsThreadSuspended);
   RegisterFunctionExport(OSIsThreadTerminated);
   RegisterFunctionExport(OSJoinThread);
   RegisterFunctionExport(OSPrintCurrentThreadState);
   RegisterFunctionExport(OSResumeThread);
   RegisterFunctionExport(OSRunThread);
   RegisterFunctionExport(OSSetThreadAffinity);
   RegisterFunctionExport(OSSetThreadCancelState);
   RegisterFunctionExport(OSSetThreadCleanupCallback);
   RegisterFunctionExport(OSSetThreadDeallocator);
   RegisterFunctionExport(OSSetThreadName);
   RegisterFunctionExport(OSSetThreadPriority);
   RegisterFunctionExport(OSSetThreadRunQuantum);
   RegisterFunctionExport(OSSetThreadSpecific);
   RegisterFunctionExport(OSSetThreadStackUsage);
   RegisterFunctionExport(OSSleepThread);
   RegisterFunctionExport(OSSleepTicks);
   RegisterFunctionExport(OSSuspendThread);
   RegisterFunctionExport(OSTestThreadCancel);
   RegisterFunctionExport(OSWakeupThread);
   RegisterFunctionExport(OSYieldThread);

   RegisterDataInternal(sThreadData);
   RegisterFunctionInternal(threadEntry, sThreadEntryPoint);
   RegisterFunctionInternal(sleepAlarmHandler, sSleepAlarmHandler);
   RegisterFunctionInternal(internal::deallocatorThreadEntry, sDeallocatorThreadEntryPoint);
   RegisterFunctionInternal(internal::defaultThreadEntry, sDefaultThreadEntryPoint);
}

} // namespace Internalcafe::coreinit
