#pragma once
#include "coreinit_threadqueue.h"
#include "utils/be_val.h"
#include "utils/structsize.h"
#include "utils/virtual_ptr.h"

#pragma pack(push, 1)

struct OSMutex;

struct OSMutexLink
{
   be_ptr<OSMutex> next;
   be_ptr<OSMutex> prev;
};
CHECK_OFFSET(OSMutexLink, 0x00, next);
CHECK_OFFSET(OSMutexLink, 0x04, prev);
CHECK_SIZE(OSMutexLink, 0x8);

struct OSMutex
{
   static const uint32_t Tag = 0x6D557458;

   be_val<uint32_t> tag;
   be_ptr<const char> name;
   UNKNOWN(4);
   OSThreadQueue queue;
   be_ptr<OSThread> owner;
   be_val<int32_t> count;
   OSMutexLink link;       // For thread's mutexQueue
};
CHECK_OFFSET(OSMutex, 0x00, tag);
CHECK_OFFSET(OSMutex, 0x04, name);
CHECK_OFFSET(OSMutex, 0x0c, queue);
CHECK_OFFSET(OSMutex, 0x1c, owner);
CHECK_OFFSET(OSMutex, 0x20, count);
CHECK_OFFSET(OSMutex, 0x24, link);
CHECK_SIZE(OSMutex, 0x2c);

struct OSCondition
{
   static const uint32_t Tag = 0x634E6456;

   be_val<uint32_t> tag;
   be_ptr<const char> name;
   UNKNOWN(4);
   OSThreadQueue queue;
};
CHECK_OFFSET(OSCondition, 0x00, tag);
CHECK_OFFSET(OSCondition, 0x04, name);
CHECK_OFFSET(OSCondition, 0x0c, queue);
CHECK_SIZE(OSCondition, 0x1c);

#pragma pack(pop)

void
OSInitMutex(OSMutex *mutex);

void
OSInitMutexEx(OSMutex *mutex, const char *name);

void
OSLockMutex(OSMutex *mutex);

void
OSUnlockMutex(OSMutex *mutex);

BOOL
OSTryLockMutex(OSMutex *mutex);

void
OSInitCond(OSCondition *condition);

void
OSInitCondEx(OSCondition *condition, const char *name);

void
OSWaitCond(OSCondition *condition, OSMutex *mutex);

void
OSSignalCond(OSCondition *condition);
