#ifndef KERNEL_ENUM_H
#define KERNEL_ENUM_H

#include <common/enum_start.h>

ENUM_NAMESPACE_BEG(kernel)

ENUM_BEG(MapPermission, uint32_t)
   ENUM_VALUE(ReadOnly,                1)
   ENUM_VALUE(ReadWrite,               2)
ENUM_END(MapPermission)

ENUM_BEG(SyscallID, uint32_t)
   ENUM_VALUE(AllocVirtAddr,           0x3800)
   ENUM_VALUE(FreeVirtAddr,            0x3900)
   ENUM_VALUE(GetMapVirtAddrRange,     0x3A00)
   ENUM_VALUE(GetDataPhysAddrRange,    0x3B00)
   ENUM_VALUE(GetAvailPhysAddrRange,   0x3C00)
   ENUM_VALUE(MapMemory,               0x3D00)
   ENUM_VALUE(UnmapMemory,             0x3E00)
   ENUM_VALUE(QueryVirtAddr,           0x7500)
ENUM_END(SyscallID)

ENUM_BEG(VirtualMemoryType, uint32_t)
   ENUM_VALUE(Invalid,                 0)
   ENUM_VALUE(MappedReadOnly,          1)
   ENUM_VALUE(MappedReadWrite,         2)
   ENUM_VALUE(Free,                    3)
   ENUM_VALUE(Allocated,               4)
ENUM_END(VirtualMemoryType)

ENUM_NAMESPACE_END(kernel)

#include <common/enum_end.h>

#endif // ifdef KERNEL_ENUM_H
