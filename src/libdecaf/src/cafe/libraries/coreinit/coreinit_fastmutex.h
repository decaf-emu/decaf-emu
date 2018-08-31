#pragma once
#include "coreinit_thread.h"

#include <atomic>
#include <libcpu/be2_struct.h>

namespace cafe::coreinit
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
   be2_virt_ptr<OSFastMutex> next;
   be2_virt_ptr<OSFastMutex> prev;
};
CHECK_OFFSET(OSFastMutexLink, 0x00, next);
CHECK_OFFSET(OSFastMutexLink, 0x04, prev);
CHECK_SIZE(OSFastMutexLink, 0x08);

struct OSFastMutex
{
   static constexpr uint32_t Tag = 0x664D7458u;

   //! Should always be set to the value OSFastMutex::Tag.
   be2_val<uint32_t> tag;

   //! Name set by OSFastMutex_Init.
   be2_virt_ptr<const char> name;

   //! Is this thread in the queue
   be2_val<BOOL> isContended;

   //! Queue of threads waiting for this mutex to unlock.
   be2_struct<OSThreadSimpleQueue> queue;

   //! Link used inside OSThread's fast mutex queue.
   be2_struct<OSFastMutexLink> link;

   //! Lock bits for the mutex, owner thread and some bits
   std::atomic<uint32_t> lock;

   //! Current recursion lock count of mutex.
   be2_val<int32_t> count;

   //! Link used for contended mutexes
   be2_struct<OSFastMutexLink> contendedLink;
};
CHECK_OFFSET(OSFastMutex, 0x00, tag);
CHECK_OFFSET(OSFastMutex, 0x04, name);
CHECK_OFFSET(OSFastMutex, 0x08, isContended);
CHECK_OFFSET(OSFastMutex, 0x0C, queue);
CHECK_OFFSET(OSFastMutex, 0x14, link);
CHECK_OFFSET(OSFastMutex, 0x1C, lock);
CHECK_OFFSET(OSFastMutex, 0x20, count);
CHECK_OFFSET(OSFastMutex, 0x24, contendedLink);
CHECK_SIZE(OSFastMutex, 0x2c);

struct OSFastCondition
{
   static constexpr uint32_t Tag = 0x664E6456u;

   //! Should always be set to the value OSFastCondition::Tag.
   be2_val<uint32_t> tag;

   //! Name set by OSFastCond_Init.
   be2_virt_ptr<const char> name;

   //! Unknown data
   be2_val<uint32_t> unk;

   //! Queue of threads waiting for this condition to signal.
   be2_struct<OSThreadQueue> queue;
};
CHECK_OFFSET(OSFastCondition, 0x00, tag);
CHECK_OFFSET(OSFastCondition, 0x04, name);
CHECK_OFFSET(OSFastCondition, 0x0C, queue);
CHECK_SIZE(OSFastCondition, 0x1c);

#pragma pack(pop)

void
OSFastMutex_Init(virt_ptr<OSFastMutex> mutex,
                 virt_ptr<const char> name);

void
OSFastMutex_Lock(virt_ptr<OSFastMutex> mutex);

void
OSFastMutex_Unlock(virt_ptr<OSFastMutex> mutex);

BOOL
OSFastMutex_TryLock(virt_ptr<OSFastMutex> mutex);

void
OSFastCond_Init(virt_ptr<OSFastCondition> condition,
                virt_ptr<const char> name);

void
OSFastCond_Wait(virt_ptr<OSFastCondition> condition,
                virt_ptr<OSFastMutex> mutex);

void
OSFastCond_Signal(virt_ptr<OSFastCondition> condition);

/** @} */

namespace internal
{

void
unlockAllFastMutexNoLock(virt_ptr<OSThread> thread);

} // namespace internal

} // namespace cafe::coreinit
