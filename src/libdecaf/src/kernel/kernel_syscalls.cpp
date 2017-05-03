#include "kernel_memory.h"
#include "kernel_syscalls.h"

#include <common/log.h>
#include <libcpu/address.h>
#include <libcpu/mmu.h>

namespace kernel
{

namespace syscall
{

static bool
inVirtualMapRange(cpu::VirtualAddress address,
                  uint32_t size)
{
   auto range = kernel::getVirtualMapRange();
   return (range.start <= address && address + size <= range.start + range.size);
}

cpu::VirtualAddress
allocVirtAddr(cpu::VirtualAddress address,
              uint32_t size,
              uint32_t alignment)
{
   if (!inVirtualMapRange(address, size)) {
      return cpu::VirtualAddress { 0u };
   }

   if (address) {
      address = align_up(address, alignment);
      size = align_up(size, alignment);
   } else {
      auto range = cpu::findFreeVirtualAddress(size, alignment);
      address = range.start;
      size = range.size;
   }

   if (cpu::allocateVirtualAddress(address, size)) {
      return address;
   } else {
      return cpu::VirtualAddress { 0u };
   }
}

bool
freeVirtAddr(cpu::VirtualAddress address,
             uint32_t size)
{
   if (!inVirtualMapRange(address, size)) {
      return false;
   }

   return cpu::freeVirtualAddress(address, size);
}

cpu::VirtualAddressRange
getMapVirtAddrRange()
{
   return kernel::getVirtualMapRange();
}

cpu::PhysicalAddressRange
getDataPhysAddrRange()
{
   return kernel::getDataPhysicalRange();
}

cpu::PhysicalAddressRange
getAvailPhysAddrRange()
{
   return kernel::getAvailPhysicalRange();
}

bool
mapMemory(cpu::VirtualAddress virtAddr,
          cpu::PhysicalAddress physAddr,
          uint32_t size,
          MapPermission permission)
{
   if (!inVirtualMapRange(virtAddr, size)) {
      return false;
   }

   auto cpuPermission = cpu::MapPermission { };

   if (permission == MapPermission::ReadOnly) {
      cpuPermission = cpu::MapPermission::ReadOnly;
   } else if (permission == MapPermission::ReadWrite) {
      cpuPermission = cpu::MapPermission::ReadWrite;
   } else {
      gLog->error("Unexpected mapMemory permission: {}", permission);
      return false;
   }

   return cpu::mapMemory(virtAddr, physAddr, size, cpuPermission);
}

bool
unmapMemory(cpu::VirtualAddress virtAddr,
            uint32_t size)
{
   if (!inVirtualMapRange(virtAddr, size)) {
      return false;
   }

   return cpu::unmapMemory(virtAddr, size);
}

VirtualMemoryType
queryVirtAddr(cpu::VirtualAddress address)
{
   if (!inVirtualMapRange(address, 0)) {
      return VirtualMemoryType::Invalid;
   }

   auto cpuType = cpu::queryVirtualAddress(address);

   switch (cpuType) {
   case cpu::VirtualMemoryType::MappedReadOnly:
      return VirtualMemoryType::MappedReadOnly;
   case cpu::VirtualMemoryType::MappedReadWrite:
      return VirtualMemoryType::MappedReadWrite;
   case cpu::VirtualMemoryType::Free:
      return VirtualMemoryType::Free;
   case cpu::VirtualMemoryType::Allocated:
      return VirtualMemoryType::Allocated;
   default:
      return VirtualMemoryType::Invalid;
   }
}

} // namespace syscall

} // namespace kernel
