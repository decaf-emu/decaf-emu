#pragma once
#include "ios_kernel_threadqueue.h"
#include "ios/ios_enum.h"

#include <common/cbool.h>
#include <common/structsize.h>
#include <libcpu/be2_struct.h>

namespace ios::kernel
{

// Max num semaphores: 750

// SemaphoreID seems to be (id | (something << 12));
using SemaphoreID = int32_t;

struct Thread;

struct Semaphore
{
   //! List of threads waiting on this semaphore, ordered by priority.
   be2_struct<ThreadQueue> waitThreadQueue;

   //! Unknown list of threads.
   be2_phys_ptr<Thread> unknown0x04;

   //! Unique semaphore identifier.
   be2_val<SemaphoreID> id;

   //! Process this semaphore belongs to.
   be2_val<ProcessID> pid;

   //! Current semaphore signal count.
   be2_val<int32_t> count;

   //! Maximum semaphore signal count.
   be2_val<int32_t> maxCount;

   //! Previous free semaphore, linked list.
   be2_val<int16_t> prevFreeSemaphoreIndex;

   //! Next free semaphore, linked list.
   be2_val<int16_t> nextFreeSemaphoreIndex;
};
CHECK_OFFSET(Semaphore, 0x00, waitThreadQueue);
CHECK_OFFSET(Semaphore, 0x04, unknown0x04);
CHECK_OFFSET(Semaphore, 0x08, id);
CHECK_OFFSET(Semaphore, 0x0C, pid);
CHECK_OFFSET(Semaphore, 0x10, count);
CHECK_OFFSET(Semaphore, 0x14, maxCount);
CHECK_OFFSET(Semaphore, 0x18, prevFreeSemaphoreIndex);
CHECK_OFFSET(Semaphore, 0x1A, nextFreeSemaphoreIndex);
CHECK_SIZE(Semaphore, 0x1C);

Error
IOS_CreateSemaphore(int32_t maxCount,
                    int32_t initialCount);

Error
IOS_DestroySempahore(SemaphoreID id);

Error
IOS_WaitSemaphore(SemaphoreID id,
                  BOOL tryWait);

Error
IOS_SignalSempahore(SemaphoreID id);

} // namespace ios::kernel
