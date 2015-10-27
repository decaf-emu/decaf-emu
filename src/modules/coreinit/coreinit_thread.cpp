#include <limits>
#include "coreinit.h"
#include "coreinit_alarm.h"
#include "coreinit_core.h"
#include "coreinit_memheap.h"
#include "coreinit_scheduler.h"
#include "coreinit_systeminfo.h"
#include "coreinit_thread.h"
#include "memory_translate.h"
#include "processor.h"
#include "system.h"
#include "usermodule.h"

static OSThread *
gDefaultThreads[CoreCount];

static uint32_t
gThreadId = 1;

void
__OSClearThreadStack32(OSThread *thread, uint32_t value)
{
   virtual_ptr<be_val<uint32_t>> clearStart, clearEnd;

   if (OSGetCurrentThread() == thread) {
      clearStart = thread->stackEnd + 4;
      clearEnd = make_virtual_ptr<be_val<uint32_t>>(OSGetStackPointer());
   } else {
      clearStart = thread->stackEnd + 4;
      clearEnd = make_virtual_ptr<be_val<uint32_t>>(thread->fiber->state.gpr[1]);
   }

   for (auto addr = clearStart; addr < clearEnd; addr += 4) {
      *addr = value;
   }
}

void
OSCancelThread(OSThread *thread)
{
   bool reschedule = false;
   OSLockScheduler();

   if (thread->requestFlag == OSThreadRequest::Suspend) {
      OSWakeupThreadWaitForSuspensionNoLock(&thread->suspendQueue, -1);
      reschedule = true;
   }

   if (thread->suspendCounter != 0) {
      if (thread->cancelState == 0) {
         OSResumeThreadNoLock(thread, thread->suspendCounter);
         reschedule = true;
      }
   }

   if (reschedule) {
      OSRescheduleNoLock();
   }

   thread->suspendCounter = 0;
   thread->needSuspend = 0;
   thread->requestFlag = OSThreadRequest::Cancel;
   OSUnlockScheduler();

   if (OSGetCurrentThread() == thread) {
      OSExitThread(-1);
   }
}

int32_t
OSCheckActiveThreads()
{
   // TODO: Count threads in active thread queue
   assert(false);
   return 1;
}

int32_t
OSCheckThreadStackUsage(OSThread *thread)
{
   virtual_ptr<be_val<uint32_t>> addr;
   OSLockScheduler();

   for (addr = thread->stackEnd + 4; addr < thread->stackStart; addr += 4) {
      if (*addr != 0xfefefefe) {
         break;
      }
   }

   auto result = static_cast<int32_t>(thread->stackStart.getAddress()) - static_cast<int32_t>(addr.getAddress());
   OSUnlockScheduler();
   return result;
}

void
OSClearThreadStackUsage(OSThread *thread)
{
   OSLockScheduler();
   thread->attr &= ~OSThreadAttributes::StackUsage;
   OSUnlockScheduler();
}

void
OSContinueThread(OSThread *thread)
{
   OSLockScheduler();
   OSResumeThreadNoLock(thread, thread->suspendCounter);
   OSRescheduleNoLock();
   OSUnlockScheduler();
}

// Setup thread run state, shared by OSRunThread and OSCreateThread
static void
InitialiseThreadState(OSThread *thread, uint32_t entry, uint32_t argc, void *argv)
{
   auto module = gSystem.getUserModule();
   auto sdaBase = module ? module->sdaBase : 0u;
   auto sda2Base = module ? module->sda2Base : 0u;

   assert(thread->fiber == nullptr);

   // We allocate an extra 8 bytes to deal with any main() calling
   //  conventions where the stack is allocated by the caller?
   //  Honestly I don't have any clue why the PPC entrypoint is
   //  trampling past the edge of the stack, since I don't think
   //  WiiU calling convention actually says that its supposed to
   //  be doing this... Arg...
   const uint32_t EXTRA_STACK_ALLOC = 8;

   // Setup context
   thread->context.gpr[0] = 0;
   thread->context.gpr[1] = thread->stackStart.getAddress() - 4 - EXTRA_STACK_ALLOC;
   thread->context.gpr[2] = sda2Base;
   thread->context.gpr[3] = argc;
   thread->context.gpr[4] = memory_untranslate(argv);
   thread->context.gpr[13] = sdaBase;

   // Setup thread
   thread->entryPoint = entry;
   thread->state = entry ? OSThreadState::Ready : OSThreadState::None;
   thread->requestFlag = OSThreadRequest::None;
   thread->suspendCounter = 1;
   thread->needSuspend = 0;
}

BOOL
OSCreateThread(OSThread *thread, ThreadEntryPoint entry, uint32_t argc, void *argv, be_val<uint32_t> *stack, uint32_t stackSize, int32_t priority, OSThreadAttributes::Flags attributes)
{
   // Setup OSThread
   memset(thread, 0, sizeof(OSThread));
   thread->userStackPointer = stack;
   thread->stackStart = stack;
   thread->stackEnd = reinterpret_cast<be_val<uint32_t>*>(reinterpret_cast<uint8_t*>(stack) - stackSize);
   thread->basePriority = priority;
   thread->attr = attributes;
   thread->id = gThreadId++;

   // Write magic stack ending!
   *thread->stackEnd = 0xDEADBABE;

   // Setup thread state
   InitialiseThreadState(thread, entry, argc, argv);

   return TRUE;
}

void
OSDetachThread(OSThread *thread)
{
   OSLockScheduler();
   thread->attr |= OSThreadAttributes::Detached;
   OSUnlockScheduler();
}

void
OSExitThread(int value)
{
   auto thread = OSGetCurrentThread();
   OSLockScheduler();
   thread->exitValue = value;
   thread->fiber = nullptr;

   if (thread->attr & OSThreadAttributes::Detached) {
      thread->state = OSThreadState::None;
   } else {
      thread->state = OSThreadState::Moribund;
   }

   OSWakeupThreadNoLock(&thread->joinQueue);
   OSWakeupThreadWaitForSuspensionNoLock(&thread->suspendQueue, -1);
   OSUnlockScheduler();
   gProcessor.exit();
}

void
OSGetActiveThreadLink(OSThread *thread, OSThreadLink *link)
{
   link->next = thread->activeLink.next;
   link->prev = thread->activeLink.prev;
}

OSThread *
OSGetCurrentThread()
{
   auto fiber = gProcessor.getCurrentFiber();
   return fiber ? fiber->thread : nullptr;
}

OSThread *
OSGetDefaultThread(uint32_t coreID)
{
   if (coreID >= CoreCount) {
      return nullptr;
   }

   return gDefaultThreads[coreID];
}

uint32_t
OSGetStackPointer()
{
   return OSGetCurrentThread()->fiber->state.gpr[1];
}

uint32_t
OSGetThreadAffinity(OSThread *thread)
{
   return thread->attr & OSThreadAttributes::AffinityAny;
}

const char *
OSGetThreadName(OSThread *thread)
{
   return thread->name;
}

uint32_t
OSGetThreadPriority(OSThread *thread)
{
   return thread->basePriority;
}

uint32_t
OSGetThreadSpecific(uint32_t id)
{
   return OSGetCurrentThread()->specific[id];
}

BOOL
OSIsThreadSuspended(OSThread *thread)
{
   return thread->suspendCounter > 0;
}

BOOL
OSIsThreadTerminated(OSThread *thread)
{
   return thread->state == OSThreadState::None
       || thread->state == OSThreadState::Moribund;
}

BOOL
OSJoinThread(OSThread *thread, be_val<int> *val)
{
   OSLockScheduler();

   if (thread->attr & OSThreadAttributes::Detached) {
      OSUnlockScheduler();
      return FALSE;
   }

   if (thread->state != OSThreadState::Moribund) {
      OSSleepThreadNoLock(&thread->joinQueue);
      OSRescheduleNoLock();
   }

   if (val) {
      *val = thread->exitValue;
   }

   OSUnlockScheduler();
   return TRUE;
}

void
OSPrintCurrentThreadState()
{
   // TODO: Implement OSPrintCurrentThreadState
   assert(false);
}

int32_t
OSResumeThread(OSThread *thread)
{
   OSLockScheduler();
   auto old = OSResumeThreadNoLock(thread, 1);

   if (thread->suspendCounter == 0) {
      OSRescheduleNoLock();
   }

   OSUnlockScheduler();
   return old;
}

BOOL
OSRunThread(OSThread *thread, ThreadEntryPoint entry, uint32_t argc, void *argv)
{
   BOOL result = FALSE;
   OSLockScheduler();

   if (OSIsThreadTerminated(thread)) {
      InitialiseThreadState(thread, entry, argc, argv);
      OSResumeThreadNoLock(thread, 1);
      OSRescheduleNoLock();
      result = TRUE;
   }

   OSUnlockScheduler();
   return result;
}

OSThread *
OSSetDefaultThread(uint32_t core, OSThread *thread)
{
   assert(core < CoreCount);
   auto old = gDefaultThreads[core];
   gDefaultThreads[core] = thread;
   return old;
}

BOOL
OSSetThreadAffinity(OSThread *thread, uint32_t affinity)
{
   OSLockScheduler();
   thread->attr &= ~OSThreadAttributes::AffinityAny;
   thread->attr |= affinity;
   OSRescheduleNoLock();
   OSUnlockScheduler();
   return TRUE;
}

BOOL
OSSetThreadCancelState(BOOL state)
{
   auto thread = OSGetCurrentThread();
   auto old = thread->cancelState;
   thread->cancelState = state;
   return old;
}

void
OSSetThreadName(OSThread *thread, const char *name)
{
   thread->name = name;
}

BOOL
OSSetThreadPriority(OSThread *thread, uint32_t priority)
{
   if (priority > 31) {
      return FALSE;
   }

   OSLockScheduler();
   thread->basePriority = priority;
   OSRescheduleNoLock();
   OSUnlockScheduler();
   return TRUE;
}

BOOL
OSSetThreadRunQuantum(OSThread *thread, uint32_t quantum)
{
   // TODO: Implement OSSetThreadRunQuantum
   assert(false);
   return FALSE;
}

void
OSSetThreadSpecific(uint32_t id, uint32_t value)
{
   OSGetCurrentThread()->specific[id] = value;
}


BOOL
OSSetThreadStackUsage(OSThread *thread)
{
   OSLockScheduler();

   if (!thread) {
      thread = OSGetCurrentThread();
   } else if (thread->state == OSThreadState::Running) {
      OSUnlockScheduler();
      return FALSE;
   }

   __OSClearThreadStack32(thread, 0xfefefefe);
   thread->attr |= OSThreadAttributes::StackUsage;
   OSUnlockScheduler();
   return TRUE;
}

void
OSSleepThread(OSThreadQueue *queue)
{
   OSLockScheduler();
   OSSleepThreadNoLock(queue);
   OSRescheduleNoLock();
   OSUnlockScheduler();
}

static AlarmCallback
pSleepAlarmHandler = nullptr;

struct SleepAlarmData
{
   OSThread *thread;
};

void
SleepAlarmHandler(OSAlarm *alarm, OSContext *context)
{
   OSLockScheduler();

   // Wakeup the thread waiting on this alarm
   auto data = reinterpret_cast<SleepAlarmData*>(OSGetAlarmUserData(alarm));
   OSWakeupOneThreadNoLock(data->thread);

   OSUnlockScheduler();
}

void
OSSleepTicks(OSTime ticks)
{
   auto thread = OSGetCurrentThread();

   // Create the alarm user data
   auto data = OSAllocFromSystem<SleepAlarmData>();
   data->thread = thread;

   // Create an alarm to trigger wakeup
   auto alarm = OSAllocFromSystem<OSAlarm>();
   OSCreateAlarm(alarm);
   OSSetAlarmUserData(alarm, data);
   OSSetAlarm(alarm, ticks, pSleepAlarmHandler);

   // Sleep thread
   OSSleepThread(nullptr);

   OSFreeToSystem(data);
   OSFreeToSystem(alarm);
}

uint32_t
OSSuspendThread(OSThread *thread)
{
   OSLockScheduler();
   int32_t result;

   if (thread->state == OSThreadState::Moribund || thread->state == OSThreadState::None) {
      OSUnlockScheduler();
      return -1;
   }

   if (thread->requestFlag == OSThreadRequest::Cancel) {
      OSUnlockScheduler();
      return -1;
   }

   auto curThread = OSGetCurrentThread();

   if (curThread == thread) {
      if (thread->cancelState) {
         OSUnlockScheduler();
         return -1;
      }

      thread->needSuspend++;
      result = thread->suspendCounter;
      OSSuspendThreadNoLock(thread);
      OSRescheduleNoLock();
   } else {
      if (thread->suspendCounter != 0) {
         result = thread->suspendCounter++;
      } else {
         thread->needSuspend++;
         thread->requestFlag = OSThreadRequest::Suspend;
         OSSleepThreadNoLock(&thread->suspendQueue);
         OSRescheduleNoLock();
         result = thread->suspendResult;
      }
   }

   OSUnlockScheduler();
   return result;
}

void
OSTestThreadCancel()
{
   OSLockScheduler();
   OSTestThreadCancelNoLock();
   OSUnlockScheduler();
}

void
OSWakeupThread(OSThreadQueue *queue)
{
   OSLockScheduler();
   OSWakeupThreadNoLock(queue);
   OSUnlockScheduler();
}

void
OSYieldThread()
{
   gProcessor.yield();
}

void
CoreInit::registerThreadFunctions()
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
   RegisterKernelFunction(OSInsertThreadQueue);
   RegisterKernelFunction(OSIsThreadSuspended);
   RegisterKernelFunction(OSIsThreadTerminated);
   RegisterKernelFunction(OSJoinThread);
   RegisterKernelFunction(OSPrintCurrentThreadState);
   RegisterKernelFunction(OSResumeThread);
   RegisterKernelFunction(OSRunThread);
   RegisterKernelFunction(OSSetThreadAffinity);
   RegisterKernelFunction(OSSetThreadCancelState);
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
   RegisterKernelFunction(SleepAlarmHandler);
}

void
CoreInit::initialiseThread()
{
   pSleepAlarmHandler = findExportAddress("SleepAlarmHandler");
}
