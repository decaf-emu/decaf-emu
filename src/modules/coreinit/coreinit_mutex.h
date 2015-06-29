#pragma once
#include <mutex>
#include <condition_variable>
#include "systemobject.h"
#include "coreinit_thread.h"

struct Mutex : public SystemObject
{
   static const uint32_t Tag = 0x6D557458;

   char *name;
   std::recursive_mutex mutex;
};

struct Condition : public SystemObject
{
   static const uint32_t Tag = 0x634E6456;

   char *name;
   std::condition_variable_any condition;
};

using MutexHandle = p32<SystemObjectHeader>;
using ConditionHandle = p32<SystemObjectHeader>;

void
OSInitMutex(MutexHandle handle);

void
OSInitMutexEx(MutexHandle handle, char *name);

void
OSLockMutex(MutexHandle handle);

void
OSUnlockMutex(MutexHandle handle);

BOOL
OSTryLockMutex(MutexHandle handle);

void
OSInitCond(ConditionHandle handle);

void
OSInitCondEx(ConditionHandle handle, char *nname);

void
OSWaitCond(ConditionHandle conditionHandle, MutexHandle mutexHandle);

void
OSSignalCond(ConditionHandle handle);
