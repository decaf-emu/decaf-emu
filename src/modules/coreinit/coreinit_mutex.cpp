#include "coreinit.h"
#include "coreinit_mutex.h"
#include "coreinit_scheduler.h"
#include "coreinit_thread.h"
#include "coreinit_queue.h"

namespace coreinit
{

const uint32_t
OSMutex::Tag;

const uint32_t
OSCondition::Tag;


/**
 * Initialise a mutex structure.
 */
void
OSInitMutex(OSMutex *mutex)
{
   OSInitMutexEx(mutex, nullptr);
}


/**
 * Initialise a mutex structure with a name.
 */
void
OSInitMutexEx(OSMutex *mutex, const char *name)
{
   mutex->tag = OSMutex::Tag;
   mutex->name = name;
   mutex->owner = nullptr;
   mutex->count = 0;
   OSInitThreadQueueEx(&mutex->queue, mutex);
   OSInitQueueLink(&mutex->link);
}


static void
lockMutexNoLock(OSMutex *mutex)
{
   assert(mutex && mutex->tag == OSMutex::Tag);
   auto thread = OSGetCurrentThread();

   while (mutex->owner && mutex->owner != thread) {
      thread->mutex = mutex;

      // Wait for other owner to unlock
      coreinit::internal::sleepThreadNoLock(&mutex->queue);
      coreinit::internal::rescheduleNoLock();

      thread->mutex = nullptr;
   }

   if (mutex->owner != thread) {
      // Add to mutex queue
      OSAppendQueue(&thread->mutexQueue, mutex);
   }

   mutex->owner = thread;
   mutex->count++;
}


/**
 * Lock the mutex.
 *
 * If no one owns the mutex, set current thread as owner.
 * If the lock is owned by the current thread, increase the recursion count.
 * If the lock is owned by another thread, the current thread will sleep until
 * the owner has unlocked this mutex.
 *
 * Similar to <a href="http://en.cppreference.com/w/cpp/thread/recursive_mutex/lock">std::recursive_mutex::lock</a>.
 */
void
OSLockMutex(OSMutex *mutex)
{
   coreinit::internal::lockScheduler();
   coreinit::internal::testThreadCancelNoLock();
   lockMutexNoLock(mutex);
   coreinit::internal::unlockScheduler();
}


/**
 * Try to lock a mutex.
 *
 * If no one owns the mutex, set current thread as owner.
 * If the lock is owned by the current thread, increase the recursion count.
 * If the lock is owned by another thread, do not block, return FALSE.
 *
 * \return TRUE if the mutex is locked, FALSE if the mutex is owned by another thread.
 *
 * Similar to <a href="http://en.cppreference.com/w/cpp/thread/recursive_mutex/try_lock">std::recursive_mutex::try_lock</a>.
 */
BOOL
OSTryLockMutex(OSMutex *mutex)
{
   auto thread = OSGetCurrentThread();
   coreinit::internal::lockScheduler();
   coreinit::internal::testThreadCancelNoLock();

   if (mutex->owner && mutex->owner != thread) {
      // Someone else owns this mutex
      coreinit::internal::unlockScheduler();
      return FALSE;
   }

   if (mutex->owner != thread) {
      // Add to mutex queue
      OSAppendQueue(&thread->mutexQueue, mutex);
   }

   mutex->owner = thread;
   mutex->count++;
   coreinit::internal::unlockScheduler();

   return TRUE;
}


static void
unlockMutexNoLock(OSMutex *mutex)
{
   auto thread = OSGetCurrentThread();
   assert(mutex && mutex->tag == OSMutex::Tag);
   assert(mutex->owner == thread);
   assert(mutex->count > 0);
   mutex->count--;

   if (mutex->count == 0) {
      mutex->owner = nullptr;

      // Remove mutex from thread's mutex queue
      OSEraseFromQueue(&thread->mutexQueue, mutex);

      // Wakeup any threads trying to lock this mutex
      coreinit::internal::wakeupThreadNoLock(&mutex->queue);
      coreinit::internal::rescheduleNoLock();
   }
}


/**
 * Unlocks the mutex.
 *
 * Will decrease the recursion count, will only unlock the mutex when the
 * recursion count reaches 0.
 * If any other threads are waiting to lock the mutex they will be woken.
 *
 * Similar to <a href="http://en.cppreference.com/w/cpp/thread/recursive_mutex/unlock">std::recursive_mutex::unlock</a>.
 */
void
OSUnlockMutex(OSMutex *mutex)
{
   coreinit::internal::lockScheduler();
   unlockMutexNoLock(mutex);
   coreinit::internal::testThreadCancelNoLock();
   coreinit::internal::unlockScheduler();
}


/**
 * Initialise a condition variable structure.
 */
void
OSInitCond(OSCondition *condition)
{
   OSInitCondEx(condition, nullptr);
}


/**
 * Initialise a condition variable structure with a name.
 */
void
OSInitCondEx(OSCondition *condition, const char *name)
{
   condition->tag = OSCondition::Tag;
   condition->name = name;
   OSInitThreadQueueEx(&condition->queue, condition);
}


/**
 * Sleep the current thread until the condition variable has been signalled.
 *
 * The mutex must be locked when entering this function.
 * Will unlock the mutex and then sleep, reacquiring the mutex when woken.
 *
 * Similar to <a href="http://en.cppreference.com/w/cpp/thread/condition_variable/wait">std::condition_variable::wait</a>.
 */
void
OSWaitCond(OSCondition *condition, OSMutex *mutex)
{
   auto thread = OSGetCurrentThread();
   coreinit::internal::lockScheduler();
   assert(mutex && mutex->tag == OSMutex::Tag);
   assert(condition && condition->tag == OSCondition::Tag);
   assert(mutex->owner == thread);

   // Force an unlock
   auto mutexCount = mutex->count;
   mutex->count = 1;
   unlockMutexNoLock(mutex);

   // Sleep on the condition
   coreinit::internal::sleepThreadNoLock(&condition->queue);
   coreinit::internal::rescheduleNoLock();

   // Restore lock
   lockMutexNoLock(mutex);
   mutex->count = mutexCount;

   coreinit::internal::unlockScheduler();
}


/**
 * Will wake up any threads waiting on the condition with OSWaitCond.
 *
 * Similar to <a href="http://en.cppreference.com/w/cpp/thread/condition_variable/notify_all">std::condition_variable::notify_all</a>.
 */
void
OSSignalCond(OSCondition *condition)
{
   assert(condition && condition->tag == OSCondition::Tag);
   OSWakeupThread(&condition->queue);
}


void
Module::registerMutexFunctions()
{
   RegisterKernelFunction(OSInitMutex);
   RegisterKernelFunction(OSInitMutexEx);
   RegisterKernelFunction(OSLockMutex);
   RegisterKernelFunction(OSTryLockMutex);
   RegisterKernelFunction(OSUnlockMutex);
   RegisterKernelFunction(OSInitCond);
   RegisterKernelFunction(OSInitCondEx);
   RegisterKernelFunction(OSWaitCond);
   RegisterKernelFunction(OSSignalCond);
}

} // namespace coreinit
