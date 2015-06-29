#pragma once
#include <mutex>
#include <condition_variable>
#include "systemobject.h"
#include "systemtypes.h"

struct Semaphore : public SystemObject
{
   static const uint32_t Tag = 0x73506852;

   char *name;
   int32_t count;
   std::mutex mutex;
   std::condition_variable condition;
};

using SemaphoreHandle = p32<SystemObjectHeader>;

void
OSInitSemaphore(SemaphoreHandle handle, int32_t count);

void
OSInitSemaphoreEx(SemaphoreHandle handle, int32_t count, char *name);

int32_t
OSWaitSemaphore(SemaphoreHandle handle);

int32_t
OSTryWaitSemaphore(SemaphoreHandle handle);

int32_t
OSSignalSemaphore(SemaphoreHandle handle);

int32_t
OSGetSemaphoreCount(SemaphoreHandle handle);
