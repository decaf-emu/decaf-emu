#include "ios/ios_enum.h"
#include "ios_kernel.h"
#include "ios_kernel_thread.h"
#include "ios_kernel_scheduler.h"

#include <common/log.h>
#include <functional>

namespace ios::kernel
{

struct ProcessInfo
{
   std::function<int32_t(ProcessId)> entryPoint;
   int32_t priority;
   uint32_t stackSize;
   uint32_t stackPtr;
   uint32_t memPermMask;
};

ProcessInfo x[] = {
   ProcessInfo { nullptr, 0x7C, 0x2000, 0x50BA4A0, 0xC0030 }, // IOS_MCP
};

/*
ProcessInfo <0xB,   0, 9, j__Reset,             0x7D,     0, 0x7E,          0,       0x7F,          0,       0x80,          0>
ProcessInfo <0xB,   1, 9, Launch_IOS_MCP,       0x7D,  0x7C, 0x7E,     0x2000,       0x7F,  0x50BA4A0,       0x80,    0xC0030>
ProcessInfo <0xB,   2, 9, Launch_IOS_BSP,       0x7D,  0x7D, 0x7E,     0x1000,       0x7F, 0xE7000000,       0x80,   0x100000>
ProcessInfo <0xB,   3, 9, Launch_IOS_CRYPTO,    0x7D,  0x7B, 0x7E,     0x1000,       0x7F,  0x4028628,       0x80,    0xC0030>
ProcessInfo <0xB,   4, 9, Launch_IOS_USB,       0x7D,  0x6B, 0x7E,     0x4000,       0x7F, 0x104B92C8,       0x80,    0x38600>
ProcessInfo <0xB,   5, 9, Launch_IOS_FS,        0x7D,  0x55, 0x7E,     0x4000,       0x7F, 0x1114117C,       0x80,   0x1C5870>
ProcessInfo <0xB,   6, 9, Launch_IOS_PAD,       0x7D,  0x75, 0x7E,     0x2000,       0x7F, 0x1214AB4C,       0x80,     0x8180>
ProcessInfo <0xB,   7, 9, Launch_IOS_NET,       0x7D,  0x50, 0x7E,     0x4000,       0x7F, 0x12804498,       0x80,     0x2000>
ProcessInfo <0xB, 0xB, 9, Launch_IOS_NIM_BOSS,  0x7D,  0x32, 0x7E,     0x4000,       0x7F, 0xE22CB000,       0x80,          0>
ProcessInfo <0xB,   9, 9, Launch_IOS_NSEC,      0x7D,  0x32, 0x7E,     0x1000,       0x7F, 0xE12E71A4,       0x80,          0>
ProcessInfo <0xB, 0xC, 9, Launch_IOS_FPD,       0x7D,  0x32, 0x7E,     0x4000,       0x7F, 0xE31AF000,       0x80,          0>
ProcessInfo <0xB,   8, 9, Launch_IOS_ACP,       0x7D,  0x32, 0x7E,     0x4000,       0x7F, 0xE0125390,       0x80,          0>
ProcessInfo <0xB, 0xA, 9, Launch_IOS_AUXIL,     0x7D,  0x46, 0x7E,     0x4000,       0x7F, 0xE506A900,       0x80,          0>
ProcessInfo <0xB, 0xD, 9, Launch_IOS_TEST,      0x7D,  0x4B, 0x7E,     0x2000,       0x7F, 0xE415623C,       0x80,          0>
*/
static void
setupMMU()
{

}

Error
kernelEntryPoint(phys_ptr<void> context)
{
   setupMMU();

   // TODO: Initialise semaphore list
   // TODO: Initialise heap list

   // TODO: Initialise kernel timer

   // TODO: Create heap 0x1D000000, 0x2B00000

   // TODO: IOS_CreateCrossProcessHeap 0x20000

   // TODO: Set thread to 124 priority
   // TODO: Startup BSP
   // TODO: Wait for BSP startup complete
   // TODO: Set thread to 126 priority

   // TODO: Create a timer at +5000us running at 1 second intervals
   // TODO: Wait for timer to proc once

   // Now loop on that timer
   // Once a second we're checking the status of the system, like if threads are
   // in valid states.
   return Error::OK;
}

// Kernel thread has priority 126, stack size 0x2000 at fixed location 0xFFFF4CF8

Error
start()
{
   // Initialise memory
   internal::kernelInitialiseThread();

   // Create root kernel thread
   auto error = IOS_CreateThread(kernelEntryPoint,
                                 nullptr,
                                 phys_addr { 0xFFFF4CF8 },
                                 0x2000u,
                                 126,
                                 ThreadFlags::Detached);
   if (error < Error::OK) {
      return error;
   }

   // Force start the root kernel thread. We cannot use IOS_StartThread
   // because it reschedules and we are not running on an IOS thread.
   internal::lockScheduler();
   auto threadId = static_cast<ThreadId>(error);
   auto thread = internal::getThread(threadId);
   thread->state = ThreadState::Ready;
   internal::queueThreadNoLock(thread);
   internal::unlockScheduler();
   return Error::OK;
}

} // namespace ios::kernel
