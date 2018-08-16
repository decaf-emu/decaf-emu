#pragma once
#include "coreinit_core.h"
#include "coreinit_time.h"
#include <atomic>
#include <libcpu/be2_struct.h>

namespace cafe::coreinit
{

/**
 * \defgroup coreinit_fiber Fiber
 * \ingroup coreinit
 * @{
 */

#pragma pack(push, 1)

struct OSRendezvous
{
   std::atomic<uint32_t> core[CoreCount];
   UNKNOWN(4);
};
CHECK_OFFSET(OSRendezvous, 0x00, core);
CHECK_SIZE(OSRendezvous, 0x10);

#pragma pack(pop)

void
OSInitRendezvous(virt_ptr<OSRendezvous> rendezvous);

BOOL
OSWaitRendezvous(virt_ptr<OSRendezvous> rendezvous,
                 uint32_t coreMask);

BOOL
OSWaitRendezvousWithTimeout(virt_ptr<OSRendezvous> rendezvous,
                            uint32_t coreMask,
                            OSTimeNanoseconds timeout);

/** @} */

} // namespace cafe::coreinit
