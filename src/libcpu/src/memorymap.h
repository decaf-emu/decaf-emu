#pragma once
#include "address.h"
#include "mmu.h"
#include "pointer.h"

#include <common/platform_memory.h>
#include <cstdint>
#include <vector>

namespace cpu
{

class MemoryMap
{
   struct VirtualReservation
   {
      //! Inclusive start address.
      VirtualAddress start;

      //! Inclusive end address.
      VirtualAddress end;
   };

   struct VirtualMemoryMap
   {
      VirtualAddress virtualAddress;
      PhysicalAddress physicalAddress;
      uint32_t size;
      MapPermission permission;
   };

public:
   MemoryMap() = default;
   ~MemoryMap();

   bool
   reserve();

   void
   free();

   bool
   allocateVirtualAddress(VirtualAddress start,
                          uint32_t size);

   bool
   freeVirtualAddress(VirtualAddress start,
                      uint32_t size);

   VirtualAddressRange
   findFreeVirtualAddress(uint32_t size,
                          uint32_t align);

   VirtualAddressRange
   findFreeVirtualAddressInRange(VirtualAddressRange range,
                                 uint32_t size,
                                 uint32_t align);

   bool
   virtualToPhysicalAddress(VirtualAddress virtualAddress,
                            PhysicalAddress &out);

   bool
   mapMemory(VirtualAddress virtualAddress,
             PhysicalAddress physicalAddress,
             uint32_t size,
             MapPermission permission);

   bool
   unmapMemory(VirtualAddress virtualAddress,
               uint32_t size);

   bool
   resetVirtualMemory();

   PhysicalMemoryType
   queryPhysicalAddress(PhysicalAddress physicalAddress);

   VirtualMemoryType
   queryVirtualAddress(VirtualAddress virtualAddress);

private:
   uintptr_t reserveBaseAddress();

   bool
   isVirtualAddressFree(VirtualAddress start,
                        uint32_t size);

   bool
   acquireReservation(VirtualReservation reservation);

   bool
   releaseReservation(VirtualReservation reservation);

   void *getPhysicalPointer(PhysicalAddress physicalAddress);
   void *getVirtualPointer(VirtualAddress virtualAddress);

private:
   platform::MapFileHandle mMem0 = platform::InvalidMapFileHandle;
   platform::MapFileHandle mMem1 = platform::InvalidMapFileHandle;
   platform::MapFileHandle mMem2 = platform::InvalidMapFileHandle;
   platform::MapFileHandle mUnkRam = platform::InvalidMapFileHandle;
   platform::MapFileHandle mSram0 = platform::InvalidMapFileHandle;
   platform::MapFileHandle mSram1 = platform::InvalidMapFileHandle;
   platform::MapFileHandle mLockedCache = platform::InvalidMapFileHandle;
   platform::MapFileHandle mTilingAperture = platform::InvalidMapFileHandle;

   uintptr_t mVirtualBase = 0;
   uintptr_t mPhysicalBase = 0;
   std::vector<VirtualMemoryMap> mMappedMemory;
   std::vector<VirtualReservation> mReservedMemory;
};

} // namespace cpu
