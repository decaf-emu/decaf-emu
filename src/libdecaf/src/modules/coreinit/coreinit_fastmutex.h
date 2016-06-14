#pragma once
#include <atomic>
#include "coreinit_thread.h"
#include "common/be_val.h"
#include "common/structsize.h"
#include "virtual_ptr.h"

namespace coreinit
{

/**
 * \defgroup coreinit_fastmutex Fast Mutex
 * \ingroup coreinit
 *
 * Similar to OSMutex but tries to acquire the mutex without using the global
 * scheduler lock, and does not test for thread cancel.
 * @{
 */

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

   //! Should always be set to the value OSFastMutex::Tag.
   be_val<uint32_t> tag;

   //! Name set by OSFastMutex_Init.
   be_ptr<const char> name;

   UNKNOWN(4);

   //! Queue of threads waiting for this mutex to unlock.
   OSThreadSimpleQueue queue;

   //! Link used inside OSThread's fast mutex queue.
   OSFastMutexLink link;

   //! Current owner of mutex.
   std::atomic<uint32_t> owner;

   //! Current recursion lock count of mutex.
   be_val<int32_t> count;

   UNKNOWN(8);
};
CHECK_OFFSET(OSFastMutex, 0x00, tag);
CHECK_OFFSET(OSFastMutex, 0x04, name);
CHECK_OFFSET(OSFastMutex, 0x0c, queue);
CHECK_OFFSET(OSFastMutex, 0x14, link);
CHECK_OFFSET(OSFastMutex, 0x1c, owner);
CHECK_OFFSET(OSFastMutex, 0x20, count);
CHECK_SIZE(OSFastMutex, 0x2c);

struct OSFastCondition
{
   static const uint32_t Tag = 0x664E6456;

   //! Should always be set to the value OSFastCondition::Tag.
   be_val<uint32_t> tag;

   //! Name set by OSFastCond_Init.
   be_ptr<const char> name;

   UNKNOWN(4);

   //! Queue of threads waiting for this condition to signal.
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

/** @} */

} // namespace coreinit
