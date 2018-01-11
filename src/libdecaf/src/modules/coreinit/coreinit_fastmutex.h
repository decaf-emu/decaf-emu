#pragma once
#include "coreinit_thread.h"

#include <atomic>
#include <common/be_ptr.h>
#include <common/be_val.h>
#include <common/cbool.h>
#include <common/structsize.h>

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

static_assert(sizeof(std::atomic<be_val<uint32_t>>) == sizeof(be_val<uint32_t>),
   "We rely on non-atomic class-templated atomics");

struct OSFastMutex
{
   static const uint32_t Tag = 0x664D7458;

   //! Should always be set to the value OSFastMutex::Tag.
   be_val<uint32_t> tag;

   //! Name set by OSFastMutex_Init.
   be_ptr<const char> name;

   //! Is this thread in the queue
   be_val<BOOL> isContended;

   //! Queue of threads waiting for this mutex to unlock.
   OSThreadSimpleQueue queue;

   //! Link used inside OSThread's fast mutex queue.
   OSFastMutexLink link;

   //! Lock bits for the mutex, owner thread and some bits
   std::atomic<be_val<uint32_t>> lock;

   //! Current recursion lock count of mutex.
   be_val<int32_t> count;

   //! Link used for contended mutexes
   OSFastMutexLink contendedLink;
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
   static const uint32_t Tag = 0x664E6456;

   //! Should always be set to the value OSFastCondition::Tag.
   be_val<uint32_t> tag;

   //! Name set by OSFastCond_Init.
   be_ptr<const char> name;

   //! Unknown data
   be_val<uint32_t> unk;

   //! Queue of threads waiting for this condition to signal.
   OSThreadQueue queue;
};
CHECK_OFFSET(OSFastCondition, 0x00, tag);
CHECK_OFFSET(OSFastCondition, 0x04, name);
CHECK_OFFSET(OSFastCondition, 0x0C, queue);
CHECK_SIZE(OSFastCondition, 0x1c);

#pragma pack(pop)

void
OSFastMutex_Init(OSFastMutex *mutex,
                 const char *name);

void
OSFastMutex_Lock(OSFastMutex *mutex);

void
OSFastMutex_Unlock(OSFastMutex *mutex);

BOOL
OSFastMutex_TryLock(OSFastMutex *mutex);

void
OSFastCond_Init(OSFastCondition *condition,
                const char *name);

void
OSFastCond_Wait(OSFastCondition *condition,
                OSFastMutex *mutex);

void
OSFastCond_Signal(OSFastCondition *condition);

/** @} */

namespace internal
{

void
unlockAllFastMutexNoLock(OSThread *thread);

} // namespace internal

} // namespace coreinit
