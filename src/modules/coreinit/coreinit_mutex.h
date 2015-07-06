#pragma once
#include <mutex>
#include <condition_variable>
#include "systemobject.h"
#include "coreinit_thread.h"

struct Fiber;

struct Mutex : public SystemObject
{
   static const uint32_t Tag = 0x6D557458;

   char *name;
   uint32_t count;
   std::mutex mutex;
   Fiber *owner;
   std::vector<Fiber *> queue;
};

struct Condition : public SystemObject
{
   static const uint32_t Tag = 0x634E6456;

   char *name;
   bool value;
   std::mutex mutex;
   std::vector<Fiber *> queue;
};

using MutexHandle = SystemObjectHeader *;
using ConditionHandle = SystemObjectHeader *;

MutexHandle
OSAllocMutex();

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

ConditionHandle
OSAllocCondition();

void
OSInitCond(ConditionHandle handle);

void
OSInitCondEx(ConditionHandle handle, char *name);

void
OSWaitCond(ConditionHandle conditionHandle, MutexHandle mutexHandle);

void
OSSignalCond(ConditionHandle handle);
