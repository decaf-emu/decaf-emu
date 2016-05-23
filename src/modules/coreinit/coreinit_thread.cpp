#include <array>
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

namespace coreinit
{

static std::array<OSThread *, CoreCount>
sDefaultThreads;

static uint32_t
sThreadId = 1;

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
   coreinit::internal::lockScheduler();

   if (thread->requestFlag == OSThreadRequest::Suspend) {
      coreinit::internal::wakeupThreadWaitForSuspensionNoLock(&thread->suspendQueue, -1);
      reschedule = true;
   }

   if (thread->suspendCounter != 0) {
      if (thread->cancelState == 0) {
         coreinit::internal::resumeThreadNoLock(thread, thread->suspendCounter);
         reschedule = true;
      }
   }

   if (reschedule) {
      coreinit::internal::rescheduleNoLock();
   }

   thread->suspendCounter = 0;
   thread->needSuspend = 0;
   thread->requestFlag = OSThreadRequest::Cancel;
   coreinit::internal::unlockScheduler();

   if (OSGetCurrentThread() == thread) {
      OSExitThread(-1);
   }
}

int32_t
OSCheckActiveThreads()
{
   // TODO: Count threads in active thread queue
   return 1;
}

int32_t
OSCheckThreadStackUsage(OSThread *thread)
{
   virtual_ptr<be_val<uint32_t>> addr;
   coreinit::internal::lockScheduler();

   for (addr = thread->stackEnd + 4; addr < thread->stackStart; addr += 4) {
      if (*addr != 0xfefefefe) {
         break;
      }
   }

   auto result = static_cast<int32_t>(thread->stackStart.getAddress()) - static_cast<int32_t>(addr.getAddress());
   coreinit::internal::unlockScheduler();
   return result;
}

void
OSClearThreadStackUsage(OSThread *thread)
{
   coreinit::internal::lockScheduler();

   if (!thread) {
      thread = OSGetCurrentThread();
   }

   thread->attr &= ~OSThreadAttributes::StackUsage;
   coreinit::internal::unlockScheduler();
}

void
OSContinueThread(OSThread *thread)
{
   coreinit::internal::lockScheduler();
   coreinit::internal::resumeThreadNoLock(thread, thread->suspendCounter);
   coreinit::internal::rescheduleNoLock();
   coreinit::internal::unlockScheduler();
}

// Setup thread run state, shared by OSRunThread and OSCreateThread
static void
InitialiseThreadState(OSThread *thread,
                      uint32_t entry,
                      uint32_t argc,
                      void *argv)
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
OSCreateThread(OSThread *thread,
               ThreadEntryPoint entry,
               uint32_t argc,
               void *argv,
               be_val<uint32_t> *stack,
               uint32_t stackSize,
               int32_t priority,
               OSThreadAttributes attributes)
{
   // Setup OSThread
   memset(thread, 0, sizeof(OSThread));
   thread->userStackPointer = stack;
   thread->stackStart = stack;
   thread->stackEnd = reinterpret_cast<be_val<uint32_t>*>(reinterpret_cast<uint8_t*>(stack) - stackSize);
   thread->basePriority = priority;
   thread->attr = attributes;
   thread->id = sThreadId++;

   // Write magic stack ending!
   *thread->stackEnd = 0xDEADBABE;

   // Setup thread state
   InitialiseThreadState(thread, entry, argc, argv);

   return TRUE;
}

void
OSDetachThread(OSThread *thread)
{
   coreinit::internal::lockScheduler();
   thread->attr |= OSThreadAttributes::Detached;
   coreinit::internal::unlockScheduler();
}

void
OSExitThread(int value)
{
   auto thread = OSGetCurrentThread();
   coreinit::internal::lockScheduler();
   thread->exitValue = value;
   thread->fiber = nullptr;

   if (thread->attr & OSThreadAttributes::Detached) {
      thread->state = OSThreadState::None;
   } else {
      thread->state = OSThreadState::Moribund;
   }

   coreinit::internal::wakeupThreadNoLock(&thread->joinQueue);
   coreinit::internal::wakeupThreadWaitForSuspensionNoLock(&thread->suspendQueue, -1);
   coreinit::internal::unlockScheduler();
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

   return sDefaultThreads[coreID];
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
   coreinit::internal::lockScheduler();

   if (thread->attr & OSThreadAttributes::Detached) {
      coreinit::internal::unlockScheduler();
      return FALSE;
   }

   if (thread->state != OSThreadState::Moribund) {
      coreinit::internal::sleepThreadNoLock(&thread->joinQueue);
      coreinit::internal::rescheduleNoLock();
   }

   if (val) {
      *val = thread->exitValue;
   }

   coreinit::internal::unlockScheduler();
   return TRUE;
}

void
OSPrintCurrentThreadState()
{
   auto thread = OSGetCurrentThread();

   if (!thread || !thread->fiber) {
      return;
   }

   auto &state = thread->fiber->state;

   fmt::MemoryWriter out;
   out.write("id   = {}\n", thread->id);

   if (thread->name) {
      out.write("name  = {}\n", thread->name.get());
   }

   out.write("cia   = 0x{:08X}\n", state.cia);
   out.write("lr    = 0x{:08X}\n", state.lr);
   out.write("cr    = 0x{:08X}\n", state.cr.value);
   out.write("xer   = 0x{:08X}\n", state.xer.value);
   out.write("ctr   = 0x{:08X}\n", state.ctr);

   for (auto i = 0u; i < 32; ++i) {
      out.write("r{:<2}   = 0x{:08X}\n", i, state.gpr[i]);
   }

   out.write("fpscr = 0x{:08X}\n", state.fpscr.value);

   for (auto i = 0u; i < 32; ++i) {
      out.write("f{:<2}   = {}\n", i, state.fpr[i].value);
   }

   for (auto i = 0u; i < 32; ++i) {
      out.write("ps{:<2}   = {:<16} ps{:<2}   = {}\n", i, state.fpr[i].paired0, i, state.fpr[i].paired1);
   }

   gLog->info(out.str());
}

int32_t
OSResumeThread(OSThread *thread)
{
   coreinit::internal::lockScheduler();
   auto old = coreinit::internal::resumeThreadNoLock(thread, 1);

   if (thread->suspendCounter == 0) {
      coreinit::internal::rescheduleNoLock();
   }

   coreinit::internal::unlockScheduler();
   return old;
}

BOOL
OSRunThread(OSThread *thread, ThreadEntryPoint entry, uint32_t argc, void *argv)
{
   BOOL result = FALSE;
   coreinit::internal::lockScheduler();

   if (OSIsThreadTerminated(thread)) {
      InitialiseThreadState(thread, entry, argc, argv);
      coreinit::internal::resumeThreadNoLock(thread, 1);
      coreinit::internal::rescheduleNoLock();
      result = TRUE;
   }

   coreinit::internal::unlockScheduler();
   return result;
}

OSThread *
OSSetDefaultThread(uint32_t core, OSThread *thread)
{
   assert(core < CoreCount);
   auto old = sDefaultThreads[core];
   sDefaultThreads[core] = thread;
   return old;
}

BOOL
OSSetThreadAffinity(OSThread *thread, uint32_t affinity)
{
   coreinit::internal::lockScheduler();
   thread->attr &= ~OSThreadAttributes::AffinityAny;
   thread->attr |= affinity;
   coreinit::internal::rescheduleNoLock();
   coreinit::internal::unlockScheduler();
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

   coreinit::internal::lockScheduler();
   thread->basePriority = priority;
   coreinit::internal::rescheduleNoLock();
   coreinit::internal::unlockScheduler();
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
   coreinit::internal::lockScheduler();

   if (!thread) {
      thread = OSGetCurrentThread();
   } else if (thread->state == OSThreadState::Running) {
      coreinit::internal::unlockScheduler();
      return FALSE;
   }

   __OSClearThreadStack32(thread, 0xfefefefe);
   thread->attr |= OSThreadAttributes::StackUsage;
   coreinit::internal::unlockScheduler();
   return TRUE;
}

void
OSSleepThread(OSThreadQueue *queue)
{
   coreinit::internal::lockScheduler();
   coreinit::internal::sleepThreadNoLock(queue);
   coreinit::internal::rescheduleNoLock();
   coreinit::internal::unlockScheduler();
}

void
OSSleepTicks(OSTime ticks)
{
   // Create an alarm to trigger wakeup
   auto alarm = coreinit::internal::sysAlloc<OSAlarm>();
   OSCreateAlarm(alarm);
   OSSetAlarm(alarm, ticks, nullptr);

   // Sleep thread
   OSWaitAlarm(alarm);

   coreinit::internal::sysFree(alarm);
}

uint32_t
OSSuspendThread(OSThread *thread)
{
   coreinit::internal::lockScheduler();
   int32_t result;

   if (thread->state == OSThreadState::Moribund || thread->state == OSThreadState::None) {
      coreinit::internal::unlockScheduler();
      return -1;
   }

   if (thread->requestFlag == OSThreadRequest::Cancel) {
      coreinit::internal::unlockScheduler();
      return -1;
   }

   auto curThread = OSGetCurrentThread();

   if (curThread == thread) {
      if (thread->cancelState) {
         coreinit::internal::unlockScheduler();
         return -1;
      }

      thread->needSuspend++;
      result = thread->suspendCounter;
      coreinit::internal::suspendThreadNoLock(thread);
      coreinit::internal::rescheduleNoLock();
   } else {
      if (thread->suspendCounter != 0) {
         result = thread->suspendCounter++;
      } else {
         thread->needSuspend++;
         thread->requestFlag = OSThreadRequest::Suspend;
         coreinit::internal::sleepThreadNoLock(&thread->suspendQueue);
         coreinit::internal::rescheduleNoLock();
         result = thread->suspendResult;
      }
   }

   coreinit::internal::unlockScheduler();
   return result;
}

void
OSTestThreadCancel()
{
   coreinit::internal::lockScheduler();
   coreinit::internal::testThreadCancelNoLock();
   coreinit::internal::unlockScheduler();
}

void
OSWakeupThread(OSThreadQueue *queue)
{
   coreinit::internal::lockScheduler();
   coreinit::internal::wakeupThreadNoLock(queue);
   coreinit::internal::unlockScheduler();
}

void
OSYieldThread()
{
   gProcessor.yield();
}

void
Module::initialiseThreadFunctions()
{
   sDefaultThreads.fill(nullptr);
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
}

} // namespace coreinit
