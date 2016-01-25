#pragma once
#include "coreinit_time.h"
#include "types.h"
#include "utils/structsize.h"
#include <atomic>

namespace coreinit
{

#pragma pack(push, 1)

struct OSRendezvous
{
   std::atomic<uint32_t> core[3];
   UNKNOWN(4);
};
CHECK_OFFSET(OSRendezvous, 0x00, core);
CHECK_SIZE(OSRendezvous, 0x10);
CHECK_SIZE(std::atomic<uint32_t>, 0x04);

#pragma pack(pop)

void
OSInitRendezvous(OSRendezvous *rendezvous);

BOOL
OSWaitRendezvous(OSRendezvous *rendezvous, uint32_t coreMask);

BOOL
OSWaitRendezvousWithTimeout(OSRendezvous *rendezvous, uint32_t coreMask, OSTime timeout);

} // namespace coreinit
