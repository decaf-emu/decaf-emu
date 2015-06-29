#include "coreinit.h"
#include "coreinit_fastmutex.h"
#include "coreinit_mutex.h"

// TODO: Fast Mutex is platform specific, lets just use normal mutex for now.
// Linux: futex, Windows8+ WaitOnAddress

void
OSFastMutex_Init(FastMutexHandle handle, char *name)
{
   OSInitMutexEx(handle, name);
}

void
OSFastMutex_Lock(FastMutexHandle handle)
{
   OSLockMutex(handle);
}

void
OSFastMutex_Unlock(FastMutexHandle handle)
{
   OSUnlockMutex(handle);
}

BOOL
OSFastMutex_TryLock(FastMutexHandle handle)
{
   return OSTryLockMutex(handle);
}

void
OSFastCond_Init(FastConditionHandle handle, char *name)
{
   OSInitCondEx(handle, name);
}

void
OSFastCond_Wait(FastConditionHandle conditionHandle, FastMutexHandle mutexHandle)
{
   OSWaitCond(conditionHandle, mutexHandle);
}

void
OSFastCond_Signal(FastConditionHandle handle)
{
   OSSignalCond(handle);
}

void
CoreInit::registerFastMutexFunctions()
{
   RegisterSystemFunction(OSFastMutex_Init);
   RegisterSystemFunction(OSFastMutex_Lock);
   RegisterSystemFunction(OSFastMutex_TryLock);
   RegisterSystemFunction(OSFastMutex_Unlock);
   RegisterSystemFunction(OSFastCond_Init);
   RegisterSystemFunction(OSFastCond_Wait);
   RegisterSystemFunction(OSFastCond_Signal);
}
