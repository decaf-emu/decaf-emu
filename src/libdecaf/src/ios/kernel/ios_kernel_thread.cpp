#include "ios_kernel_thread.h"
#include "ios_kernel_threadqueue.h"
#include "ios_kernel_process.h"
#include "ios_kernel_scheduler.h"

#include <array>
#include <mutex>

namespace ios::kernel
{

struct StaticThreadData
{
   be2_array<Thread, MaxNumThreads> threads;
   be2_val<uint32_t> numActiveThreads = uint32_t { 0 };
};

static phys_ptr<StaticThreadData>
sData = nullptr;

namespace internal
{

static void
memset32(phys_ptr<void> ptr,
         uint32_t value,
         uint32_t bytes);

static void
iosFiberEntryPoint(void *context);

} // namespace internal

Error
IOS_CreateThread(ThreadEntryFn entry,
                 phys_ptr<void> context,
                 phys_ptr<uint8_t> stackTop,
                 uint32_t stackSize,
                 int priority,
                 ThreadFlags flags)
{
   phys_ptr<Thread> thread = nullptr;

   if (priority > MaxThreadPriority) {
      return Error::Invalid;
   }

   // We cannot create thread with priority higher than current thread's max
   // priority
   auto currentThread = internal::getCurrentThread();
   if (currentThread && priority > currentThread->maxPriority) {
      return Error::Invalid;
   }

   // Check stack pointer and alignment
   if (!stackTop || (phys_cast<phys_addr>(stackTop) & 3)) {
      return Error::Invalid;
   }

   // Check stack size and alignment
   if (stackSize < 0x68 || (stackSize & 3)) {
      return Error::Invalid;
   }

   // Find a free thread
   for (auto i = 0u; i < sData->threads.size(); ++i) {
      if (sData->threads[i].state == ThreadState::Available) {
         thread = phys_addrof(sData->threads[i]);
         thread->id = i;
         break;
      }
   }

   if (!thread) {
      // Maximum number of threads running
      return Error::Max;
   }

   if (currentThread) {
      thread->pid = currentThread->pid;
   } else {
      thread->pid = ProcessId::KERNEL;
   }

   thread->state = ThreadState::Stopped;
   thread->maxPriority = priority;
   thread->priority = priority;
   thread->userStackAddr = stackTop;
   thread->userStackSize = stackSize;
   // TODO: thread->sysStackAddr = systemStack + thread->id * 0x400;
   thread->stackPointer = stackTop - 0x10;

   internal::memset32(stackTop - stackSize, 0xF5A5A5A5, stackSize);

   if (flags & ThreadFlags::AllocateTLS) {
      stackTop -= sizeof(ThreadLocalStorage);
      thread->threadLocalStorage = phys_cast<ThreadLocalStorage *>(stackTop);
      std::memset(thread->threadLocalStorage.get(), 0, 0x24);
   } else {
      thread->threadLocalStorage = nullptr;
   }

   sData->numActiveThreads++;
   thread->flags = flags;
   thread->threadQueueNext = nullptr;
   thread->threadQueue = nullptr;
   ThreadQueue_Initialise(phys_addrof(thread->joinQueue));

#ifdef IOS_EMULATE_ARM
   // We are not doing ARM emulation...
   thread->context.gpr[0] = phys_addr { context }.getAddress();
   thread->context.gpr[13] = phys_addr { stackTop - 0x20 }.getAddress();
   thread->context.pc = entry;

   if (entry & 1) {
      // Disable both FIQ and IRQ interrupts
      thread->context.cpsr = 0x30;
   } else {
      // Only disable IRQ interrupts
      thread->context.cpsr = 0x10;
   }

   thread->context.lr = 0xDEADC0DE;
#else
   thread->context.entryPoint = entry;
   thread->context.entryPointArg = context;
   thread->context.fiber = platform::createFiber(internal::iosFiberEntryPoint,
                                                 thread.get());
#endif

   return static_cast<Error>(thread->id);
}

Error
IOS_JoinThread(ThreadId id,
               phys_ptr<Error> returnedValue)
{
   auto currentThread = internal::getCurrentThread();

   if (!id) {
      id = currentThread->id;
   } else if (id >= sData->threads.size()) {
      return Error::Invalid;
   }

   auto thread = phys_addrof(sData->threads[id]);
   if (thread->pid != currentThread->pid) {
      // Can only join threads belonging to the same process.
      return Error::Invalid;
   }

   if (thread == currentThread) {
      // Can't join self.
      return Error::Invalid;
   }

   if (thread->flags & ThreadFlags::Detached) {
      // Can't join a detached thread.
      return Error::Invalid;
   }

   /*
   if (thread->unknown0x64 != dword_81430D4) {
      return Error::Invalid;
   }
   */

   if (thread->state != ThreadState::Dead) {
      internal::sleepThread(phys_addrof(thread->joinQueue));
      internal::reschedule();
   }

   if (returnedValue) {
      *returnedValue = thread->exitValue;
   }

   thread->state = ThreadState::Available;
   return Error::OK;
}

Error
IOS_CancelThread(ThreadId id,
                 Error exitValue)
{
   auto currentThread = internal::getCurrentThread();

   if (!id) {
      id = currentThread->id;
   } else if (id >= sData->threads.size()) {
      return Error::Invalid;
   }

   auto thread = phys_addrof(sData->threads[id]);
   if (thread->pid != currentThread->pid) {
      // Can only cancel threads belonging to the same process.
      return Error::Invalid;
   }

   if (thread->state != ThreadState::Stopped) {
      ThreadQueue_RemoveThread(thread->threadQueue, thread);
   }

   thread->exitValue = exitValue;
   sData->numActiveThreads--;

   if (thread->flags & ThreadFlags::Detached) {
      thread->state = ThreadState::Available;
   } else {
      thread->state = ThreadState::Dead;
      internal::wakeupAllThreads(phys_addrof(thread->joinQueue), Error::OK);
   }

   if (thread == currentThread) {
      internal::reschedule();
   }

   return Error::Invalid;
}

Error
IOS_StartThread(ThreadId id)
{
   auto currentThread = internal::getCurrentThread();

   if (!id) {
      id = currentThread->id;
   } else if (id >= sData->threads.size()) {
      return Error::Invalid;
   }

   auto thread = phys_addrof(sData->threads[id]);
   if (currentThread->pid != ProcessId::KERNEL && thread->pid != currentThread->pid) {
      // Can only start threads belonging to the same process.
      // Unless we're the kernel of course.
      return Error::Invalid;
   }

   if (thread->state != ThreadState::Stopped) {
      // Can only start a stopped thread.
      return Error::Invalid;
   }

   if (thread->threadQueue && !internal::isThreadInRunQueue(thread)) {
      thread->state = ThreadState::Waiting;
      ThreadQueue_PushThread(thread->threadQueue, thread);
   } else {
      thread->state = ThreadState::Ready;
      internal::queueThread(thread);
   }

   internal::reschedule();
   return Error::OK;
}

Error
IOS_SuspendThread(ThreadId id)
{
   auto currentThread = internal::getCurrentThread();

   if (!id) {
      id = currentThread->id;
   } else if (id >= sData->threads.size()) {
      return Error::Invalid;
   }

   auto thread = phys_addrof(sData->threads[id]);
   if (thread->pid != currentThread->pid) {
      // Can only suspend threads belonging to the same process.
      return Error::Invalid;
   }

   if (thread->state == ThreadState::Running) {
      thread->state = ThreadState::Stopped;
   } else if (thread->state != ThreadState::Waiting &&
              thread->state != ThreadState::Ready) {
      // Cannot suspend a thread which is not Running, Ready or Waiting.
      return Error::Invalid;
   } else {
      thread->state = ThreadState::Stopped;
      ThreadQueue_RemoveThread(thread->threadQueue, thread);
   }

   if (thread == currentThread) {
      internal::reschedule();
   }

   return Error::OK;
}

Error
IOS_YieldCurrentThread()
{
   internal::reschedule(true);
   return Error::Invalid;
}

Error
IOS_GetCurrentThreadId()
{
   auto thread = internal::getCurrentThread();
   if (!thread) {
      return Error::Invalid;
   }

   return static_cast<Error>(thread->id);
}

phys_ptr<ThreadLocalStorage>
IOS_GetCurrentThreadLocalStorage()
{
   auto thread = internal::getCurrentThread();
   if (!thread) {
      return nullptr;
   }

   return thread->threadLocalStorage;
}

Error
IOS_GetThreadPriority(ThreadId id)
{
   auto currentThread = internal::getCurrentThread();

   if (!id) {
      id = currentThread->id;
   } else if (id >= sData->threads.size()) {
      return Error::Invalid;
   }

   auto thread = phys_addrof(sData->threads[id]);
   if (thread->pid != currentThread->pid) {
      // Can only access threads in same process.
      return Error::Invalid;
   }

   return static_cast<Error>(thread->priority);
}

Error
IOS_SetThreadPriority(ThreadId id,
                      ThreadPriority priority)
{
   auto currentThread = internal::getCurrentThread();

   if (!id) {
      id = currentThread->id;
   } else if (id >= sData->threads.size()) {
      return Error::Invalid;
   }

   auto thread = phys_addrof(sData->threads[id]);
   if (thread->pid != currentThread->pid) {
      // Can only access threads in same process.
      return Error::Invalid;
   }

   if (priority > MaxThreadPriority || priority > thread->maxPriority) {
      return Error::Invalid;
   }

   if (thread->priority == priority) {
      return Error::OK;
   }

   thread->priority = priority;

   if (thread != currentThread) {
      ThreadQueue_RemoveThread(thread->threadQueue, thread);
      ThreadQueue_PushThread(thread->threadQueue, thread);
   }

   internal::reschedule();
   return Error::OK;
}

namespace internal
{

static void
memset32(phys_ptr<void> ptr,
         uint32_t value,
         uint32_t bytes)
{
   auto ptr32 = phys_cast<uint32_t *>(ptr);

   for (auto i = 0u; i < bytes / 4; ++i) {
      ptr32[i] = value;
   }
}

static void
iosFiberEntryPoint(void *context)
{
   auto thread = reinterpret_cast<Thread *>(context);
#ifndef IOS_EMULATE_ARM
   auto exitValue = thread->context.entryPoint(thread->context.entryPointArg);
#else
   decaf_abort("iosFiberEntryPoint for IOS_EMULATE_ARM unimplemented");
#endif

   if (thread->flags & ThreadFlags::Detached) {
      IOS_CancelThread(thread->id, exitValue);
   } else {
      thread->exitValue = exitValue;
      IOS_SuspendThread(thread->id);
   }
}

phys_ptr<Thread>
getThread(ThreadId id)
{
   auto thread = phys_addrof(sData->threads[id]);
   if (thread->id == id) {
      return thread;
   }

   return nullptr;
}

void
setThreadName(ThreadId id,
              const char *name)
{
   if (auto thread = getThread(id)) {
      thread->context.threadName = name;
   }
}

void
initialiseStaticThreadData()
{
   sData = allocProcessStatic<StaticThreadData>();
}

} // namespace internal

} // namespace ios::kernel
