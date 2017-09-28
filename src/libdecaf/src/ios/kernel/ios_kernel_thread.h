#pragma once
#include "ios_kernel_enum.h"
#include "ios_kernel_threadqueue.h"
#include "ios/ios_enum.h"

#include <cstdint>
#include <common/structsize.h>
#include <common/platform_fiber.h>
#include <libcpu/be2_struct.h>

namespace ios::kernel
{

static constexpr auto MaxNumThreads = 180u;

#pragma pack(push, 1)

using ThreadId = uint32_t;
using ThreadPriority = int32_t;
using ThreadEntryFn = Error(*)(phys_ptr<void>);

constexpr auto CurrentThread = ThreadId { 0 };

#ifdef IOS_EMULATE_ARM
struct ContextLLE
{
   be2_val<uint32_t> cpsr;
   be2_val<uint32_t> gpr[14];
   be2_val<uint32_t> lr;
   be2_val<uint32_t> pc;
};
CHECK_OFFSET(ContextLLE, 0x00, cpsr);
CHECK_OFFSET(ContextLLE, 0x04, gpr);
CHECK_OFFSET(ContextLLE, 0x3C, lr);
CHECK_OFFSET(ContextLLE, 0x40, pc);
CHECK_SIZE(ContextLLE, 0x44);
#else
struct ContextHLE
{
   ThreadEntryFn entryPoint;
   phys_ptr<void> entryPointArg;
   platform::Fiber *fiber;
   Error queueWaitResult;
   PADDING(0x2C);
};
CHECK_SIZE(ContextHLE, 0x44);
#endif

struct Thread
{
   be2_struct<ContextHLE> context;

   //! Link to next item in the thread queue.
   be2_phys_ptr<Thread> threadQueueNext;

   be2_val<ThreadPriority> minPriority;
   be2_val<ThreadPriority> priority;
   be2_val<ThreadState> state;
   be2_val<ProcessId> pid;
   be2_val<ThreadId> id;
   be2_val<ThreadFlags> flags;
   be2_val<Error> exitValue;

   //! Queue of threads waiting to join this thread.
   be2_struct<ThreadQueue> joinQueue;

   //! The thread queue this therad is currently in.
   be2_phys_ptr<ThreadQueue> threadQueue;

   UNKNOWN(0xA4 - 0x6C);
   be2_phys_ptr<void> stackPointer;
   UNKNOWN(0x8);
   be2_phys_ptr<void> sysStackAddr;
   be2_phys_ptr<void> userStackAddr;
   be2_val<uint32_t> userStackSize;
   be2_phys_ptr<void> ipcBufferPool;
   be2_val<uint32_t> profileCount;
   be2_val<uint32_t> profileTime;
};
CHECK_OFFSET(Thread, 0, context);
CHECK_OFFSET(Thread, 0x44, threadQueueNext);
CHECK_OFFSET(Thread, 0x48, minPriority);
CHECK_OFFSET(Thread, 0x4C, priority);
CHECK_OFFSET(Thread, 0x50, state);
CHECK_OFFSET(Thread, 0x54, pid);
CHECK_OFFSET(Thread, 0x58, id);
CHECK_OFFSET(Thread, 0x5C, flags);
CHECK_OFFSET(Thread, 0x60, exitValue);
CHECK_OFFSET(Thread, 0x64, joinQueue);
CHECK_OFFSET(Thread, 0x68, threadQueue);
CHECK_OFFSET(Thread, 0xA4, stackPointer);
CHECK_OFFSET(Thread, 0xB0, sysStackAddr);
CHECK_OFFSET(Thread, 0xB4, userStackAddr);
CHECK_OFFSET(Thread, 0xB8, userStackSize);
CHECK_OFFSET(Thread, 0xBC, ipcBufferPool);
CHECK_OFFSET(Thread, 0xC0, profileCount);
CHECK_OFFSET(Thread, 0xC4, profileTime);
CHECK_SIZE(Thread, 0xC8);

#pragma pack(pop)

Error
IOS_CreateThread(ThreadEntryFn entry,
                 phys_ptr<void> context,
                 phys_ptr<uint8_t> stackTop,
                 uint32_t stackSize,
                 int priority,
                 ThreadFlags flags);

Error
IOS_JoinThread(ThreadId id,
               phys_ptr<int32_t> returnedValue);

Error
IOS_CancelThread(ThreadId id,
                 Error exitValue);

Error
IOS_StartThread(ThreadId id);

Error
IOS_SuspendThread(ThreadId id);

Error
IOS_YieldCurrentThread();

Error
IOS_GetCurrentThreadId();

Error
IOS_GetThreadPriority(ThreadId id);

Error
IOS_SetThreadPriority(ThreadId id,
                      ThreadPriority priority);

namespace internal
{

phys_ptr<Thread>
getCurrentThread();

ThreadId
getCurrentThreadId();

phys_ptr<Thread>
getThread(ThreadId id);

void
initialiseStaticThreadData();

} // namespace internal

} // namespace ios::kernel
