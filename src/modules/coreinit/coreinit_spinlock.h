#pragma once
#include <atomic>
#include "systemtypes.h"
#include "coreinit_thread.h"

#pragma pack(push, 1)

struct OSSpinLock
{
   std::atomic<uint32_t> owner;
   UNKNOWN(0x4);
   be_val<uint32_t> recursion;
   UNKNOWN(0x4);
};
CHECK_OFFSET(OSSpinLock, 0x0, owner);
CHECK_OFFSET(OSSpinLock, 0x8, recursion);
CHECK_SIZE(OSSpinLock, 0x10);

#pragma pack(pop)

void OSInitSpinLock(OSSpinLock *spinlock);
BOOL OSAcquireSpinLock(OSSpinLock *spinlock);
BOOL OSTryAcquireSpinLock(OSSpinLock *spinlock);
BOOL OSTryAcquireSpinLockWithTimeout(OSSpinLock *spinlock, int64_t timeout);
BOOL OSReleaseSpinLock(OSSpinLock *spinlock);

BOOL OSUninterruptibleSpinLock_Acquire(OSSpinLock *spinlock);
BOOL OSUninterruptibleSpinLock_TryAcquire(OSSpinLock *spinlock);
BOOL OSUninterruptibleSpinLock_TryAcquireWithTimeout(OSSpinLock *spinlock, int64_t timeout);
BOOL OSUninterruptibleSpinLock_Release(OSSpinLock *spinlock);
