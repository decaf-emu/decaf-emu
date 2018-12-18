#include "memtrack.h"
#include "mmu.h"

#include <common/datahash.h>
#include <common/platform.h>
#ifdef PLATFORM_POSIX

namespace cpu
{

namespace internal
{

void
initialiseMemtrack()
{
}

void
registerTrackedRange(VirtualAddress virtualAddress,
                     PhysicalAddress physicalAddress,
                     uint32_t size)
{
}

void
unregisterTrackedRange(VirtualAddress virtualAddress,
                       uint32_t size)
{
}

void
clearTrackedRanges()
{
}

} // namespace internal

MemtrackState
getMemoryState(PhysicalAddress physicalAddress,
               uint32_t size)
{
   // POSIX always hashes for the moment.  This is due to my inability to actually
   // test any sort of implementation on a linux system.
   auto physPtr = reinterpret_cast<void *>(cpu::getBasePhysicalAddress() + physicalAddress.getAddress());
   auto hashVal = DataHash {}.write(physPtr, size);
   return MemtrackState { hashVal.value() };
}

} // namespace cpu

#endif // PLATFORM_POSIX
