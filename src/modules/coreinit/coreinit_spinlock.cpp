#include "coreinit.h"
#include "coreinit_spinlock.h"
#include "coreinit_thread.h"
#include <atomic>

// TODO: Rewrite using std::atomic

void
OSInitSpinLock(OSSpinLock * spinlock)
{
   spinlock->owner = 0;
   spinlock->recursion = 0;
}

BOOL
OSAcquireSpinLock(OSSpinLock * spinlock)
{
   auto owner = static_cast<uint32_t>(OSGetCurrentThread());
   auto expected = 0u;

   if (spinlock->owner.load(std::memory_order_relaxed) == owner) {
      ++spinlock->recursion;
      return TRUE;
   }

   // Spin until acquired
   while (!spinlock->owner.compare_exchange_weak(expected, owner, std::memory_order_release, std::memory_order_relaxed)) {
      expected = 0;
   }

   return TRUE;
}

BOOL
OSTryAcquireSpinLock(OSSpinLock * spinlock)
{
   auto owner = static_cast<uint32_t>(OSGetCurrentThread());
   auto expected = 0u;

   if (spinlock->owner.load(std::memory_order_relaxed) == owner) {
      ++spinlock->recursion;
      return TRUE;
   }

   if (spinlock->owner.compare_exchange_weak(expected, owner, std::memory_order_release, std::memory_order_relaxed)) {
      return TRUE;
   } else {
      return FALSE;
   }
}

BOOL
OSTryAcquireSpinLockWithTimeout(OSSpinLock * spinlock, int64_t timeout)
{
   xError() << "TODO: OSTryAcquireSpinLockWithTimeout";
   return FALSE;
}

BOOL
OSReleaseSpinLock(OSSpinLock * spinlock)
{
   auto owner = static_cast<uint32_t>(OSGetCurrentThread());

   if (spinlock->recursion > 0) {
      --spinlock->recursion;
      return TRUE;
   }

   if (spinlock->owner.load(std::memory_order_relaxed) == owner) {
      spinlock->owner = 0;
      return TRUE;
   }

   return FALSE;
}

BOOL
OSUninterruptibleSpinLock_Acquire(OSSpinLock * spinlock)
{
   return OSAcquireSpinLock(spinlock);
}

BOOL
OSUninterruptibleSpinLock_TryAcquire(OSSpinLock * spinlock)
{
   return OSTryAcquireSpinLock(spinlock);
}

BOOL
OSUninterruptibleSpinLock_TryAcquireWithTimeout(OSSpinLock * spinlock, int64_t timeout)
{
   return OSTryAcquireSpinLockWithTimeout(spinlock, timeout);
}

BOOL
OSUninterruptibleSpinLock_Release(OSSpinLock * spinlock)
{
   return OSReleaseSpinLock(spinlock);
}

void
CoreInit::registerSpinLockFunctions()
{
   RegisterSystemFunction(OSInitSpinLock);
   RegisterSystemFunction(OSAcquireSpinLock);
   RegisterSystemFunction(OSTryAcquireSpinLock);
   RegisterSystemFunction(OSTryAcquireSpinLockWithTimeout);
   RegisterSystemFunction(OSReleaseSpinLock);

   RegisterSystemFunction(OSUninterruptibleSpinLock_Acquire);
   RegisterSystemFunction(OSUninterruptibleSpinLock_TryAcquire);
   RegisterSystemFunction(OSUninterruptibleSpinLock_TryAcquireWithTimeout);
   RegisterSystemFunction(OSUninterruptibleSpinLock_Release);
}
