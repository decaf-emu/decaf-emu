#include "coreinit.h"
#include "coreinit_mutex.h"
#include "system.h"

// TODO: Ensure we can use custom OSMutex data instead of original

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
}

void
OSLockMutex(MutexHandle handle)
{
   auto mutex = gSystem.getSystemObject<Mutex>(handle);
   mutex->mutex.lock();
}

BOOL
OSTryLockMutex(MutexHandle handle)
{
   auto mutex = gSystem.getSystemObject<Mutex>(handle);
   return mutex->mutex.try_lock();
}

void
OSUnlockMutex(MutexHandle handle)
{
   auto mutex = gSystem.getSystemObject<Mutex>(handle);
   mutex->mutex.unlock();
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
}

void
OSWaitCond(ConditionHandle conditionHandle, MutexHandle mutexHandle)
{
   auto condition = gSystem.getSystemObject<Condition>(conditionHandle);
   auto mutex = gSystem.getSystemObject<Mutex>(mutexHandle);
   auto lock = std::unique_lock<std::recursive_mutex> { mutex->mutex };
   condition->condition.wait(lock);
}

void
OSSignalCond(ConditionHandle handle)
{
   auto condition = gSystem.getSystemObject<Condition>(handle);
   condition->condition.notify_all();
}

void
CoreInit::registerMutexFunctions()
{
   RegisterSystemFunction(OSInitMutex);
   RegisterSystemFunction(OSInitMutexEx);
   RegisterSystemFunction(OSLockMutex);
   RegisterSystemFunction(OSTryLockMutex);
   RegisterSystemFunction(OSUnlockMutex);
   RegisterSystemFunction(OSInitCond);
   RegisterSystemFunction(OSInitCondEx);
   RegisterSystemFunction(OSWaitCond);
   RegisterSystemFunction(OSSignalCond);
}
