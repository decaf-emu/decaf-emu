#pragma once
#include "coreinit_thread.h"

#include <libcpu/be2_struct.h>

namespace cafe::coreinit
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
   static constexpr uint32_t Tag = 0x73506852u;

   //! Should always be set to the value OSSemaphore::Tag.
   be2_val<uint32_t> tag;

   //! Name set by OSInitMutexEx.
   be2_virt_ptr<const char> name;

   UNKNOWN(4);

   //! Current count of semaphore
   be2_val<int32_t> count;

   //! Queue of threads waiting on semaphore object with OSWaitSemaphore
   be2_struct<OSThreadQueue> queue;
};
CHECK_OFFSET(OSSemaphore, 0x00, tag);
CHECK_OFFSET(OSSemaphore, 0x04, name);
CHECK_OFFSET(OSSemaphore, 0x0C, count);
CHECK_OFFSET(OSSemaphore, 0x10, queue);
CHECK_SIZE(OSSemaphore, 0x20);

#pragma pack(pop)

void
OSInitSemaphore(virt_ptr<OSSemaphore> semaphore,
                int32_t count);

void
OSInitSemaphoreEx(virt_ptr<OSSemaphore> semaphore,
                  int32_t count,
                  virt_ptr<const char> name);

int32_t
OSWaitSemaphore(virt_ptr<OSSemaphore> semaphore);

int32_t
OSTryWaitSemaphore(virt_ptr<OSSemaphore> semaphore);

int32_t
OSSignalSemaphore(virt_ptr<OSSemaphore> semaphore);

int32_t
OSGetSemaphoreCount(virt_ptr<OSSemaphore> semaphore);

/** @} */

} // namespace cafe::coreinit
