#include "coreinit.h"
#include "coreinit_mutex.h"
#include "coreinit_memheap.h"
#include "system.h"
#include "processor.h"

MutexHandle
OSAllocMutex()
{
   return OSAllocFromSystem<SystemObjectHeader>();
}

ConditionHandle
OSAllocCondition()
{
   return OSAllocFromSystem<SystemObjectHeader>();
}

void
OSInitMutex(MutexHandle handle)
{
   OSInitMutexEx(handle, nullptr);
}

void
OSInitMutexEx(MutexHandle handle, char *name)
{
   auto mutex = gSystem.addSystemObject<Mutex>(handle);
   mutex->name = name;
   mutex->count = 0;
   mutex->owner = nullptr;
}

void
OSLockMutex(MutexHandle handle)
{
   auto mutex = gSystem.getSystemObject<Mutex>(handle);
   auto fiber = gProcessor.getCurrentFiber();
   
   while (true) {
      OSTestThreadCancel();
      std::unique_lock<std::mutex> lock { mutex->mutex };

      if (!mutex->owner || mutex->owner == fiber) {
         break;
      }

      mutex->queue.push_back(fiber);
      gProcessor.wait(lock);
   }

   mutex->count++;
   mutex->owner = fiber;
}

BOOL
OSTryLockMutex(MutexHandle handle)
{
   auto mutex = gSystem.getSystemObject<Mutex>(handle);
   auto fiber = gProcessor.getCurrentFiber();

   std::unique_lock<std::mutex> lock { mutex->mutex };

   if (mutex->owner == fiber) {
      mutex->count++;
      return TRUE;
   } else if (!mutex->owner) {
      mutex->owner = fiber;
      mutex->count = 1;
      return TRUE;
   }

   return FALSE;
}

void
OSUnlockMutex(MutexHandle handle)
{
   auto mutex = gSystem.getSystemObject<Mutex>(handle);
   auto fiber = gProcessor.getCurrentFiber();

   std::unique_lock<std::mutex> lock { mutex->mutex };
   assert(mutex->owner == fiber);
   assert(mutex->count > 0);
   
   mutex->count--;

   if (!mutex->count) {
      // Fully unlocked, wake up any waiting fibers
      mutex->owner = nullptr;

      for (auto fiber : mutex->queue) {
         gProcessor.queue(fiber);
      }

      mutex->queue.clear();
   }

   lock.unlock();
   OSTestThreadCancel();
}

void
OSInitCond(ConditionHandle handle)
{
   OSInitCondEx(handle, nullptr);
}

void
OSInitCondEx(ConditionHandle handle, char *name)
{
   auto condition = gSystem.addSystemObject<Condition>(handle);
   condition->name = name;
   condition->value = false;
}

void
OSWaitCond(ConditionHandle conditionHandle, MutexHandle mutexHandle)
{
   auto condition = gSystem.getSystemObject<Condition>(conditionHandle);
   auto fiber = gProcessor.getCurrentFiber();
   
   // Unlock mutex
   auto mutex = gSystem.getSystemObject<Mutex>(mutexHandle);
   auto count = mutex->count;
   mutex->count = 1;
   OSUnlockMutex(mutexHandle);

   // Try acquire condition
   while (true) {
      std::unique_lock<std::mutex> lock { condition->mutex };

      if (condition->value) {
         break;
      }

      condition->queue.push_back(fiber);
      gProcessor.wait(lock);
   }

   condition->value = false;

   // Lock mutex
   OSLockMutex(mutexHandle);
   mutex->count = count;
}

void
OSSignalCond(ConditionHandle handle)
{
   auto condition = gSystem.getSystemObject<Condition>(handle);
   std::unique_lock<std::mutex> lock { condition->mutex };

   // Set condition
   condition->value = true;

   // Wake all waiting fibers
   for (auto fiber : condition->queue) {
      gProcessor.queue(fiber);
   }

   condition->queue.clear();
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
