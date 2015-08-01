#include <limits>
#include "coreinit.h"
#include "coreinit_alarm.h"
#include "coreinit_memheap.h"
#include "coreinit_scheduler.h"
#include "coreinit_systeminfo.h"
#include "coreinit_thread.h"
#include "memory.h"
#include "processor.h"
#include "system.h"
#include "usermodule.h"

static OSThread *
gDefaultThreads[3];

static uint32_t
gThreadId = 1;

void
__OSClearThreadStack32(OSThread *thread, uint32_t value)
{
   uint32_t clearStart = 0, clearEnd = 0;

   if (OSGetCurrentThread() == thread) {
      clearStart = thread->stackEnd + 4;
      clearEnd = OSGetStackPointer();
   } else {
      clearStart = thread->stackEnd + 4;
      clearEnd = thread->fiber->state.gpr[1];
   }

   for (auto addr = clearStart; addr < clearEnd; addr += 4) {
      *reinterpret_cast<uint32_t*>(gMemory.translate(addr)) = value;
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

long
OSCheckActiveThreads()
{
   // TODO: Count threads in active thread queue
   assert(false);
   return 1;
}

int32_t
OSCheckThreadStackUsage(OSThread *thread)
{
   uint32_t addr, result;
   OSLockScheduler();

   for (addr = thread->stackEnd + 4; addr < thread->stackStart; addr += 4) {
      auto val = *reinterpret_cast<uint32_t*>(gMemory.translate(addr));

      if (val != 0xfefefefe) {
         break;
      }
   }

   result = thread->stackStart - addr;
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
   assert(thread->fiber == nullptr);

   // Setup context
   thread->context.gpr[0] = 0;
   thread->context.gpr[1] = thread->stackStart - 4;
   thread->context.gpr[2] = module->sda2Base;
   thread->context.gpr[3] = argc;
   thread->context.gpr[4] = gMemory.untranslate(argv);
   thread->context.gpr[13] = module->sdaBase;

   // Setup thread
   thread->entryPoint = entry;
   thread->state = OSThreadState::Ready;
   thread->requestFlag = OSThreadRequest::None;
   thread->suspendCounter = 1;
   thread->needSuspend = 0;
}

BOOL
OSCreateThread(OSThread *thread, ThreadEntryPoint entry, uint32_t argc, void *argv, uint8_t *stack, uint32_t stackSize, int32_t priority, OSThreadAttributes::Flags attributes)
{
   // Setup OSThread
   memset(thread, 0, sizeof(OSThread));
   thread->userStackPointer = stack;
   thread->stackStart = gMemory.untranslate(stack);
   thread->stackEnd = thread->stackStart - stackSize;
   thread->basePriority = priority;
   thread->attr = attributes;
   thread->id = gThreadId++;

   // Write magic stack ending!
   gMemory.write(thread->stackEnd, 0xDEADBABE);

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
   if (coreID >= 3) {
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
OSRunThread(OSThread *thread, ThreadEntryPoint entry, uint32_t argc, p32<void> argv)
{
   BOOL result = false;
   OSLockScheduler();

   if (OSIsThreadTerminated(thread)) {
      InitialiseThreadState(thread, entry, argc, argv);
      OSResumeThreadNoLock(thread, 1);
      OSRescheduleNoLock();
   }

   OSUnlockScheduler();
   return result;
}

OSThread *
OSSetDefaultThread(uint32_t core, OSThread *thread)
{
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
OSSetThreadName(OSThread* thread, const char *name)
{
   thread->name = name;
}

BOOL
OSSetThreadPriority(OSThread* thread, uint32_t priority)
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
OSSetThreadRunQuantum(OSThread* thread, uint32_t quantum)
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
   OSLockScheduler();

   // Create the alarm user data
   auto data = OSAllocFromSystem<SleepAlarmData>();
   data->thread = thread;

   // Create an alarm to trigger wakeup
   auto alarm = OSAllocFromSystem<OSAlarm>();
   OSCreateAlarm(alarm);
   OSSetAlarmUserData(alarm, data);
   OSSetAlarm(alarm, ticks, pSleepAlarmHandler);

   // Sleep thread
   OSSleepThreadNoLock(nullptr);
   OSRescheduleNoLock();

   OSFreeToSystem(data);
   OSFreeToSystem(alarm);
   OSUnlockScheduler();
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
