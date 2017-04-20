#include "mmu.h"
#include "memorymap.h"

namespace cpu
{

static MemoryMap
sMemoryMap;

bool
initialiseMemory()
{
   return sMemoryMap.reserve();
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

bool
mapMemory(VirtualAddress virtualAddress,
          PhysicalAddress physicalAddress,
          uint32_t size,
          MapPermission permission)
{
   return sMemoryMap.mapMemory(virtualAddress, physicalAddress, size, permission);
}

bool
unmapMemory(VirtualAddress virtualAddress,
            uint32_t size)
{
   return sMemoryMap.unmapMemory(virtualAddress, size);
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
