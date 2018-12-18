#include "mmu.h"
#include "memorymap.h"
#include "memtrack.h"

namespace cpu
{

static MemoryMap
sMemoryMap;

bool
initialiseMemory()
{
   if (sMemoryMap.reserve()) {
      internal::initialiseMemtrack();
      return true;
   }
   return false;
}

bool
allocateVirtualAddress(VirtualAddress virtualAddress,
                       uint32_t size)
{
   return sMemoryMap.allocateVirtualAddress(virtualAddress, size);
}

bool
freeVirtualAddress(VirtualAddress virtualAddress,
                   uint32_t size)
{
   return sMemoryMap.freeVirtualAddress(virtualAddress, size);
}

VirtualAddressRange
findFreeVirtualAddress(uint32_t size,
                       uint32_t align)
{
   return sMemoryMap.findFreeVirtualAddress(size, align);
}

VirtualAddressRange
findFreeVirtualAddressInRange(VirtualAddressRange range,
                              uint32_t size,
                              uint32_t align)
{
   return sMemoryMap.findFreeVirtualAddressInRange(range, size, align);
}

bool
mapMemory(VirtualAddress virtualAddress,
          PhysicalAddress physicalAddress,
          uint32_t size,
          MapPermission permission)
{
   if (sMemoryMap.mapMemory(virtualAddress, physicalAddress, size, permission)) {
      if (permission == MapPermission::ReadWrite) {
         internal::registerTrackedRange(virtualAddress, physicalAddress, size);
      }
      return true;
   }
   return false;
}

bool
unmapMemory(VirtualAddress virtualAddress,
            uint32_t size)
{
   if (sMemoryMap.unmapMemory(virtualAddress, size)) {
      internal::unregisterTrackedRange(virtualAddress, size);
      return true;
   }
   return false;
}

bool
resetVirtualMemory()
{
   internal::clearTrackedRanges();
   return sMemoryMap.resetVirtualMemory();
}

VirtualMemoryType
queryVirtualAddress(VirtualAddress virtualAddress)
{
   return sMemoryMap.queryVirtualAddress(virtualAddress);
}

bool
isValidAddress(VirtualAddress address)
{
   PhysicalAddress physical;
   return sMemoryMap.virtualToPhysicalAddress(address, physical);
}

bool
virtualToPhysicalAddress(VirtualAddress virtualAddress,
                         PhysicalAddress &out)
{
   return sMemoryMap.virtualToPhysicalAddress(virtualAddress, out);
}

} // namespace cpu
