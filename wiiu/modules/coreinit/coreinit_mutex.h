#pragma once
#include <mutex>
#include <condition_variable>
#include "systemtypes.h"
#include "coreinit_thread.h"

#pragma pack(push, 1)

template<typename Type>
struct OSLinkedListEntry
{
   be_ptr<Type> next;
   be_ptr<Type> prev;
};
CHECK_OFFSET(OSLinkedListEntry<void>, 0x0, next);
CHECK_OFFSET(OSLinkedListEntry<void>, 0x4, prev);
CHECK_SIZE(OSLinkedListEntry<void>, 0x8);

// OSInitMutexEx
// OSLockMutex
struct OSMutex
{
   static const uint32_t Tag = 0x6D557458;

   be_val<uint32_t> tag;
   be_ptr<char> name;
   UNKNOWN(4);
   OSThreadQueue queue;
   be_ptr<OSThread> thread;
   be_val<int32_t> count;
   OSLinkedListEntry<OSMutex> threadLink; // thread->queueMutex
};
CHECK_OFFSET(OSMutex, 0x0, tag);
CHECK_OFFSET(OSMutex, 0x4, name);
CHECK_OFFSET(OSMutex, 0xc, queue);
CHECK_OFFSET(OSMutex, 0x1c, thread);
CHECK_OFFSET(OSMutex, 0x20, count);
CHECK_OFFSET(OSMutex, 0x24, threadLink);
CHECK_SIZE(OSMutex, 0x2c);

struct OSCond
{
   static const uint32_t Tag = 0x634E6456;
   be_val<uint32_t> tag;
   be_ptr<char> name;
   UNKNOWN(4);
   OSThreadQueue queue;
};
CHECK_OFFSET(OSMutex, 0x0, tag);
CHECK_OFFSET(OSMutex, 0x4, name);
CHECK_OFFSET(OSMutex, 0xc, queue);
CHECK_SIZE(OSCond, 0x1c);

struct WMutex
{
   static const uint32_t Tag = 0x6D557458;

   be_val<uint32_t> tag;
   be_ptr<char> name;
   std::recursive_mutex mutex;
   PADDING(0x24 - sizeof(std::recursive_mutex));
};
CHECK_SIZE(WMutex, 0x2c);

struct WCondition
{
   static const uint32_t Tag = 0x634E6456;

   be_val<uint32_t> tag;
   be_ptr<char> name;
   std::condition_variable_any cvar;
   PADDING(0x14 - sizeof(std::condition_variable_any));
};
CHECK_SIZE(WCondition, 0x1c);

#pragma pack(pop)

void
OSInitMutex(WMutex *mutex);

void
OSInitMutexEx(WMutex *mutex, char *name);

void
OSLockMutex(WMutex *mutex);

void
OSUnlockMutex(WMutex *mutex);

BOOL
OSTryLockMutex(WMutex *mutex);

void
OSInitCond(WCondition *cond);

void
OSInitCondEx(WCondition *cond, char *nname);

void
OSWaitCond(WCondition *cond, WMutex *mutex);

void
OSSignalCond(WCondition *cond);
