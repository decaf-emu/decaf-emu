#pragma once
#include "be_val.h"
#include "coreinit_threadqueue.h"
#include "virtual_ptr.h"

#pragma pack(push, 1)

struct OSSemaphore
{
   static const uint32_t Tag = 0x73506852;

   be_val<uint32_t> tag;
   be_ptr<const char> name;
   UNKNOWN(4);
   be_val<int32_t> count;
   OSThreadQueue queue;
};

#pragma pack(pop)

void
OSInitSemaphore(OSSemaphore *semaphore, int32_t count);

void
OSInitSemaphoreEx(OSSemaphore *semaphore, int32_t count, char *name);

int32_t
OSWaitSemaphore(OSSemaphore *semaphore);

int32_t
OSTryWaitSemaphore(OSSemaphore *semaphore);

int32_t
OSSignalSemaphore(OSSemaphore *semaphore);

int32_t
OSGetSemaphoreCount(OSSemaphore *semaphore);
