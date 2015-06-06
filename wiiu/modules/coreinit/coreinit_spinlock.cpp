#include "coreinit.h"
#include "coreinit_spinlock.h"
#include "coreinit_thread.h"

// TODO: Rewrite using std::atomic

void
OSInitSpinLock(OSSpinLock * spinlock)
{
   spinlock->owner = nullptr;
   spinlock->recursion = 0;
}

BOOL
OSAcquireSpinLock(OSSpinLock * spinlock)
{
   auto curThread = OSGetCurrentThread();

   if (spinlock->owner == curThread) {
      ++spinlock->recursion;
      return TRUE;
   }

   // Spin until spinlock owner is nullptr
   while (_InterlockedCompareExchange(reinterpret_cast<long*>(&spinlock->owner.value), curThread.value, 0));

   return TRUE;
}

BOOL
OSTryAcquireSpinLock(OSSpinLock * spinlock)
{
   auto curThread = OSGetCurrentThread();

   if (spinlock->owner == curThread) {
      ++spinlock->recursion;
      return TRUE;
   }

   if (_InterlockedCompareExchange(reinterpret_cast<long*>(&spinlock->owner.value), curThread.value, 0)) {
      return FALSE;
   } else {
      return TRUE;
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
   auto curThread = OSGetCurrentThread();

   if (spinlock->recursion > 0) {
      --spinlock->recursion;
      return TRUE;
   }

   if (spinlock->owner == curThread) {
      spinlock->owner = nullptr;
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
