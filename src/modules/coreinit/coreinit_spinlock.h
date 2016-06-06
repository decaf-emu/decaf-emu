#pragma once
#include <atomic>
#include "coreinit_thread.h"
#include "common/be_val.h"
#include "common/structsize.h"

namespace coreinit
{

/**
 * \defgroup coreinit_spinlock Spinlock
 * \ingroup coreinit
 * @{
 */

#pragma pack(push, 1)

struct OSSpinLock
{
   //! Address of OSThread* for owner of this lock.
   std::atomic<uint32_t> owner;

   UNKNOWN(0x4);

   //! Recursion count of spin lock.
   be_val<uint32_t> recursion;

   //! Used by OSUninterruptibleSpinLock_{Acquire,Release} to restore previous.
   //! state of interrupts.
   be_val<uint32_t> restoreInterruptState;
};
CHECK_OFFSET(OSSpinLock, 0x0, owner);
CHECK_OFFSET(OSSpinLock, 0x8, recursion);
CHECK_OFFSET(OSSpinLock, 0xC, restoreInterruptState);
CHECK_SIZE(OSSpinLock, 0x10);

#pragma pack(pop)

void
OSInitSpinLock(OSSpinLock *spinlock);

BOOL
OSAcquireSpinLock(OSSpinLock *spinlock);

BOOL
OSTryAcquireSpinLock(OSSpinLock *spinlock);

BOOL
OSTryAcquireSpinLockWithTimeout(OSSpinLock *spinlock,
                                OSTime timeout);

BOOL
OSReleaseSpinLock(OSSpinLock *spinlock);

BOOL
OSUninterruptibleSpinLock_Acquire(OSSpinLock *spinlock);

BOOL
OSUninterruptibleSpinLock_TryAcquire(OSSpinLock *spinlock);

BOOL
OSUninterruptibleSpinLock_TryAcquireWithTimeout(OSSpinLock *spinlock,
                                                OSTime timeout);

BOOL
OSUninterruptibleSpinLock_Release(OSSpinLock *spinlock);

struct ScopedSpinLock
{
   ScopedSpinLock(OSSpinLock *lock) :
      lock(lock)
   {
      OSUninterruptibleSpinLock_Acquire(lock);
   }

   ~ScopedSpinLock()
   {
      OSUninterruptibleSpinLock_Release(lock);
   }

   OSSpinLock *lock;
};

/** @} */

} // namespace coreinit
