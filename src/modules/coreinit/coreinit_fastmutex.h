#pragma once
#include "coreinit_threadqueue.h"
#include "utils/be_val.h"
#include "utils/structsize.h"
#include "utils/virtual_ptr.h"

#pragma pack(push, 1)

struct OSFastMutex;

struct OSFastMutexLink
{
   be_ptr<OSFastMutex> next;
   be_ptr<OSFastMutex> prev;
};
CHECK_OFFSET(OSFastMutexLink, 0x00, next);
CHECK_OFFSET(OSFastMutexLink, 0x04, prev);
CHECK_SIZE(OSFastMutexLink, 0x08);

struct OSFastMutex
{
   static const uint32_t Tag = 0x664D7458;

   be_val<uint32_t> tag;
   be_ptr<const char> name;
   UNKNOWN(4);
   OSThreadSimpleQueue queue;
   OSFastMutexLink link;
   UNKNOWN(16);
};
CHECK_OFFSET(OSFastMutex, 0x00, tag);
CHECK_OFFSET(OSFastMutex, 0x04, name);
CHECK_OFFSET(OSFastMutex, 0x0c, queue);
CHECK_OFFSET(OSFastMutex, 0x14, link);
CHECK_SIZE(OSFastMutex, 0x2c);

struct OSFastCondition
{
   static const uint32_t Tag = 0x664E6456;

   be_val<uint32_t> tag;
   be_ptr<const char> name;
   UNKNOWN(4);
   OSThreadQueue queue;
};
CHECK_OFFSET(OSFastCondition, 0x00, tag);
CHECK_OFFSET(OSFastCondition, 0x04, name);
CHECK_OFFSET(OSFastCondition, 0x0c, queue);
CHECK_SIZE(OSFastCondition, 0x1c);

#pragma pack(pop)

void
OSFastMutex_Init(OSFastMutex *mutex, const char *name);

void
OSFastMutex_Lock(OSFastMutex *mutex);

void
OSFastMutex_Unlock(OSFastMutex *mutex);

BOOL
OSFastMutex_TryLock(OSFastMutex *mutex);

void
OSFastCond_Init(OSFastCondition *condition, const char *name);

void
OSFastCond_Wait(OSFastCondition *condition, OSFastMutex *mutex);

void
OSFastCond_Signal(OSFastCondition *condition);
