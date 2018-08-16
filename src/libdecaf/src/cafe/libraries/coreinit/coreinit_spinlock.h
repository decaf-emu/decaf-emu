#pragma once
#include "coreinit_time.h"

#include <atomic>
#include <libcpu/be2_struct.h>

namespace cafe::coreinit
{

/**
 * \defgroup coreinit_spinlock Spinlock
 * \ingroup coreinit
 * @{
 */

#pragma pack(push, 1)

struct OSThread;

struct OSSpinLock
{
   //! Address of OSThread* for owner of this lock.
   std::atomic<be2_virt_ptr<OSThread>> owner;

   UNKNOWN(0x4);

   //! Recursion count of spin lock.
   be2_val<uint32_t> recursion;

   //! Used by OSUninterruptibleSpinLock_{Acquire,Release} to restore previous.
   //! state of interrupts.
   be2_val<BOOL> restoreInterruptState;
};
CHECK_OFFSET(OSSpinLock, 0x0, owner);
CHECK_OFFSET(OSSpinLock, 0x8, recursion);
CHECK_OFFSET(OSSpinLock, 0xC, restoreInterruptState);
CHECK_SIZE(OSSpinLock, 0x10);

#pragma pack(pop)

void
OSInitSpinLock(virt_ptr<OSSpinLock> spinlock);

BOOL
OSAcquireSpinLock(virt_ptr<OSSpinLock> spinlock);

BOOL
OSTryAcquireSpinLock(virt_ptr<OSSpinLock> spinlock);

BOOL
OSTryAcquireSpinLockWithTimeout(virt_ptr<OSSpinLock> spinlock,
                                OSTimeNanoseconds timeoutNS);

BOOL
OSReleaseSpinLock(virt_ptr<OSSpinLock> spinlock);

BOOL
OSUninterruptibleSpinLock_Acquire(virt_ptr<OSSpinLock> spinlock);

BOOL
OSUninterruptibleSpinLock_TryAcquire(virt_ptr<OSSpinLock> spinlock);

BOOL
OSUninterruptibleSpinLock_TryAcquireWithTimeout(virt_ptr<OSSpinLock> spinlock,
                                                OSTimeNanoseconds timeoutNS);

BOOL
OSUninterruptibleSpinLock_Release(virt_ptr<OSSpinLock> spinlock);

struct ScopedSpinLock
{
   ScopedSpinLock(virt_ptr<OSSpinLock> lock_) :
      lock(lock_)
   {
      OSUninterruptibleSpinLock_Acquire(lock);
   }

   ~ScopedSpinLock()
   {
      OSUninterruptibleSpinLock_Release(lock);
   }

   virt_ptr<OSSpinLock> lock;
};

/** @} */

} // namespace cafe::coreinit
