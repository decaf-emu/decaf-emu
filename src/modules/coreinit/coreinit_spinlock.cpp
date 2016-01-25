#include <atomic>
#include "coreinit.h"
#include "coreinit_spinlock.h"
#include "coreinit_thread.h"
#include "memory_translate.h"
#include "processor.h"

namespace coreinit
{

static void
spinLock(OSSpinLock *spinlock)
{
   uint32_t owner, expected;
   auto thread = OSGetCurrentThread();

   if (!thread) {
      return;
   }

   owner = memory_untranslate(thread);

   if (spinlock->owner.load(std::memory_order_relaxed) == owner) {
      ++spinlock->recursion;
      return;
   }

   expected = 0;

   while (!spinlock->owner.compare_exchange_weak(expected, owner, std::memory_order_release, std::memory_order_relaxed)) {
      expected = 0;
   }
}

static BOOL
spinTryLock(OSSpinLock *spinlock)
{
   uint32_t owner, expected;
   auto thread = OSGetCurrentThread();

   if (!thread) {
      return FALSE;
   }

   owner = memory_untranslate(thread);

   if (spinlock->owner.load(std::memory_order_relaxed) == owner) {
      ++spinlock->recursion;
      return TRUE;
   }

   expected = 0;

   if (spinlock->owner.compare_exchange_weak(expected, owner, std::memory_order_release, std::memory_order_relaxed)) {
      return TRUE;
   } else {
      return FALSE;
   }
}

static BOOL
spinReleaseLock(OSSpinLock *spinlock)
{
   uint32_t owner;
   auto thread = OSGetCurrentThread();

   if (!thread) {
      return FALSE;
   }

   owner = memory_untranslate(OSGetCurrentThread());

   if (spinlock->recursion > 0u) {
      --spinlock->recursion;
      return TRUE;
   } else if (spinlock->owner.load(std::memory_order_relaxed) == owner) {
      spinlock->owner = 0u;
      return TRUE;
   }

   return FALSE;
}

void
OSInitSpinLock(OSSpinLock *spinlock)
{
   spinlock->owner = 0;
   spinlock->recursion = 0;
}

BOOL
OSAcquireSpinLock(OSSpinLock *spinlock)
{
   OSTestThreadCancel();
   spinLock(spinlock);
   return TRUE;
}

BOOL
OSTryAcquireSpinLock(OSSpinLock *spinlock)
{
   OSTestThreadCancel();
   return spinTryLock(spinlock);
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
   auto result = spinReleaseLock(spinlock);
   OSTestThreadCancel();
   return result;
}

BOOL
OSUninterruptibleSpinLock_Acquire(OSSpinLock *spinlock)
{
   spinLock(spinlock);
   return TRUE;
}

BOOL
OSUninterruptibleSpinLock_TryAcquire(OSSpinLock *spinlock)
{
   return spinTryLock(spinlock);
}

BOOL
OSUninterruptibleSpinLock_TryAcquireWithTimeout(OSSpinLock *spinlock, int64_t timeout)
{
   assert(false);
   return FALSE;
}

BOOL
OSUninterruptibleSpinLock_Release(OSSpinLock *spinlock)
{
   return spinReleaseLock(spinlock);
}

void
Module::registerSpinLockFunctions()
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

} // namespace coreinit
