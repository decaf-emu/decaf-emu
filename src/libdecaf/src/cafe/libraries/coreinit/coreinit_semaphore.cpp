#include "coreinit.h"
#include "coreinit_semaphore.h"
#include "coreinit_scheduler.h"
#include <common/decaf_assert.h>

namespace cafe::coreinit
{

constexpr uint32_t
OSSemaphore::Tag;


/**
 * Initialise semaphore object with count.
 */
void
OSInitSemaphore(virt_ptr<OSSemaphore> semaphore,
                int32_t count)
{
   OSInitSemaphoreEx(semaphore, count, nullptr);
}


/**
 * Initialise semaphore object with count and name.
 */
void
OSInitSemaphoreEx(virt_ptr<OSSemaphore> semaphore,
                  int32_t count,
                  virt_ptr<const char> name)
{
   decaf_check(semaphore);
   semaphore->tag = OSSemaphore::Tag;
   semaphore->name = name;
   semaphore->count = count;
   OSInitThreadQueueEx(virt_addrof(semaphore->queue), semaphore);
}


/**
 * Decrease the semaphore value.
 *
 * If the value is less than or equal to zero the current thread will be put to
 * sleep until the count is above zero and it can decrement it safely.
 */
int32_t
OSWaitSemaphore(virt_ptr<OSSemaphore> semaphore)
{
   internal::lockScheduler();
   decaf_check(semaphore);
   decaf_check(semaphore->tag == OSSemaphore::Tag);

   // Wait until we can decrease semaphore
   while (semaphore->count <= 0) {
      internal::sleepThreadNoLock(virt_addrof(semaphore->queue));
      internal::rescheduleSelfNoLock();
   }

   auto previous = semaphore->count;

   // Decrease semaphore
   semaphore->count--;

   internal::unlockScheduler();
   return previous;
}


/**
 * Try to decrease the semaphore value.
 *
 * If the value is greater than zero then it will be decremented, else the function
 * will return immediately with a value <= 0 indicating a failure.
 *
 * \return Returns previous semaphore count, before the decrement in this function.
 *         If the value is >0 then it means the call was succesful.
 */
int32_t
OSTryWaitSemaphore(virt_ptr<OSSemaphore> semaphore)
{
   internal::lockScheduler();
   decaf_check(semaphore);
   decaf_check(semaphore->tag == OSSemaphore::Tag);

   auto previous = semaphore->count;

   // Try to decrease semaphore
   if (semaphore->count > 0) {
      semaphore->count--;
   }

   internal::unlockScheduler();
   return previous;
}


/**
 * Increase the semaphore value.
 *
 * If any threads are waiting for semaphore, they are woken.
 */
int32_t
OSSignalSemaphore(virt_ptr<OSSemaphore> semaphore)
{
   internal::lockScheduler();
   decaf_check(semaphore);
   decaf_check(semaphore->tag == OSSemaphore::Tag);

   auto previous = semaphore->count;

   // Increase semaphore
   semaphore->count++;

   // Wakeup any waiting threads
   internal::wakeupThreadNoLock(virt_addrof(semaphore->queue));
   internal::rescheduleAllCoreNoLock();

   internal::unlockScheduler();
   return previous;
}


/**
 * Get the current semaphore count.
 */
int32_t
OSGetSemaphoreCount(virt_ptr<OSSemaphore> semaphore)
{
   internal::lockScheduler();
   decaf_check(semaphore);
   decaf_check(semaphore->tag == OSSemaphore::Tag);

   // Return count
   auto count = semaphore->count;

   internal::unlockScheduler();
   return count;
}


void
Library::registerSemaphoreSymbols()
{
   RegisterFunctionExport(OSInitSemaphore);
   RegisterFunctionExport(OSInitSemaphoreEx);
   RegisterFunctionExport(OSWaitSemaphore);
   RegisterFunctionExport(OSTryWaitSemaphore);
   RegisterFunctionExport(OSSignalSemaphore);
   RegisterFunctionExport(OSGetSemaphoreCount);
}

} // namespace cafe::coreinit
