#include "coreinit.h"
#include "coreinit_spinlock.h"
#include "coreinit_thread.h"
#include "processor.h"
#include <atomic>

void
OSInitSpinLock(OSSpinLock *spinlock)
{
   spinlock->owner = 0;
   spinlock->recursion = 0;
}

BOOL
OSAcquireSpinLock(OSSpinLock *spinlock)
{
   auto owner = gMemory.untranslate(OSGetCurrentThread());
   auto expected = 0u;
   OSTestThreadCancel();

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
OSTryAcquireSpinLock(OSSpinLock *spinlock)
{
   auto owner = gMemory.untranslate(OSGetCurrentThread());
   auto expected = 0u;
   OSTestThreadCancel();

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
OSTryAcquireSpinLockWithTimeout(OSSpinLock *spinlock, int64_t timeout)
{
   OSTestThreadCancel();
   assert(false);
   return FALSE;
}

BOOL
OSReleaseSpinLock(OSSpinLock *spinlock)
{
   auto owner = gMemory.untranslate(OSGetCurrentThread());
   auto result = TRUE;

   if (spinlock->recursion > 0u) {
      --spinlock->recursion;
   } else if (spinlock->owner.load(std::memory_order_relaxed) == owner) {
      spinlock->owner = 0u;
   } else {
      result = FALSE;
   }

   OSTestThreadCancel();
   return result;
}

BOOL
OSUninterruptibleSpinLock_Acquire(OSSpinLock *spinlock)
{
   return OSAcquireSpinLock(spinlock);
}

BOOL
OSUninterruptibleSpinLock_TryAcquire(OSSpinLock *spinlock)
{
   return OSTryAcquireSpinLock(spinlock);
}

BOOL
OSUninterruptibleSpinLock_TryAcquireWithTimeout(OSSpinLock *spinlock, int64_t timeout)
{
   return OSTryAcquireSpinLockWithTimeout(spinlock, timeout);
}

BOOL
OSUninterruptibleSpinLock_Release(OSSpinLock *spinlock)
{
   return OSReleaseSpinLock(spinlock);
}

void
CoreInit::registerSpinLockFunctions()
{
   RegisterKernelFunction(OSInitSpinLock);
   RegisterKernelFunction(OSAcquireSpinLock);
   RegisterKernelFunction(OSTryAcquireSpinLock);
   RegisterKernelFunction(OSTryAcquireSpinLockWithTimeout);
   RegisterKernelFunction(OSReleaseSpinLock);

   RegisterKernelFunction(OSUninterruptibleSpinLock_Acquire);
   RegisterKernelFunction(OSUninterruptibleSpinLock_TryAcquire);
   RegisterKernelFunction(OSUninterruptibleSpinLock_TryAcquireWithTimeout);
   RegisterKernelFunction(OSUninterruptibleSpinLock_Release);
}
