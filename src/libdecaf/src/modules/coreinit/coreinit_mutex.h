#pragma once
#include "coreinit_thread.h"
#include "common/be_val.h"
#include "common/structsize.h"
#include "virtual_ptr.h"

namespace coreinit
{

/**
 * \defgroup coreinit_mutex Mutex
 * \ingroup coreinit
 *
 * Standard mutex and condition variable implementation.
 *
 * Similar to <a href="http://en.cppreference.com/w/cpp/thread/condition_variable">std::condition_variable</a>.
 * Similar to <a href="http://en.cppreference.com/w/cpp/thread/recursive_mutex">std::recursive_mutex</a>.
 * @{
 */

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

   //! Should always be set to the value OSMutex::Tag.
   be_val<uint32_t> tag;

   //! Name set by OSInitMutexEx.
   be_ptr<const char> name;

   UNKNOWN(4);

   //! Queue of threads waiting for this mutex to unlock.
   OSThreadQueue queue;

   //! Current owner of mutex.
   be_ptr<OSThread> owner;

   //! Current recursion lock count of mutex.
   be_val<int32_t> count;

   //! Link used inside OSThread's mutex queue.
   OSMutexLink link;
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

   //! Should always be set to the value OSCondition::Tag.
   be_val<uint32_t> tag;

   //! Name set by OSInitCondEx.
   be_ptr<const char> name;

   UNKNOWN(4);

   //! Queue of threads currently waiting on condition with OSWaitCond.
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
OSInitMutexEx(OSMutex *mutex,
              const char *name);

void
OSLockMutex(OSMutex *mutex);

void
OSUnlockMutex(OSMutex *mutex);

BOOL
OSTryLockMutex(OSMutex *mutex);

void
OSInitCond(OSCondition *condition);

void
OSInitCondEx(OSCondition *condition,
             const char *name);

void
OSWaitCond(OSCondition *condition,
           OSMutex *mutex);

void
OSSignalCond(OSCondition *condition);

/** @} */

namespace internal
{

void
unlockAllMutexNoLock(OSThread *thread);

} // namespace internal

} // namespace coreinit
