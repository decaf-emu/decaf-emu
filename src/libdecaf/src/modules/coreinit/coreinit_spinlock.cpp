#include "coreinit.h"
#include "coreinit_interrupts.h"
#include "coreinit_spinlock.h"
#include "coreinit_scheduler.h"
#include "coreinit_thread.h"
#include "libcpu/mem.h"
#include "common/decaf_assert.h"
#include <atomic>

namespace coreinit
{

static void
increaseSpinLockCount(OSThread *thread)
{
   internal::lockScheduler();
   thread->context.spinLockCount++;
   thread->priority = 0;
   internal::unlockScheduler();
}

static void
decreaseSpinLockCount(OSThread *thread)
{
   internal::lockScheduler();
   thread->context.spinLockCount--;
   thread->priority = internal::calculateThreadPriorityNoLock(thread);
   internal::unlockScheduler();
}

static bool
spinAcquireLock(OSSpinLock *spinlock)
{
   auto thread = OSGetCurrentThread();
   auto owner = mem::untranslate(thread);

   if (spinlock->owner.load(std::memory_order_acquire) == owner) {
      ++spinlock->recursion;
      return false;
   }

   auto expected = be_val<uint32_t> { 0 };

   while (!spinlock->owner.compare_exchange_weak(expected, owner, std::memory_order_release, std::memory_order_relaxed)) {
      expected = 0;
   }

   increaseSpinLockCount(thread);
   return true;
}

static bool
spinTryLock(OSSpinLock *spinlock)
{
   auto thread = OSGetCurrentThread();
   auto owner = mem::untranslate(thread);

   if (spinlock->owner.load(std::memory_order_acquire) == owner) {
      ++spinlock->recursion;
      return true;
   }

   auto expected = be_val<uint32_t> { 0 };

   if (spinlock->owner.compare_exchange_weak(expected, owner, std::memory_order_release, std::memory_order_relaxed)) {
      increaseSpinLockCount(thread);
      return true;
   } else {
      return false;
   }
}

static bool
spinTryLockWithTimeout(OSSpinLock *spinlock,
                       OSTime durationTicks)
{
   auto thread = OSGetCurrentThread();
   auto owner = mem::untranslate(thread);

   if (spinlock->owner.load(std::memory_order_acquire) == owner) {
      ++spinlock->recursion;
      return true;
   }

   auto expected = be_val<uint32_t> { 0 };
   auto timeout = OSGetSystemTime() + durationTicks;

   while (!spinlock->owner.compare_exchange_weak(expected, owner, std::memory_order_release, std::memory_order_relaxed)) {
      if (OSGetSystemTime() >= timeout) {
         return false;
      }

      expected = 0;
   }

   increaseSpinLockCount(thread);
   return true;
}

static bool
spinReleaseLock(OSSpinLock *spinlock)
{
   auto thread = OSGetCurrentThread();
   auto owner = mem::untranslate(thread);

   if (spinlock->recursion > 0u) {
      --spinlock->recursion;
      return false;
   } else if (spinlock->owner.load(std::memory_order_acquire) == owner) {
      spinlock->owner = 0u;
      decreaseSpinLockCount(thread);
      return true;
   }

   decaf_abort("Attempt to release spin lock which is not owned.");
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
OSTryAcquireSpinLockWithTimeout(OSSpinLock *spinlock,
                                OSTime timeoutNS)
{
   auto timeoutTicks = internal::nanosToTicks(timeoutNS);
   OSTestThreadCancel();
   return spinTryLockWithTimeout(spinlock, timeoutTicks) ? TRUE : FALSE;
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

   // Update thread's cancel state
   if (auto thread = OSGetCurrentThread()) {
      thread->cancelState |= OSThreadCancelState::DisabledBySpinlock;
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

   // Update thread's cancel state
   if (auto thread = OSGetCurrentThread()) {
      thread->cancelState |= OSThreadCancelState::DisabledBySpinlock;
   }

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
OSUninterruptibleSpinLock_TryAcquireWithTimeout(OSSpinLock *spinlock,
                                                OSTime timeoutNS)
{
   auto timeoutTicks = internal::nanosToTicks(timeoutNS);

   if (!spinTryLockWithTimeout(spinlock, timeoutTicks)) {
      return FALSE;
   }

   spinlock->restoreInterruptState = OSDisableInterrupts();

   // Update thread's cancel state
   if (auto thread = OSGetCurrentThread()) {
      thread->cancelState |= OSThreadCancelState::DisabledBySpinlock;
   }

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

   // Update thread's cancel state
   if (auto thread = OSGetCurrentThread()) {
      thread->cancelState &= ~OSThreadCancelState::DisabledBySpinlock;
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
