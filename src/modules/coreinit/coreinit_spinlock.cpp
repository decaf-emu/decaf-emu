#include <atomic>
#include "coreinit.h"
#include "coreinit_interrupts.h"
#include "coreinit_spinlock.h"
#include "coreinit_thread.h"
#include "memory_translate.h"

namespace coreinit
{

static bool
spinAcquireLock(OSSpinLock *spinlock)
{
   auto owner = memory_untranslate(OSGetCurrentThread());

   if (spinlock->owner.load(std::memory_order_relaxed) == owner) {
      ++spinlock->recursion;
      return false;
   }

   uint32_t expected = 0;

   while (!spinlock->owner.compare_exchange_weak(expected, owner, std::memory_order_release, std::memory_order_relaxed)) {
      expected = 0;
   }

   return true;
}

static bool
spinTryLock(OSSpinLock *spinlock)
{
   auto owner = memory_untranslate(OSGetCurrentThread());

   if (spinlock->owner.load(std::memory_order_relaxed) == owner) {
      ++spinlock->recursion;
      return true;
   }

   uint32_t expected = 0;

   if (spinlock->owner.compare_exchange_weak(expected, owner, std::memory_order_release, std::memory_order_relaxed)) {
      return true;
   } else {
      return false;
   }
}

static bool
spinTryLockWithTimeout(OSSpinLock *spinlock, OSTime duration)
{
   auto owner = memory_untranslate(OSGetCurrentThread());

   if (spinlock->owner.load(std::memory_order_relaxed) == owner) {
      ++spinlock->recursion;
      return true;
   }

   uint32_t expected = 0;
   auto timeout = OSGetSystemTime() + duration;

   while (!spinlock->owner.compare_exchange_weak(expected, owner, std::memory_order_release, std::memory_order_relaxed)) {
      if (OSGetSystemTime() >= timeout) {
         return false;
      }

      expected = 0;
   }

   return true;
}

static bool
spinReleaseLock(OSSpinLock *spinlock)
{
   auto owner = memory_untranslate(OSGetCurrentThread());

   if (spinlock->recursion > 0u) {
      --spinlock->recursion;
      return false;
   } else if (spinlock->owner.load(std::memory_order_relaxed) == owner) {
      spinlock->owner = 0u;
      return true;
   }

   gLog->error("Attempt to release spin lock which is not owned.");
   return true;
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
   spinAcquireLock(spinlock);
   return TRUE;
}

BOOL
OSTryAcquireSpinLock(OSSpinLock *spinlock)
{
   OSTestThreadCancel();
   return spinTryLock(spinlock) ? TRUE : FALSE;
}

BOOL
OSTryAcquireSpinLockWithTimeout(OSSpinLock *spinlock, OSTime timeout)
{
   OSTestThreadCancel();
   return spinTryLockWithTimeout(spinlock, timeout) ? TRUE : FALSE;
}

BOOL
OSReleaseSpinLock(OSSpinLock *spinlock)
{
   spinReleaseLock(spinlock);
   OSTestThreadCancel();
   return TRUE;
}


/**
 * Acquires an Uninterruptible Spin Lock.
 *
 * Will block until spin lock is acquired.
 * Disables interrupts before returning, only if non recursive lock.
 *
 * \return Returns TRUE if the lock was acquired.
 */
BOOL
OSUninterruptibleSpinLock_Acquire(OSSpinLock *spinlock)
{
   if (spinAcquireLock(spinlock)) {
      spinlock->restoreInterruptState = OSDisableInterrupts();
   }

   return TRUE;
}


/**
 * Try acquire an Uninterruptible Spin Lock.
 *
 * Will return immediately if the lock has already been acquired by another thread.
 * If lock is acquired, interrupts will be disabled.
 *
 * \return Returns TRUE if the lock was acquired.
 */
BOOL
OSUninterruptibleSpinLock_TryAcquire(OSSpinLock *spinlock)
{
   if (!spinTryLock(spinlock)) {
      return FALSE;
   }

   spinlock->restoreInterruptState = OSDisableInterrupts();
   return TRUE;
}


/**
 * Try acquire an Uninterruptible Spin Lock with a timeout.
 *
 * Will return after a timeout if unable to acquire lock.
 * If lock is acquired, interrupts will be disabled.
 *
 * \return Returns TRUE if the lock was acquired.
 */
BOOL
OSUninterruptibleSpinLock_TryAcquireWithTimeout(OSSpinLock *spinlock, OSTime timeout)
{
   if (!spinTryLockWithTimeout(spinlock, timeout)) {
      return FALSE;
   }

   spinlock->restoreInterruptState = OSDisableInterrupts();
   return TRUE;
}


/**
 * Release an Uninterruptible Spin Lock.
 *
 * Interrupts will be restored to their previous state before
 * the lock was acquired.
 *
 * \return Returns TRUE if the lock was released.
 */
BOOL
OSUninterruptibleSpinLock_Release(OSSpinLock *spinlock)
{
   if (spinReleaseLock(spinlock)) {
      OSRestoreInterrupts(spinlock->restoreInterruptState);
   }

   return TRUE;
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
