#include "coreinit.h"
#include "coreinit_fastmutex.h"
#include "coreinit_scheduler.h"

namespace coreinit
{

using FastMutexQueue = internal::Queue<OSFastMutexQueue, OSFastMutexLink, OSFastMutex, &OSFastMutex::link>;
using ContendedQueue = internal::Queue<OSFastMutexQueue, OSFastMutexLink, OSFastMutex, &OSFastMutex::contendedLink>;
using ThreadSimpleQueue = internal::SortedQueue<OSThreadSimpleQueue, OSThreadLink, OSThread, &OSThread::link, internal::threadSortFunc>;

const uint32_t
OSFastMutex::Tag;

const uint32_t
OSFastCondition::Tag;

void
OSFastMutex_Init(OSFastMutex *mutex, const char *name)
{
   mutex->tag = OSFastMutex::Tag;
   mutex->name = name;
   mutex->isContended = 0;
   mutex->lock = 0;
   mutex->count = 0;
   ThreadSimpleQueue::init(&mutex->queue);
   FastMutexQueue::initLink(mutex);
   ContendedQueue::initLink(mutex);
}

static void
fastMutexHardLock(OSFastMutex *mutex)
{
   decaf_check(mutex->tag == OSFastMutex::Tag);

   auto thread = OSGetCurrentThread();
   decaf_check(thread->state == OSThreadState::Running);

   internal::lockScheduler();
   internal::testThreadCancelNoLock();

   auto lockValue = mutex->lock.load();
   while (true) {
      if (lockValue != 0) {
         if (!(lockValue & 1)) {
            if (!mutex->lock.compare_exchange_weak(lockValue, lockValue | 1)) {
               continue;
            }
         }

         // We now have set the waiter bit and we were not the owner

         auto ownerThread = mem::translate<OSThread>(lockValue & ~1);

         if (!mutex->isContended) {
            ContendedQueue::append(&thread->contendedFastMutexes, mutex);
            mutex->isContended = 1;
         }

         // Record which fast mutex we are trying to lock before we sleep
         thread->fastMutex = mutex;

         // Promote the priority of the owning thread to prevent priority inversion problems
         internal::promoteThreadPriorityNoLock(ownerThread, thread->priority);

         // Sleep on the queue waiting for a hard unlock
         internal::sleepThreadNoLock(&mutex->queue);
         internal::rescheduleSelfNoLock();

         // We are no longer attempting to lock this fast mutex
         thread->fastMutex = nullptr;

         lockValue = mutex->lock.load();
         if (lockValue != 0) {
            continue;
         }
      }

      if (!mutex->lock.compare_exchange_weak(lockValue, mem::untranslate(thread))) {
         continue;
      }

      decaf_check(!(lockValue & 1));
      decaf_check(mutex->count == 0);

      mutex->count = 1;

      thread->cancelState |= 0x80000;

      break;
   }

   internal::unlockScheduler();
}

void
OSFastMutex_Lock(OSFastMutex *mutex)
{
   // Grab the current thread
   auto thread = OSGetCurrentThread();

   while (true) {
      if (thread->cancelState == 0 && thread->requestFlag != 0) {
         internal::lockScheduler();
         internal::testThreadCancelNoLock();
         internal::unlockScheduler();
         continue;
      }

      auto lockValue = mutex->lock.load();
      if (lockValue) {
         auto lockThread = mem::translate<OSThread>(lockValue & ~1);
         if (lockThread == thread) {
            mutex->count++;
            return;
         } else {
            fastMutexHardLock(mutex);
            return;
         }
      } else {
         if (!mutex->lock.compare_exchange_weak(lockValue, mem::untranslate(thread))) {
            continue;
         }

         thread->cancelState |= 0x80000;

         FastMutexQueue::append(&thread->fastMutexQueue, mutex);

         mutex->count = 1;
         return;
      }
   }
}

static void
fastMutexHardUnlock(OSFastMutex *mutex)
{
   decaf_check(mutex->tag == OSFastMutex::Tag);

   // Grab our current thread and make sure we are running
   auto thread = OSGetCurrentThread();
   decaf_check(thread->state == OSThreadState::Running);

   // Check to make sure the mutex is locked by us
   auto lockValue = mutex->lock.load();
   auto lockThread = mem::translate<OSThread>(lockValue & ~1);
   decaf_check(thread == lockThread);

   internal::lockScheduler();

   // Double check that the caller actually did everything properly...
   decaf_check(lockValue & 1);
   decaf_check(mutex->count == 0);

   if (!mutex->isContended) {
      ContendedQueue::erase(&thread->contendedFastMutexes, mutex);
      mutex->isContended = 0;
   }

   // Adjust the priority if needed
   if (thread->priority > thread->basePriority) {
      thread->priority = internal::calculateThreadPriorityNoLock(thread);
   }

   // Free the cancel state if we arn't holding any more locks
   if (!thread->fastMutexQueue.head) {
      thread->cancelState &= ~0x80000;
   }

   // Wake up anyone who is hard-lock waiting on the mutex
   internal::wakeupThreadNoLock(&mutex->queue);

   // Release the lock!
   mutex->lock.store(0);

   internal::testThreadCancelNoLock();
   internal::rescheduleSelfNoLock();

   internal::unlockScheduler();
}

void
OSFastMutex_Unlock(OSFastMutex *mutex)
{
   auto thread = OSGetCurrentThread();

   auto lockValue = mutex->lock.load();
   mutex->count--;

   auto lockThread = mem::translate<OSThread>(lockValue & ~1);
   decaf_check(lockThread == thread);

   if (mutex->count != 0) {
      return;
   }

   // Remove ourselves from the queue
   FastMutexQueue::erase(&thread->fastMutexQueue, mutex);

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
      thread->cancelState &= ~0x80000;
   }

   // Lock the scheduler and consider cancelling
   if (!thread->cancelState) {
      internal::lockScheduler();
      internal::testThreadCancelNoLock();
      internal::unlockScheduler();
   }
}

BOOL
OSFastMutex_TryLock(OSFastMutex *mutex)
{
   auto thread = OSGetCurrentThread();

   while (true) {
      if (thread->cancelState == 0 && thread->requestFlag != 0) {
         internal::lockScheduler();
         internal::testThreadCancelNoLock();
         internal::unlockScheduler();
         continue;
      }

      auto lockValue = mutex->lock.load();
      if (lockValue) {
         auto lockThread = mem::translate<OSThread>(lockValue & ~1);
         if (lockThread == thread) {
            mutex->count++;
            return TRUE;
         } else {
            return FALSE;
         }
      } else {
         if (!mutex->lock.compare_exchange_weak(lockValue, mem::untranslate(thread))) {
            continue;
         }

         thread->cancelState |= 0x80000;

         FastMutexQueue::append(&thread->fastMutexQueue, mutex);

         mutex->count = 1;
         return TRUE;
      }
   }
}

void
OSFastCond_Init(OSFastCondition *condition, const char *name)
{
   condition->tag = OSFastCondition::Tag;
   condition->name = name;
   condition->unk = 0;
   OSInitThreadQueueEx(&condition->queue, condition);
}

void
OSFastCond_Wait(OSFastCondition *condition, OSFastMutex *mutex)
{
   decaf_check(condition->tag == OSFastCondition::Tag);

   internal::lockScheduler();

   auto thread = OSGetCurrentThread();

   auto lockValue = mutex->lock.load();
   auto lockThread = mem::translate<OSThread>(lockValue & ~1);
   decaf_check(lockThread == thread);

   if (!mutex->isContended) {
      ContendedQueue::erase(&thread->contendedFastMutexes, mutex);
      mutex->isContended = 0;
   }

   if (thread->priority > thread->basePriority) {
      thread->priority = internal::calculateThreadPriorityNoLock(thread);
   }

   auto mutexCount = mutex->count;

   internal::disableScheduler();
   internal::wakeupThreadNoLock(&mutex->queue);
   mutex->count = 0;
   mutex->lock.store(0);
   internal::rescheduleAllCoreNoLock();
   internal::enableScheduler();

   internal::sleepThreadNoLock(&condition->queue);
   internal::rescheduleSelfNoLock();

   internal::unlockScheduler();

   OSFastMutex_Lock(mutex);
   mutex->count = mutexCount;
}

void
OSFastCond_Signal(OSFastCondition *condition)
{
   decaf_check(condition->tag == OSFastCondition::Tag);

   OSWakeupThread(&condition->queue);
}

void
Module::registerFastMutexFunctions()
{
   RegisterKernelFunction(OSFastMutex_Init);
   RegisterKernelFunction(OSFastMutex_Lock);
   RegisterKernelFunction(OSFastMutex_TryLock);
   RegisterKernelFunction(OSFastMutex_Unlock);
   RegisterKernelFunction(OSFastCond_Init);
   RegisterKernelFunction(OSFastCond_Wait);
   RegisterKernelFunction(OSFastCond_Signal);
}

namespace internal
{

void
unlockAllFastMutexNoLock(OSThread *thread)
{
   // This function is meant to forcibly unlock all of the fast
   //  mutexes that are currently owned by this thread.  I'm
   //  sick and tired of reversing atomics though, so I'll just
   //  assert that they don't hold any mutexes...
   decaf_check(!thread->fastMutexQueue.head);
}

} // namespace internal

} // namespace coreinit
