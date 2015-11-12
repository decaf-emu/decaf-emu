#include "coreinit.h"
#include "coreinit_semaphore.h"
#include "coreinit_scheduler.h"

const uint32_t OSSemaphore::Tag;

void
OSInitSemaphore(OSSemaphore *semaphore, int32_t count)
{
   OSInitSemaphoreEx(semaphore, count, nullptr);
}

void
OSInitSemaphoreEx(OSSemaphore *semaphore, int32_t count, char *name)
{
   semaphore->tag = OSSemaphore::Tag;
   semaphore->name = name;
   semaphore->count = count;
   OSInitThreadQueueEx(&semaphore->queue, semaphore);
}

int32_t
OSWaitSemaphore(OSSemaphore *semaphore)
{
   int32_t previous;
   OSLockScheduler();
   assert(semaphore && semaphore->tag == OSSemaphore::Tag);

   while (semaphore->count <= 0) {
      // Wait until we can decrease semaphore
      OSSleepThreadNoLock(&semaphore->queue);
      OSRescheduleNoLock();
   }

   previous = semaphore->count--;
   OSUnlockScheduler();
   return previous;
}

int32_t
OSTryWaitSemaphore(OSSemaphore *semaphore)
{
   int32_t previous;
   OSLockScheduler();
   assert(semaphore && semaphore->tag == OSSemaphore::Tag);

   // Try to decrease semaphore
   previous = semaphore->count;

   if (semaphore->count > 0) {
      semaphore->count--;
   }

   OSUnlockScheduler();
   return previous;
}

int32_t
OSSignalSemaphore(OSSemaphore *semaphore)
{
   int32_t previous;
   OSLockScheduler();
   assert(semaphore && semaphore->tag == OSSemaphore::Tag);

   // Increase semaphore
   previous =  semaphore->count++;

   // Wakeup any waiting threads
   OSWakeupThreadNoLock(&semaphore->queue);
   OSRescheduleNoLock();

   OSUnlockScheduler();
   return previous;
}

int32_t
OSGetSemaphoreCount(OSSemaphore *semaphore)
{
   int32_t count;
   OSLockScheduler();
   assert(semaphore && semaphore->tag == OSSemaphore::Tag);

   // Return count
   count = semaphore->count;

   OSUnlockScheduler();
   return count;
}

void
CoreInit::registerSemaphoreFunctions()
{
   RegisterKernelFunction(OSInitSemaphore);
   RegisterKernelFunction(OSInitSemaphoreEx);
   RegisterKernelFunction(OSWaitSemaphore);
   RegisterKernelFunction(OSTryWaitSemaphore);
   RegisterKernelFunction(OSSignalSemaphore);
   RegisterKernelFunction(OSGetSemaphoreCount);
}
