#pragma once
#include "coreinit_thread.h"
#include "common/be_val.h"
#include "common/structsize.h"
#include "common/virtual_ptr.h"

namespace coreinit
{

/**
 * \defgroup coreinit_semaphore Semaphore
 * \ingroup coreinit
 *
 * Similar to Windows <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/ms685129(v=vs.85).aspx">Semaphore Objects</a>.
 * @{
 */

#pragma pack(push, 1)

struct OSSemaphore
{
   static const uint32_t Tag = 0x73506852;

   //! Should always be set to the value OSSemaphore::Tag.
   be_val<uint32_t> tag;

   //! Name set by OSInitMutexEx.
   be_ptr<const char> name;

   UNKNOWN(4);

   //! Current count of semaphore
   be_val<int32_t> count;

   //! Queue of threads waiting on semaphore object with OSWaitSemaphore
   OSThreadQueue queue;
};
CHECK_OFFSET(OSSemaphore, 0x00, tag);
CHECK_OFFSET(OSSemaphore, 0x04, name);
CHECK_OFFSET(OSSemaphore, 0x0C, count);
CHECK_OFFSET(OSSemaphore, 0x10, queue);
CHECK_SIZE(OSSemaphore, 0x20);

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

/** @} */

} // namespace coreinit
