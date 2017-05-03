#pragma once
#include "kernel_enum.h"
#include <libcpu/address.h>

namespace kernel
{

namespace syscall
{

cpu::VirtualAddress
allocVirtAddr(cpu::VirtualAddress address,
              uint32_t size,
              uint32_t alignment);

bool
freeVirtAddr(cpu::VirtualAddress address,
             uint32_t size);

cpu::VirtualAddressRange
getMapVirtAddrRange();

cpu::PhysicalAddressRange
getDataPhysAddrRange();

cpu::PhysicalAddressRange
getAvailPhysAddrRange();

bool
mapMemory(cpu::VirtualAddress virtAddr,
          cpu::PhysicalAddress physAddr,
          uint32_t size,
          MapPermission permission);

bool
unmapMemory(cpu::VirtualAddress virtAddr,
            uint32_t size);

VirtualMemoryType
queryVirtAddr(cpu::VirtualAddress address);

} // namespace syscall

} // namespace kernel
