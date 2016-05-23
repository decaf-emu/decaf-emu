#include "coreinit.h"
#include "coreinit_semaphore.h"
#include "coreinit_scheduler.h"

namespace coreinit
{

const uint32_t
OSSemaphore::Tag;

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
   coreinit::internal::lockScheduler();
   assert(semaphore && semaphore->tag == OSSemaphore::Tag);

   while (semaphore->count <= 0) {
      // Wait until we can decrease semaphore
      coreinit::internal::sleepThreadNoLock(&semaphore->queue);
      coreinit::internal::rescheduleNoLock();
   }

   previous = semaphore->count--;
   coreinit::internal::unlockScheduler();
   return previous;
}

int32_t
OSTryWaitSemaphore(OSSemaphore *semaphore)
{
   int32_t previous;
   coreinit::internal::lockScheduler();
   assert(semaphore && semaphore->tag == OSSemaphore::Tag);

   // Try to decrease semaphore
   previous = semaphore->count;

   if (semaphore->count > 0) {
      semaphore->count--;
   }

   coreinit::internal::unlockScheduler();
   return previous;
}

int32_t
OSSignalSemaphore(OSSemaphore *semaphore)
{
   int32_t previous;
   coreinit::internal::lockScheduler();
   assert(semaphore && semaphore->tag == OSSemaphore::Tag);

   // Increase semaphore
   previous =  semaphore->count++;

   // Wakeup any waiting threads
   coreinit::internal::wakeupThreadNoLock(&semaphore->queue);
   coreinit::internal::rescheduleNoLock();

   coreinit::internal::unlockScheduler();
   return previous;
}

int32_t
OSGetSemaphoreCount(OSSemaphore *semaphore)
{
   int32_t count;
   coreinit::internal::lockScheduler();
   assert(semaphore && semaphore->tag == OSSemaphore::Tag);

   // Return count
   count = semaphore->count;

   coreinit::internal::unlockScheduler();
   return count;
}

void
Module::registerSemaphoreFunctions()
{
   RegisterKernelFunction(OSInitSemaphore);
   RegisterKernelFunction(OSInitSemaphoreEx);
   RegisterKernelFunction(OSWaitSemaphore);
   RegisterKernelFunction(OSTryWaitSemaphore);
   RegisterKernelFunction(OSSignalSemaphore);
   RegisterKernelFunction(OSGetSemaphoreCount);
}

} // namespace coreinit
