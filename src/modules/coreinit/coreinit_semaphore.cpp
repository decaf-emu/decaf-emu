#include "coreinit.h"
#include "coreinit_semaphore.h"
#include "processor.h"
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
   auto fiber = gProcessor.getCurrentFiber();

   while (true) {
      std::unique_lock<std::mutex> lock { semaphore->mutex };

      if (semaphore->count > 0) {
         break;
      }

      semaphore->queue.push_back(fiber);
      gProcessor.wait(lock);
   }

   return semaphore->count--;
}

int32_t
OSTryWaitSemaphore(SemaphoreHandle handle)
{
   auto semaphore = gSystem.getSystemObject<Semaphore>(handle);
   auto fiber = gProcessor.getCurrentFiber();
   std::unique_lock<std::mutex> lock { semaphore->mutex };

   if (semaphore->count <= 0) {
      return semaphore->count;
   }

   return semaphore->count--;
}

int32_t
OSSignalSemaphore(SemaphoreHandle handle)
{
   auto semaphore = gSystem.getSystemObject<Semaphore>(handle);
   auto fiber = gProcessor.getCurrentFiber();
   std::unique_lock<std::mutex> lock { semaphore->mutex };
   auto count = semaphore->count++;

   // Wakeup fibers
   for (auto fiber : semaphore->queue) {
      gProcessor.queue(fiber);
   }

   semaphore->queue.clear();
   return count;
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
