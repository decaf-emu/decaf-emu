#include "coreinit.h"
#include "coreinit_fastmutex.h"
#include "coreinit_scheduler.h"

namespace cafe::coreinit
{

using FastMutexQueue = internal::Queue<OSFastMutexQueue, OSFastMutexLink, OSFastMutex, &OSFastMutex::link>;
using ContendedQueue = internal::Queue<OSFastMutexQueue, OSFastMutexLink, OSFastMutex, &OSFastMutex::contendedLink>;
using ThreadSimpleQueue = internal::SortedQueue<OSThreadSimpleQueue, OSThreadLink, OSThread, &OSThread::link, internal::ThreadIsLess>;

/**
 * Initialise a fast mutex object.
 */
void
OSFastMutex_Init(virt_ptr<OSFastMutex> mutex,
                 virt_ptr<const char> name)
{
   decaf_check(mutex);
   mutex->tag = OSFastMutex::Tag;
   mutex->name = name;
   mutex->isContended = FALSE;
   mutex->lock.store(0);
   mutex->count = 0;
   ThreadSimpleQueue::init(virt_addrof(mutex->queue));
   FastMutexQueue::initLink(mutex);
   ContendedQueue::initLink(mutex);
}

static void
fastMutexHardLock(virt_ptr<OSFastMutex> mutex)
{
   auto thread = OSGetCurrentThread();
   decaf_check(thread->state == OSThreadState::Running);

   internal::lockScheduler();
   internal::testThreadCancelNoLock();

   auto lockValue = mutex->lock.load();

   while (true) {
      if (lockValue) {
         // Check if waiter bit is set, if not we must try set it
         if (!(lockValue & 1)) {
            if (!mutex->lock.compare_exchange_weak(lockValue, lockValue | 1)) {
               continue;
            }
         }

         // We now have set the waiter bit and we were not the owner
         auto ownerThread = virt_cast<OSThread *>(virt_addr { lockValue & ~1 });

         if (!mutex->isContended) {
            ContendedQueue::append(virt_addrof(thread->contendedFastMutexes), mutex);
            mutex->isContended = TRUE;
         }

         // Record which fast mutex we are trying to lock before we sleep
         thread->fastMutex = mutex;

         // Promote the priority of the owning thread to prevent priority inversion problems
         internal::promoteThreadPriorityNoLock(ownerThread, thread->priority);

         // Sleep on the queue waiting for a hard unlock
         internal::sleepThreadNoLock(virt_addrof(mutex->queue));
         internal::rescheduleSelfNoLock();

         // We are no longer attempting to lock this fast mutex
         thread->fastMutex = nullptr;

         lockValue = mutex->lock.load();

         if (lockValue != 0) {
            continue;
         }
      }

      // Try to lock the FastMutex
      auto newValue = static_cast<uint32_t>(virt_cast<virt_addr>(thread));
      if (!mutex->lock.compare_exchange_weak(lockValue, newValue)) {
         continue;
      }

      decaf_check(!(lockValue & 1));
      decaf_check(mutex->count == 0);

      // Set thread as owner
      thread->cancelState |= OSThreadCancelState::DisabledByFastMutex;
      FastMutexQueue::append(virt_addrof(thread->fastMutexQueue), mutex);
      mutex->count = 1;
      break;
   }

   internal::unlockScheduler();
}


/**
 * Lock a fast mutex object.
 */
void
OSFastMutex_Lock(virt_ptr<OSFastMutex> mutex)
{
   decaf_check(mutex);

   // HACK: Naughty games not initialising mutex before using it.
   //decaf_check(mutex->tag == OSFastMutex::Tag);
   if (mutex->tag != OSFastMutex::Tag) {
      OSFastMutex_Init(mutex, nullptr);
   }

   auto thread = OSGetCurrentThread();

   while (true) {
      if (thread->cancelState == OSThreadCancelState::Enabled &&
          thread->requestFlag != OSThreadRequest::None) {
         internal::lockScheduler();
         internal::testThreadCancelNoLock();
         internal::unlockScheduler();
         continue;
      }

      auto lockValue = mutex->lock.load();

      if (lockValue) {
         auto lockThread = virt_cast<OSThread *>(virt_addr { lockValue & ~1 });

         if (lockThread == thread) {
            // We already own this FastMutex, increase recursion count
            mutex->count++;
            break;
         } else {
            // Another thread owns this FastMutex, now we must take the slow path!
            fastMutexHardLock(mutex);
            break;
         }
      } else {
         // Attempt to lock the thread
         auto newValue = static_cast<uint32_t>(virt_cast<virt_addr>(thread));
         if (!mutex->lock.compare_exchange_weak(lockValue, newValue)) {
            continue;
         }

         // Set thread as owner
         thread->cancelState |= OSThreadCancelState::DisabledByFastMutex;
         FastMutexQueue::append(virt_addrof(thread->fastMutexQueue), mutex);
         mutex->count = 1;
         break;
      }
   }
}

static void
fastMutexHardUnlock(virt_ptr<OSFastMutex> mutex)
{
   decaf_check(mutex->tag == OSFastMutex::Tag);

   // Grab our current thread and make sure we are running
   auto thread = OSGetCurrentThread();
   decaf_check(thread->state == OSThreadState::Running);

   // Check to make sure the mutex is locked by us
   auto lockValue = mutex->lock.load();
   auto lockThread = virt_cast<OSThread *>(virt_addr { lockValue & ~1 });
   decaf_check(thread == lockThread);

   internal::lockScheduler();

   // Double check that the caller actually did everything properly...
   decaf_check(lockValue & 1);
   decaf_check(mutex->count == 0);

   if (mutex->isContended) {
      ContendedQueue::erase(virt_addrof(thread->contendedFastMutexes), mutex);
      mutex->isContended = FALSE;
   }

   // Adjust the priority if needed
   if (thread->priority > thread->basePriority) {
      thread->priority = internal::calculateThreadPriorityNoLock(thread);
   }

   // Free the cancel state if we arn't holding any more locks
   if (!thread->fastMutexQueue.head) {
      thread->cancelState &= ~OSThreadCancelState::DisabledByFastMutex;
   }

   // Wake up anyone who is hard-lock waiting on the mutex
   internal::wakeupThreadNoLock(virt_addrof(mutex->queue));

   // Release the lock!
   mutex->lock.store(0);

   internal::testThreadCancelNoLock();
   internal::rescheduleSelfNoLock();
   internal::unlockScheduler();
}


/**
 * Unlock a fast mutex object.
 */
void
OSFastMutex_Unlock(virt_ptr<OSFastMutex> mutex)
{
   decaf_check(mutex);
   decaf_check(mutex->tag == OSFastMutex::Tag);
   decaf_check(mutex->count > 0);

   auto thread = OSGetCurrentThread();
   auto lockValue = mutex->lock.load();
   auto lockThread = virt_cast<OSThread *>(virt_addr { lockValue & ~1 });
   decaf_check(lockThread == thread);

   // Reduce mutex count
   mutex->count--;

   if (mutex->count != 0) {
      // If count is not 0, then we have not unlocked!
      return;
   }

   // Remove ourselves from the queue
   FastMutexQueue::erase(virt_addrof(thread->fastMutexQueue), mutex);
   lockValue = mutex->lock.load();

   while (true) {
      // If someone contended on their lock, we need to hardUnlock
      if (lockValue & 1) {
         fastMutexHardUnlock(mutex);
         return;
      }

      // Try to clear the lock
      if (!mutex->lock.compare_exchange_weak(lockValue, 0)) {
         continue;
      }

      // Success!
      break;
   }

   // Clear the cancel state if we dont hold any more mutexes
   if (!thread->fastMutexQueue.head) {
      thread->cancelState &= ~OSThreadCancelState::DisabledByFastMutex;
   }

   // Lock the scheduler and consider cancelling
   if (thread->cancelState == OSThreadCancelState::Enabled) {
      internal::lockScheduler();
      internal::testThreadCancelNoLock();
      internal::unlockScheduler();
   }
}


/**
 * Try to lock a fast mutex object.
 *
 * \return
 * Returns FALSE if the mutex was already locked.
 * Returns TRUE if the mutex is now locked by the current thread.
 */
BOOL
OSFastMutex_TryLock(virt_ptr<OSFastMutex> mutex)
{
   auto thread = OSGetCurrentThread();

   while (true) {
      if (thread->cancelState == OSThreadCancelState::Enabled &&
          thread->requestFlag != OSThreadRequest::None) {
         internal::lockScheduler();
         internal::testThreadCancelNoLock();
         internal::unlockScheduler();
         continue;
      }

      auto lockValue = mutex->lock.load();

      if (lockValue) {
         auto lockThread = virt_cast<OSThread *>(virt_addr { lockValue & ~1 });

         if (lockThread == thread) {
            // We already own this FastMutex, increase recursion count
            mutex->count++;
            return TRUE;
         } else {
            // Another thread owns this FastMutex, we have failed!
            return FALSE;
         }
      } else {
         // Try to lock the FastMutex
         auto newValue = static_cast<uint32_t>(virt_cast<virt_addr>(thread));
         if (!mutex->lock.compare_exchange_weak(lockValue, newValue)) {
            continue;
         }

         // Set thread as owner
         thread->cancelState |= OSThreadCancelState::DisabledByFastMutex;
         FastMutexQueue::append(virt_addrof(thread->fastMutexQueue), mutex);
         mutex->count = 1;
         return TRUE;
      }
   }
}


/**
 * Initialises a fast condition object.
 */
void
OSFastCond_Init(virt_ptr<OSFastCondition> condition,
                virt_ptr<const char> name)
{
   decaf_check(condition);
   condition->tag = OSFastCondition::Tag;
   condition->name = name;
   condition->unk = 0u;
   OSInitThreadQueueEx(virt_addrof(condition->queue), condition);
}


/**
 * Sleep the current thread until the condition variable has been signalled.
 *
 * The mutex must be locked when entering this function.
 * Will unlock the mutex and then sleep, reacquiring the mutex when woken.
 */
void
OSFastCond_Wait(virt_ptr<OSFastCondition> condition,
                virt_ptr<OSFastMutex> mutex)
{
   decaf_check(condition);
   decaf_check(condition->tag == OSFastCondition::Tag);
   decaf_check(mutex);
   decaf_check(mutex->tag == OSFastMutex::Tag);
   internal::lockScheduler();

   auto thread = OSGetCurrentThread();
   auto lockValue = mutex->lock.load();
   auto lockThread = virt_cast<OSThread *>(virt_addr { lockValue & ~1 });
   decaf_check(lockValue & 1);
   decaf_check(lockThread == thread);

   if (mutex->isContended) {
      ContendedQueue::erase(virt_addrof(thread->contendedFastMutexes), mutex);
      mutex->isContended = FALSE;
   }

   if (thread->priority > thread->basePriority) {
      thread->priority = internal::calculateThreadPriorityNoLock(thread);
   }

   // Save the recursion count, then force an unlock of the mutex
   auto mutexCount = mutex->count;
   internal::disableScheduler();
   internal::wakeupThreadNoLock(virt_addrof(mutex->queue));
   mutex->count = 0;
   mutex->lock.store(0);
   internal::rescheduleAllCoreNoLock();
   internal::enableScheduler();

   // Sleep the current thread on the condition queue, wait to be signalled
   internal::sleepThreadNoLock(virt_addrof(condition->queue));
   internal::rescheduleSelfNoLock();

   // We must release the scheduler lock before trying to do a FastMutex lock
   internal::unlockScheduler();

   // Acquire the mutex, and restore the recursion count
   OSFastMutex_Lock(mutex);
   mutex->count = mutexCount;
}


/**
 * Will wake up any threads waiting on the condition with OSFastCond_Wait.
 */
void
OSFastCond_Signal(virt_ptr<OSFastCondition> condition)
{
   decaf_check(condition);
   decaf_check(condition->tag == OSFastCondition::Tag);
   OSWakeupThread(virt_addrof(condition->queue));
}

namespace internal
{

void
unlockAllFastMutexNoLock(virt_ptr<OSThread> thread)
{
   decaf_check(isSchedulerLocked());

   while (thread->fastMutexQueue.head) {
      auto mutex = thread->fastMutexQueue.head;

      // Ensure thread owns the mutex
      auto lockValue = mutex->lock.load();
      auto lockThread = virt_cast<OSThread *>(virt_addr { lockValue & ~1 });
      decaf_check(lockThread == thread);

      // Erase mutex from thread's mutex queue
      FastMutexQueue::erase(virt_addrof(thread->fastMutexQueue), mutex);

      // Erase mutex from thread's contended queue if necessary
      if (mutex->isContended) {
         ContendedQueue::erase(virt_addrof(thread->contendedFastMutexes), mutex);
         mutex->isContended = FALSE;
      }

      // Unlock the mutex
      wakeupThreadNoLock(virt_addrof(mutex->queue));
      mutex->count = 0;
      mutex->lock.store(0);
   }

   decaf_check(!thread->fastMutexQueue.head);
   decaf_check(!thread->fastMutexQueue.tail);
   decaf_check(!thread->contendedFastMutexes.head);
   decaf_check(!thread->contendedFastMutexes.tail);
}

} // namespace internal

void
Library::registerFastMutexSymbols()
{
   RegisterFunctionExport(OSFastMutex_Init);
   RegisterFunctionExport(OSFastMutex_Lock);
   RegisterFunctionExport(OSFastMutex_TryLock);
   RegisterFunctionExport(OSFastMutex_Unlock);
   RegisterFunctionExport(OSFastCond_Init);
   RegisterFunctionExport(OSFastCond_Wait);
   RegisterFunctionExport(OSFastCond_Signal);
}

} // namespace cafe::coreinit
