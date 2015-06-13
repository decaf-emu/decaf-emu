#pragma once
#include "systemobject.h"
#include "systemtypes.h"

using FastMutexHandle = p32<SystemObjectHeader>;
using FastConditionHandle = p32<SystemObjectHeader>;

void
OSFastMutex_Init(FastMutexHandle handle, char *name);

void
OSFastMutex_Lock(FastMutexHandle handle);

void
OSFastMutex_Unlock(FastMutexHandle handle);

BOOL
OSFastMutex_TryLock(FastMutexHandle handle);

void
OSFastCond_Init(FastConditionHandle handle, char *name);

void
OSFastCond_Wait(FastConditionHandle conditionHandle, FastMutexHandle mutexHandle);

void
OSFastCond_Signal(FastConditionHandle handle);
