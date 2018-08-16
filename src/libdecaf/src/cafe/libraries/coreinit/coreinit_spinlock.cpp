#include "coreinit.h"
#include "coreinit_interrupts.h"
#include "coreinit_spinlock.h"
#include "coreinit_scheduler.h"
#include "coreinit_thread.h"
#include "libcpu/mem.h"
#include <common/decaf_assert.h>
#include <atomic>

namespace cafe::coreinit
{

static void
increaseSpinLockCount(virt_ptr<OSThread> thread)
{
   internal::lockScheduler();
   thread->context.spinLockCount++;
   thread->priority = 0;
   internal::unlockScheduler();
}

static void
decreaseSpinLockCount(virt_ptr<OSThread> thread)
{
   internal::lockScheduler();
   thread->context.spinLockCount--;
   thread->priority = internal::calculateThreadPriorityNoLock(thread);
   internal::unlockScheduler();
}

static bool
spinAcquireLock(virt_ptr<OSSpinLock> spinlock)
{
   auto thread = OSGetCurrentThread();
   if (spinlock->owner.load(std::memory_order_acquire) == thread) {
      ++spinlock->recursion;
      return false;
   }

   auto expected = be2_virt_ptr<OSThread> { nullptr };
   auto owner = be2_virt_ptr<OSThread> { thread };

   while (!spinlock->owner.compare_exchange_weak(expected, owner, std::memory_order_release, std::memory_order_relaxed)) {
      expected = nullptr;
   }

   increaseSpinLockCount(thread);
   return true;
}

static bool
spinTryLock(virt_ptr<OSSpinLock> spinlock)
{
   auto thread = OSGetCurrentThread();
   if (spinlock->owner.load(std::memory_order_acquire) == thread) {
      ++spinlock->recursion;
      return true;
   }

   auto expected = be2_virt_ptr<OSThread> { nullptr };
   auto owner = be2_virt_ptr<OSThread> { thread };

   if (spinlock->owner.compare_exchange_weak(expected, owner, std::memory_order_release, std::memory_order_relaxed)) {
      increaseSpinLockCount(thread);
      return true;
   } else {
      return false;
   }
}

static bool
spinTryLockWithTimeout(virt_ptr<OSSpinLock> spinlock,
                       OSTime durationTicks)
{
   auto thread = OSGetCurrentThread();
   if (spinlock->owner.load(std::memory_order_acquire) == thread) {
      ++spinlock->recursion;
      return true;
   }

   auto expected = be2_virt_ptr<OSThread> { nullptr };
   auto owner = be2_virt_ptr<OSThread> { thread };
   auto timeout = OSGetSystemTime() + durationTicks;

   while (!spinlock->owner.compare_exchange_weak(expected, owner, std::memory_order_release, std::memory_order_relaxed)) {
      if (OSGetSystemTime() >= timeout) {
         return false;
      }

      expected = nullptr;
   }

   increaseSpinLockCount(thread);
   return true;
}

static bool
spinReleaseLock(virt_ptr<OSSpinLock> spinlock)
{
   auto thread = OSGetCurrentThread();
   if (spinlock->recursion > 0u) {
      --spinlock->recursion;
      return false;
   }

   auto owner = be2_virt_ptr<OSThread> { thread };
   if (spinlock->owner.load(std::memory_order_acquire) == owner) {
      spinlock->owner.store(be2_virt_ptr<OSThread> { nullptr });
      decreaseSpinLockCount(thread);
      return true;
   }

   decaf_abort("Attempt to release spin lock which is not owned.");
   return true;
}

void
OSInitSpinLock(virt_ptr<OSSpinLock> spinlock)
{
   spinlock->owner.store(be2_virt_ptr<OSThread> { nullptr });
   spinlock->recursion = 0u;
}

BOOL
OSAcquireSpinLock(virt_ptr<OSSpinLock> spinlock)
{
   OSTestThreadCancel();
   spinAcquireLock(spinlock);
   return TRUE;
}

BOOL
OSTryAcquireSpinLock(virt_ptr<OSSpinLock> spinlock)
{
   OSTestThreadCancel();
   return spinTryLock(spinlock) ? TRUE : FALSE;
}

BOOL
OSTryAcquireSpinLockWithTimeout(virt_ptr<OSSpinLock> spinlock,
                                OSTimeNanoseconds timeoutNS)
{
   auto timeoutTicks = internal::nsToTicks(timeoutNS);
   OSTestThreadCancel();
   return spinTryLockWithTimeout(spinlock, timeoutTicks) ? TRUE : FALSE;
}

BOOL
OSReleaseSpinLock(virt_ptr<OSSpinLock> spinlock)
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
OSUninterruptibleSpinLock_Acquire(virt_ptr<OSSpinLock> spinlock)
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
OSUninterruptibleSpinLock_TryAcquire(virt_ptr<OSSpinLock> spinlock)
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
OSUninterruptibleSpinLock_TryAcquireWithTimeout(virt_ptr<OSSpinLock> spinlock,
                                                OSTimeNanoseconds timeoutNS)
{
   auto timeoutTicks = internal::nsToTicks(timeoutNS);

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
OSUninterruptibleSpinLock_Release(virt_ptr<OSSpinLock> spinlock)
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
Library::registerSpinLockSymbols()
{
   RegisterFunctionExport(OSInitSpinLock);
   RegisterFunctionExport(OSAcquireSpinLock);
   RegisterFunctionExport(OSTryAcquireSpinLock);
   RegisterFunctionExport(OSTryAcquireSpinLockWithTimeout);
   RegisterFunctionExport(OSReleaseSpinLock);
   RegisterFunctionExport(OSUninterruptibleSpinLock_Acquire);
   RegisterFunctionExport(OSUninterruptibleSpinLock_TryAcquire);
   RegisterFunctionExport(OSUninterruptibleSpinLock_TryAcquireWithTimeout);
   RegisterFunctionExport(OSUninterruptibleSpinLock_Release);
}

} // namespace cafe::coreinit
