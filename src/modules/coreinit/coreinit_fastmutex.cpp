#include "coreinit.h"
#include "coreinit_fastmutex.h"
#include "coreinit_mutex.h"

// TODO: Implement fast mutex

void
OSFastMutex_Init(OSFastMutex *mutex, const char *name)
{
   OSInitMutexEx(reinterpret_cast<OSMutex*>(mutex), name);
}

void
OSFastMutex_Lock(OSFastMutex *mutex)
{
   OSLockMutex(reinterpret_cast<OSMutex*>(mutex));
}

void
OSFastMutex_Unlock(OSFastMutex *mutex)
{
   OSUnlockMutex(reinterpret_cast<OSMutex*>(mutex));
}

BOOL
OSFastMutex_TryLock(OSFastMutex *mutex)
{
   return OSTryLockMutex(reinterpret_cast<OSMutex*>(mutex));
}

void
OSFastCond_Init(OSFastCondition *condition, const char *name)
{
   OSInitCondEx(reinterpret_cast<OSCondition*>(condition), name);
}

void
OSFastCond_Wait(OSFastCondition *condition, OSFastMutex *mutex)
{
   OSWaitCond(reinterpret_cast<OSCondition*>(condition), reinterpret_cast<OSMutex*>(mutex));
}

void
OSFastCond_Signal(OSFastCondition *condition)
{
   OSSignalCond(reinterpret_cast<OSCondition*>(condition));
}

void
CoreInit::registerFastMutexFunctions()
{
   RegisterKernelFunction(OSFastMutex_Init);
   RegisterKernelFunction(OSFastMutex_Lock);
   RegisterKernelFunction(OSFastMutex_TryLock);
   RegisterKernelFunction(OSFastMutex_Unlock);
   RegisterKernelFunction(OSFastCond_Init);
   RegisterKernelFunction(OSFastCond_Wait);
   RegisterKernelFunction(OSFastCond_Signal);
}
