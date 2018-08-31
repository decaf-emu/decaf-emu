#pragma once
#include "coreinit_thread.h"

#include <libcpu/be2_struct.h>

namespace cafe::coreinit
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
   be2_virt_ptr<OSMutex> next;
   be2_virt_ptr<OSMutex> prev;
};
CHECK_OFFSET(OSMutexLink, 0x00, next);
CHECK_OFFSET(OSMutexLink, 0x04, prev);
CHECK_SIZE(OSMutexLink, 0x8);

struct OSMutex
{
   static constexpr auto Tag = 0x6D557458u;

   //! Should always be set to the value OSMutex::Tag.
   be2_val<uint32_t> tag;

   //! Name set by OSInitMutexEx.
   be2_virt_ptr<const char> name;

   UNKNOWN(4);

   //! Queue of threads waiting for this mutex to unlock.
   be2_struct<OSThreadQueue> queue;

   //! Current owner of mutex.
   be2_virt_ptr<OSThread> owner;

   //! Current recursion lock count of mutex.
   be2_val<int32_t> count;

   //! Link used inside OSThread's mutex queue.
   be2_struct<OSMutexLink> link;
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
   static constexpr auto Tag = 0x634E6456u;

   //! Should always be set to the value OSCondition::Tag.
   be2_val<uint32_t> tag;

   //! Name set by OSInitCondEx.
   be2_virt_ptr<const char> name;

   UNKNOWN(4);

   //! Queue of threads currently waiting on condition with OSWaitCond.
   be2_struct<OSThreadQueue> queue;
};
CHECK_OFFSET(OSCondition, 0x00, tag);
CHECK_OFFSET(OSCondition, 0x04, name);
CHECK_OFFSET(OSCondition, 0x0c, queue);
CHECK_SIZE(OSCondition, 0x1c);

#pragma pack(pop)

void
OSInitMutex(virt_ptr<OSMutex> mutex);

void
OSInitMutexEx(virt_ptr<OSMutex> mutex,
              virt_ptr<const char> name);

void
OSLockMutex(virt_ptr<OSMutex> mutex);

void
OSUnlockMutex(virt_ptr<OSMutex> mutex);

BOOL
OSTryLockMutex(virt_ptr<OSMutex> mutex);

void
OSInitCond(virt_ptr<OSCondition> condition);

void
OSInitCondEx(virt_ptr<OSCondition> condition,
             virt_ptr<const char> name);

void
OSWaitCond(virt_ptr<OSCondition> condition,
           virt_ptr<OSMutex> mutex);

void
OSSignalCond(virt_ptr<OSCondition> condition);

/** @} */

namespace internal
{

void
unlockAllMutexNoLock(virt_ptr<OSThread> thread);

} // namespace internal

} // namespace cafe::coreinit
