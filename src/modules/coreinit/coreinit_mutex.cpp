#include "coreinit.h"
#include "coreinit_mutex.h"
#include "coreinit_scheduler.h"
#include "coreinit_thread.h"
#include "coreinit_queue.h"

void
OSInitMutex(OSMutex *mutex)
{
   OSInitMutexEx(mutex, nullptr);
}

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

void
OSLockMutex(OSMutex *mutex)
{
   OSLockScheduler();
   OSTestThreadCancelNoLock();
   OSLockMutexNoLock(mutex);
   OSUnlockScheduler();
}

void
OSLockMutexNoLock(OSMutex *mutex)
{
   assert(mutex && mutex->tag == OSMutex::Tag);
   auto thread = OSGetCurrentThread();

   if (mutex->owner && mutex->owner != thread) {
      thread->mutex = mutex;

      // Wait for other owner to unlock
      OSSleepThreadNoLock(&mutex->queue);
      OSRescheduleNoLock();

      thread->mutex = nullptr;
   }

   if (mutex->owner != thread) {
      // Add to mutex queue
      OSAppendQueue(&thread->mutexQueue, mutex);
   }

   mutex->owner = thread;
   mutex->count++;
}

BOOL
OSTryLockMutex(OSMutex *mutex)
{
   auto thread = OSGetCurrentThread();
   OSLockScheduler();
   OSTestThreadCancelNoLock();

   if (mutex->owner && mutex->owner != thread) {
      // Someone else owns this mutex
      OSUnlockScheduler();
      return FALSE;
   }

   if (mutex->owner != thread) {
      // Add to mutex queue
      OSAppendQueue(&thread->mutexQueue, mutex);
   }

   mutex->owner = thread;
   mutex->count++;
   OSUnlockScheduler();

   return TRUE;
}

void
OSUnlockMutex(OSMutex *mutex)
{
   OSLockScheduler();
   OSUnlockMutexNoLock(mutex);
   OSTestThreadCancelNoLock();
   OSUnlockScheduler();
}

void
OSUnlockMutexNoLock(OSMutex *mutex)
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
      OSWakeupThreadNoLock(&mutex->queue);
      OSRescheduleNoLock();
   }
}

void
OSInitCond(OSCondition *condition)
{
   OSInitCondEx(condition, nullptr);
}

void
OSInitCondEx(OSCondition *condition, const char *name)
{
   condition->tag = OSCondition::Tag;
   condition->name = name;
   OSInitThreadQueueEx(&condition->queue, condition);
}

void
OSWaitCond(OSCondition *condition, OSMutex *mutex)
{
   auto thread = OSGetCurrentThread();
   OSLockScheduler();
   assert(mutex && mutex->tag == OSMutex::Tag);
   assert(condition && condition->tag == OSCondition::Tag);
   assert(mutex->owner == thread);

   // Force an unlock
   auto mutexCount = mutex->count;
   mutex->count = 1;
   OSUnlockMutexNoLock(mutex);

   // Sleep on the condition
   OSSleepThreadNoLock(&condition->queue);
   OSRescheduleNoLock();

   // Restore lock
   OSLockMutexNoLock(mutex);
   mutex->count = mutexCount;

   OSUnlockScheduler();
}

void
OSSignalCond(OSCondition *condition)
{
   assert(condition && condition->tag == OSCondition::Tag);
   OSWakeupThread(&condition->queue);
}

void
CoreInit::registerMutexFunctions()
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
