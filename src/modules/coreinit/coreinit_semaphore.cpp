#include "coreinit.h"
#include "coreinit_semaphore.h"
#include "system.h"

void
OSInitSemaphore(SemaphoreHandle handle, int32_t count)
{
   OSInitSemaphoreEx(handle, count, nullptr);
}

void
OSInitSemaphoreEx(SemaphoreHandle handle, int32_t count, char *name)
{
   auto semaphore = gSystem.addSystemObject<Semaphore>(handle);
   semaphore->name = name;
   semaphore->count = count;
}

int32_t
OSWaitSemaphore(SemaphoreHandle handle)
{
   auto semaphore = gSystem.getSystemObject<Semaphore>(handle);
   auto lock = std::unique_lock<std::mutex> { semaphore->mutex };
   auto previous = semaphore->count;

   while (semaphore->count <= 0) {
      semaphore->condition.wait(lock);
   }

   --semaphore->count;
   return previous;
}

int32_t
OSTryWaitSemaphore(SemaphoreHandle handle)
{
   auto semaphore = gSystem.getSystemObject<Semaphore>(handle);
   auto lock = std::unique_lock<std::mutex> { semaphore->mutex };
   auto previous = semaphore->count;

   if (semaphore->count > 0) {
      --semaphore->count;
   }

   return previous;
}

int32_t
OSSignalSemaphore(SemaphoreHandle handle)
{
   auto semaphore = gSystem.getSystemObject<Semaphore>(handle);
   auto lock = std::unique_lock<std::mutex> { semaphore->mutex };
   auto previous = semaphore->count;

   // Increase count, wake up waiting threads
   ++semaphore->count;
   semaphore->condition.notify_one();

   return previous;
}

int32_t
OSGetSemaphoreCount(SemaphoreHandle handle)
{
   auto semaphore = gSystem.getSystemObject<Semaphore>(handle);
   return semaphore->count;
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
