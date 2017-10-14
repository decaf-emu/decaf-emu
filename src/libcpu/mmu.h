#pragma once
#include "address.h"
#include <common/decaf_assert.h>

namespace cpu
{

namespace internal
{

extern uintptr_t BaseVirtualAddress;
extern uintptr_t BasePhysicalAddress;

template<typename Value>
inline Value *translate(VirtualAddress address)
{
   return reinterpret_cast<Value *>(BaseVirtualAddress + address.getAddress());
}

template<typename Value>
inline Value *translate(PhysicalAddress address)
{
   return reinterpret_cast<Value *>(BasePhysicalAddress + address.getAddress());
}

} // namespace internal

enum class MapPermission
{
   ReadOnly,
   ReadWrite,
};

enum class VirtualMemoryType
{
   Invalid,
   MappedReadOnly,
   MappedReadWrite,
   Free,
   Allocated,
};

enum class PhysicalMemoryType
{
   Invalid,
   MEM0,
   MEM1,
   MEM2,
   LockedCache,
   SRAM0,
   SRAM1,
};

constexpr auto PageSize = uint32_t { 128 * 1024 };

bool
initialiseMemory();

bool
allocateVirtualAddress(VirtualAddress virtualAddress,
                       uint32_t size);

bool
freeVirtualAddress(VirtualAddress virtualAddress,
                   uint32_t size);

VirtualAddressRange
findFreeVirtualAddress(uint32_t size,
                       uint32_t align);

VirtualAddressRange
findFreeVirtualAddressInRange(VirtualAddressRange range,
                              uint32_t size,
                              uint32_t align);

bool
mapMemory(VirtualAddress virtualAddress,
          PhysicalAddress physicalAddress,
          uint32_t size,
          MapPermission permission);

bool
unmapMemory(VirtualAddress virtualAddress,
            uint32_t size);

VirtualMemoryType
queryVirtualAddress(VirtualAddress virtualAddress);

bool
isValidAddress(VirtualAddress address);

bool
virtualToPhysicalAddress(VirtualAddress virtualAddress,
                         PhysicalAddress &out);

template<typename Type>
inline VirtualAddress
translate(Type *pointer)
{
   if (!pointer) {
      return VirtualAddress { 0u };
   } else {
      auto addr = reinterpret_cast<uintptr_t>(pointer);
      decaf_check(addr >= internal::BaseVirtualAddress);
      decaf_check(addr <= internal::BaseVirtualAddress + 0x100000000ull);
      return VirtualAddress { static_cast<uint32_t>(addr - internal::BaseVirtualAddress) };
   }
}

template<typename Type>
inline PhysicalAddress
translatePhysical(Type *pointer)
{
   if (!pointer) {
      return PhysicalAddress { 0u };
   } else {
      auto addr = reinterpret_cast<uintptr_t>(pointer);
      decaf_check(addr >= internal::BasePhysicalAddress);
      decaf_check(addr <= internal::BasePhysicalAddress + 0x100000000ull);
      return PhysicalAddress { static_cast<uint32_t>(addr - internal::BasePhysicalAddress) };
   }
}

inline uintptr_t
getBaseVirtualAddress()
{
   return internal::BaseVirtualAddress;
}

inline uintptr_t
getBasePhysicalAddress()
{
   return internal::BasePhysicalAddress;
}

} // namespace cpu
