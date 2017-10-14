#include "memorymap.h"

#include <algorithm>
#include <common/align.h>
#include <common/log.h>
#include <common/platform.h>
#include <common/platform_memory.h>

namespace cpu
{

namespace internal
{

uintptr_t BaseVirtualAddress = 0;
uintptr_t BasePhysicalAddress = 0;

} // namespace internal

static constexpr PhysicalAddress MEM1BaseAddress = PhysicalAddress { 0 };
static constexpr PhysicalAddress MEM1EndAddress  = PhysicalAddress { 0x01FFFFFF };
static constexpr size_t MEM1Size = (MEM1EndAddress - MEM1BaseAddress) + 1;

// HACK: Doesn't exist at this physical address on hardware. _DECAF ONLY_
// HACK: Set to page size (128kb) even though it's actually 16kb per core,
// due to memory map restrictions.
static constexpr PhysicalAddress LCBaseAddress = PhysicalAddress { 0x02000000 };
static constexpr PhysicalAddress LCEndAddress = PhysicalAddress { 0x0201FFFF };
static constexpr size_t LCSize = (LCEndAddress - LCBaseAddress) + 1;

static constexpr PhysicalAddress MEM0BaseAddress = PhysicalAddress { 0x08000000 };
static constexpr PhysicalAddress MEM0EndAddress = PhysicalAddress { 0x082DFFFF };
static constexpr size_t MEM0Size = (MEM0EndAddress - MEM0BaseAddress) + 1;

static constexpr PhysicalAddress MEM2BaseAddress = PhysicalAddress { 0x10000000 };
static constexpr PhysicalAddress MEM2EndAddress  = PhysicalAddress { 0x8FFFFFFF };
static constexpr size_t MEM2Size = (MEM2EndAddress - MEM2BaseAddress) + 1;

static constexpr PhysicalAddress SRAM1BaseAddress = PhysicalAddress { 0xFFF00000 };
static constexpr PhysicalAddress SRAM1EndAddress = PhysicalAddress { 0xFFF07FFF };
static constexpr size_t SRAM1Size = (SRAM1EndAddress - SRAM1BaseAddress) + 1;

static constexpr PhysicalAddress SRAM0BaseAddress = PhysicalAddress { 0xFFFF0000 };
static constexpr PhysicalAddress SRAM0EndAddress = PhysicalAddress { 0xFFFFFFFF };
static constexpr size_t SRAM0Size = (SRAM0EndAddress - SRAM0BaseAddress) + 1;


MemoryMap::~MemoryMap()
{
   free();
}


bool
MemoryMap::reserve()
{
   decaf_check(platform::getSystemPageSize() <= cpu::PageSize);

   // Reserve physical address space
   mPhysicalBase = reserveBaseAddress();

   if (!mPhysicalBase) {
      gLog->error("Unable to reserve base address for physical memory");
      free();
      return false;
   }

   // Reserve virtual address space
   mVirtualBase = reserveBaseAddress();

   if (!mVirtualBase) {
      gLog->error("Unable to reserve base address for virtual memory");
      free();
      return false;
   }

   internal::BaseVirtualAddress = mVirtualBase;
   internal::BasePhysicalAddress = mPhysicalBase;
   mReservedMemory.push_back({ VirtualAddress { 0 }, VirtualAddress { 0xFFFFFFFF } });

   // Commit MEM0
   mMem0 = platform::createMemoryMappedFile(MEM0Size);
   if (mMem0 == platform::InvalidMapFileHandle) {
      gLog->error("Unable to create MEM1 mapping");
      free();
      return false;
   }

   // Commit MEM1
   mMem1 = platform::createMemoryMappedFile(MEM1Size);
   if (mMem1 == platform::InvalidMapFileHandle) {
      gLog->error("Unable to create MEM1 mapping");
      free();
      return false;
   }

   // Commit MEM2
   mMem2 = platform::createMemoryMappedFile(MEM2Size);
   if (mMem2 == platform::InvalidMapFileHandle) {
      gLog->error("Unable to create MEM2 mapping");
      free();
      return false;
   }

   // Commit LC
   mLockedCache = platform::createMemoryMappedFile(LCSize);
   if (mLockedCache == platform::InvalidMapFileHandle) {
      gLog->error("Unable to create LC mapping");
      free();
      return false;
   }

   // Commit SRAM0
   mSram0 = platform::createMemoryMappedFile(SRAM0Size);
   if (mSram0 == platform::InvalidMapFileHandle) {
      gLog->error("Unable to create SRAM0 mapping");
      free();
      return false;
   }

   // Commit SRAM1
   mSram1 = platform::createMemoryMappedFile(SRAM1Size);
   if (mSram1 == platform::InvalidMapFileHandle) {
      gLog->error("Unable to create SRAM1 mapping");
      free();
      return false;
   }

   // Release our reserved memory so we can map it
   if (!platform::freeMemory(mPhysicalBase, 0x100000000ull)) {
      gLog->error("Unable to release physical address space");
      free();
      return false;
   }

   // Map physical address space
   auto ptrMem0 = getPhysicalPointer(MEM0BaseAddress);
   auto viewMem0 = platform::mapViewOfFile(mMem0, platform::ProtectFlags::ReadWrite, 0, MEM0Size, ptrMem0);
   if (viewMem0 != ptrMem0) {
      gLog->error("Unable to map MEM0 to physical address space");
      free();
      return false;
   }

   auto ptrMem1 = getPhysicalPointer(MEM1BaseAddress);
   auto viewMem1 = platform::mapViewOfFile(mMem1, platform::ProtectFlags::ReadWrite, 0, MEM1Size, ptrMem1);
   if (viewMem1 != ptrMem1) {
      gLog->error("Unable to map MEM1 to physical address space");
      free();
      return false;
   }

   auto ptrMem2 = getPhysicalPointer(MEM2BaseAddress);
   auto viewMem2 = platform::mapViewOfFile(mMem2, platform::ProtectFlags::ReadWrite, 0, MEM2Size, ptrMem2);
   if (viewMem2 != ptrMem2) {
      gLog->error("Unable to map MEM2 to physical address space");
      free();
      return false;
   }

   auto ptrLC = getPhysicalPointer(LCBaseAddress);
   auto viewLC = platform::mapViewOfFile(mLockedCache, platform::ProtectFlags::ReadWrite, 0, LCSize, ptrLC);
   if (viewLC != ptrLC) {
      gLog->error("Unable to map LC to physical address space");
      free();
      return false;
   }

   auto ptrSram0 = getPhysicalPointer(SRAM0BaseAddress);
   auto viewSram0 = platform::mapViewOfFile(mSram0, platform::ProtectFlags::ReadWrite, 0, SRAM0Size, ptrSram0);
   if (viewSram0 != ptrSram0) {
      gLog->error("Unable to map SRAM0 to physical address space");
      free();
      return false;
   }

   auto ptrSram1 = getPhysicalPointer(SRAM1BaseAddress);
   auto viewSram1 = platform::mapViewOfFile(mSram1, platform::ProtectFlags::ReadWrite, 0, SRAM1Size, ptrSram1);
   if (viewSram1 != ptrSram1) {
      gLog->error("Unable to map SRAM1 to physical address space");
      free();
      return false;
   }

   return true;
}


void
MemoryMap::free()
{
   // Unmap all views
   while (mMappedMemory.size()) {
      auto &mapping = mMappedMemory[0];
      unmapMemory(mapping.virtualAddress, mapping.size);
   }

   mReservedMemory.clear();

   // Close file mappings
   if (mMem0 != platform::InvalidMapFileHandle) {
      platform::unmapViewOfFile(getPhysicalPointer(MEM0BaseAddress), MEM0Size);
      platform::closeMemoryMappedFile(mMem0);
      mMem0 = platform::InvalidMapFileHandle;
   }

   if (mMem1 != platform::InvalidMapFileHandle) {
      platform::unmapViewOfFile(getPhysicalPointer(MEM1BaseAddress), MEM1Size);
      platform::closeMemoryMappedFile(mMem1);
      mMem1 = platform::InvalidMapFileHandle;
   }

   if (mMem2 != platform::InvalidMapFileHandle) {
      platform::unmapViewOfFile(getPhysicalPointer(MEM2BaseAddress), MEM2Size);
      platform::closeMemoryMappedFile(mMem2);
      mMem2 = platform::InvalidMapFileHandle;
   }

   if (mLockedCache != platform::InvalidMapFileHandle) {
      platform::unmapViewOfFile(getPhysicalPointer(LCBaseAddress), LCSize);
      platform::closeMemoryMappedFile(mLockedCache);
      mLockedCache = platform::InvalidMapFileHandle;
   }

   if (mSram0 != platform::InvalidMapFileHandle) {
      platform::unmapViewOfFile(getPhysicalPointer(SRAM0BaseAddress), SRAM0Size);
      platform::closeMemoryMappedFile(mSram0);
      mSram0 = platform::InvalidMapFileHandle;
   }

   if (mSram1 != platform::InvalidMapFileHandle) {
      platform::unmapViewOfFile(getPhysicalPointer(SRAM1BaseAddress), SRAM1Size);
      platform::closeMemoryMappedFile(mSram1);
      mSram1 = platform::InvalidMapFileHandle;
   }

   // Release virtual memory
   if (mVirtualBase) {
      platform::freeMemory(mVirtualBase, 0x100000000ull);
      mVirtualBase = 0;
   }

   // Release physical memory
   if (mPhysicalBase) {
      platform::freeMemory(mPhysicalBase, 0x100000000ull);
      mPhysicalBase = 0;
   }
}


bool
MemoryMap::isVirtualAddressFree(VirtualAddress start,
                                uint32_t size)
{
   auto end = start + (size - 1);

   for (auto &reservation : mReservedMemory) {
      if (reservation.start <= start && reservation.end >= end) {
         return true;
      }
   }

   return false;
}


VirtualMemoryType
MemoryMap::queryVirtualAddress(VirtualAddress virtualAddress)
{
   for (auto &mapped : mMappedMemory) {
      auto mapStart = mapped.virtualAddress;
      auto mapEnd = mapped.virtualAddress + (mapped.size - 1);

      if (virtualAddress >= mapStart && virtualAddress <= mapEnd) {
         if (mapped.permission == MapPermission::ReadOnly) {
            return VirtualMemoryType::MappedReadOnly;
         } else {
            return VirtualMemoryType::MappedReadWrite;
         }
      }
   }

   if (isVirtualAddressFree(virtualAddress, 1)) {
      return VirtualMemoryType::Free;
   }

   return VirtualMemoryType::Allocated;
}


PhysicalMemoryType
MemoryMap::queryPhysicalAddress(PhysicalAddress physicalAddress)
{
   if (physicalAddress >= MEM0BaseAddress && physicalAddress <= MEM0EndAddress) {
      return PhysicalMemoryType::LockedCache;
   } else if (physicalAddress >= MEM1BaseAddress && physicalAddress <= MEM1EndAddress) {
      return PhysicalMemoryType::MEM1;
   } else if (physicalAddress >= MEM2BaseAddress && physicalAddress <= MEM2EndAddress) {
      return PhysicalMemoryType::MEM2;
   } else if (physicalAddress >= LCBaseAddress && physicalAddress <= LCEndAddress) {
      return PhysicalMemoryType::LockedCache;
   } else if (physicalAddress >= SRAM0BaseAddress && physicalAddress <= SRAM0EndAddress) {
      return PhysicalMemoryType::SRAM0;
   } else if (physicalAddress >= SRAM1BaseAddress && physicalAddress <= SRAM1EndAddress) {
      return PhysicalMemoryType::SRAM1;
   }

   return PhysicalMemoryType::Invalid;
}


bool
MemoryMap::virtualToPhysicalAddress(VirtualAddress virtualAddress,
                                    PhysicalAddress &out)
{
   for (auto &mapping : mMappedMemory) {
      if (mapping.virtualAddress <= virtualAddress
         && mapping.virtualAddress + mapping.size > virtualAddress) {
         out = mapping.physicalAddress + (virtualAddress - mapping.virtualAddress);
         return true;
      }
   }

   return false;
}

bool
MemoryMap::allocateVirtualAddress(VirtualAddress start,
                                  uint32_t size)
{
   if (size == 0) {
      return false;
   }

   start = align_up(start, cpu::PageSize);
   size = align_up(size, cpu::PageSize);
   auto end = start + (size - 1);

   if (!isVirtualAddressFree(start, size)) {
      // Must be free to be able to allocate.
      return false;
   }

   for (auto itr = mReservedMemory.begin(); itr != mReservedMemory.end(); ++itr) {
      auto &reservation = *itr;

      if (start >= reservation.start && end <= reservation.end) {
         releaseReservation(reservation);

         if (start == reservation.start && end == reservation.end) {
            // Consumed whole reservation
            mReservedMemory.erase(itr);
         } else if (start == reservation.start) {
            // Consumed from start of reservation
            reservation.start += size;
            acquireReservation(reservation);
         } else if (end == reservation.end) {
            // Consumed from end of reservation
            reservation.end -= size;
            acquireReservation(reservation);
         } else {
            // Consumed in middle of reservation
            auto newReservation = VirtualReservation {};
            newReservation.start = start + size;
            newReservation.end = reservation.end;
            acquireReservation(newReservation);

            reservation.end = start;
            reservation.end -= 1;
            acquireReservation(reservation);
            mReservedMemory.insert(itr + 1, newReservation);
         }

         return true;
      }
   }

   return false;
}

bool
MemoryMap::freeVirtualAddress(VirtualAddress start,
                              uint32_t size)
{
   start = align_up(start, cpu::PageSize);
   size = align_up(size, cpu::PageSize);

   if (isVirtualAddressFree(start, size)) {
      // Check if it's already free
      return true;
   }

   // Check if we can combine with a previous reservation
   for (auto itr = mReservedMemory.begin(); itr != mReservedMemory.end(); ++itr) {
      auto &reservation = *itr;
      auto mergeItr = mReservedMemory.end();

      if (reservation.start == start + size) {
         // Expand reservation backward
         releaseReservation(reservation);
         reservation.start -= size;

         if (itr != mReservedMemory.begin()) {
            // Check if we can merge with the previous reservation
            auto prev = itr - 1;

            if (prev->end + 1 == reservation.start) {
               releaseReservation(*prev);
               reservation.start = prev->start;
               mergeItr = prev;
            }
         }

         acquireReservation(reservation);

         if (mergeItr != mReservedMemory.end()) {
            mReservedMemory.erase(mergeItr);
         }

         return true;
      } else if (reservation.end + 1 == start) {
         // Expand reservation forward
         releaseReservation(reservation);
         reservation.end += size;

         auto next = itr + 1;

         if (next != mReservedMemory.end()) {
            // Check if we can merge with the next reservation
            if (next->start == reservation.end + 1) {
               releaseReservation(*next);
               reservation.end = next->end;
               mergeItr = next;
            }
         }

         acquireReservation(reservation);

         if (mergeItr != mReservedMemory.end()) {
            mReservedMemory.erase(mergeItr);
         }

         return true;
      }
   }

   // We cannot combine with a previous reservation so we must make a new one!
   auto reservation = VirtualReservation {};
   reservation.start = start;
   reservation.end = start + (size - 1);
   acquireReservation(reservation);

   mReservedMemory.insert(std::upper_bound(mReservedMemory.begin(),
                                           mReservedMemory.end(),
                                           reservation,
                                           [](const auto &m1, const auto &m2) {
      return m1.start < m2.start;
   }),
                          reservation);
   return true;
}

VirtualAddressRange
MemoryMap::findFreeVirtualAddress(uint32_t size,
                                  uint32_t align)
{
   size = align_up(size, cpu::PageSize);

   for (auto &reservation : mReservedMemory) {
      auto alignedStart = align_up(align_up(reservation.start, align), cpu::PageSize);
      auto alignedEnd = alignedStart + (size - 1);

      if (reservation.start <= alignedStart && reservation.end >= alignedEnd) {
         return { alignedStart, size };
      }
   }
   return { VirtualAddress { 0u }, 0u };
}

VirtualAddressRange
MemoryMap::findFreeVirtualAddressInRange(VirtualAddressRange range,
                                         uint32_t size,
                                         uint32_t align)
{
   auto rangeStart = range.start;
   auto rangeEnd = range.start + (range.size - 1);
   size = align_up(size, cpu::PageSize);

   for (auto &reservation : mReservedMemory) {
      if (rangeEnd < reservation.start || rangeStart > reservation.end) {
         // Not in range
         continue;
      }

      auto start = std::max(reservation.start, rangeStart);
      auto end = std::min(reservation.end, rangeEnd);

      auto alignedStart = align_up(align_up(start, align), cpu::PageSize);
      auto alignedEnd = alignedStart + (size - 1);

      // Ensure address is in range
      if (start <= alignedStart && end >= alignedEnd) {
         return { alignedStart, size };
      }
   }

   return { VirtualAddress { 0u }, 0u };
}

bool
MemoryMap::mapMemory(VirtualAddress virtualAddress,
                     PhysicalAddress physicalAddress,
                     uint32_t size,
                     MapPermission permission)
{
   auto physicalMemoryType = queryPhysicalAddress(physicalAddress);

   if (physicalMemoryType == PhysicalMemoryType::Invalid) {
      gLog->error("Attempted to map invalid physical address 0x{:08X}",
                  physicalAddress.getAddress());
      return false;
   }

   if (queryVirtualAddress(virtualAddress) != VirtualMemoryType::Allocated) {
      gLog->error("Attempted to map physical address 0x{:08X} to an invalid virtual address 0x{:08X}",
                  virtualAddress.getAddress(), physicalAddress.getAddress());
      return false;
   }

   // Map virtual address to physical memory
   void *view = nullptr;
   auto virtualPtr = getVirtualPointer(virtualAddress);
   auto protectFlags = platform::ProtectFlags { };

   if (permission == MapPermission::ReadOnly) {
      protectFlags = platform::ProtectFlags::ReadOnly;
   } else if (permission == MapPermission::ReadWrite) {
      protectFlags = platform::ProtectFlags::ReadWrite;
   } else {
      gLog->error("Invalid permission {} passed to mapMemory", static_cast<int>(permission));
      return false;
   }

   if (physicalMemoryType == PhysicalMemoryType::MEM0) {
      view = platform::mapViewOfFile(mMem0, protectFlags, physicalAddress - MEM0BaseAddress, size, virtualPtr);
   } else if (physicalMemoryType == PhysicalMemoryType::MEM1) {
      view = platform::mapViewOfFile(mMem1, protectFlags, physicalAddress - MEM1BaseAddress, size, virtualPtr);
   } else if (physicalMemoryType == PhysicalMemoryType::MEM2) {
      view = platform::mapViewOfFile(mMem2, protectFlags, physicalAddress - MEM2BaseAddress, size, virtualPtr);
   } else if (physicalMemoryType == PhysicalMemoryType::LockedCache) {
      view = platform::mapViewOfFile(mLockedCache, protectFlags, physicalAddress - LCBaseAddress, size, virtualPtr);
   } else if (physicalMemoryType == PhysicalMemoryType::SRAM0) {
      view = platform::mapViewOfFile(mSram0, protectFlags, physicalAddress - SRAM0BaseAddress, size, virtualPtr);
   } else if (physicalMemoryType == PhysicalMemoryType::SRAM1) {
      view = platform::mapViewOfFile(mSram1, protectFlags, physicalAddress - SRAM1BaseAddress, size, virtualPtr);
   } else {
      gLog->error("Invalid physicalMemoryType {} for mapMemory", static_cast<int>(physicalMemoryType));
      return false;
   }

   // Add to the memory map
   auto virtualMemoryMap = VirtualMemoryMap {};
   virtualMemoryMap.virtualAddress = virtualAddress;
   virtualMemoryMap.physicalAddress = physicalAddress;
   virtualMemoryMap.size = size;
   virtualMemoryMap.permission = permission;

   mMappedMemory.insert(std::upper_bound(mMappedMemory.begin(),
                                         mMappedMemory.end(),
                                         virtualMemoryMap,
                                         [](const auto &m1, const auto &m2) {
                                            return m1.virtualAddress < m2.virtualAddress;
                                         }),
                        virtualMemoryMap);

   if (view != virtualPtr) {
      gLog->error("Unable to map virtual address 0x{:08X} to physical address 0x{:08X}",
                  virtualAddress.getAddress(), physicalAddress.getAddress());
      unmapMemory(virtualAddress, size);
      return false;
   }

   return true;
}


bool
MemoryMap::unmapMemory(VirtualAddress virtualAddress,
                       uint32_t size)
{
   std::vector<VirtualMemoryMap> remaps;
   auto start = align_up(virtualAddress, cpu::PageSize);
   auto end = start + (align_up(size, cpu::PageSize) - 1);

   for (auto itr = mMappedMemory.begin(); itr != mMappedMemory.end(); ) {
      auto mapStart = itr->virtualAddress;
      auto mapSize = itr->size;
      auto mapEnd = itr->virtualAddress + (itr->size - 1);
      auto remap = VirtualMemoryMap { };

      if (mapStart < start && mapEnd > start) {
         // End of map goes into unmap range
         remap.physicalAddress = itr->physicalAddress;
         remap.virtualAddress = itr->virtualAddress;
         remap.size = static_cast<uint32_t>(start - mapStart);
         remap.permission = itr->permission;
         remaps.push_back(remap);
      } else if (mapStart < end && mapEnd > end) {
         // Start of map is in unmap range
         auto offset = end - mapStart;
         remap.physicalAddress = itr->physicalAddress + offset;
         remap.virtualAddress = itr->virtualAddress + offset;
         remap.size = static_cast<uint32_t>(remap.size - offset);
         remap.permission = itr->permission;
         remaps.push_back(remap);
      } else if (mapStart > end || mapEnd < start) {
         // Not in unmap range
         ++itr;
         continue;
      }

      itr = mMappedMemory.erase(itr);

      if (!platform::unmapViewOfFile(getVirtualPointer(mapStart), mapSize)) {
         gLog->error("Unexpected error whilst unmapping virtual address 0x{:08X}",
                     virtualAddress.getAddress());
      }
   }

   for (auto &remap : remaps) {
      if (!mapMemory(remap.virtualAddress,
                     remap.physicalAddress,
                     remap.size,
                     remap.permission)) {
         gLog->error("Unexpected error whilst remapping virtual address 0x{:08X}",
                     remap.virtualAddress.getAddress());
      }
   }

   return true;
}


bool
MemoryMap::acquireReservation(VirtualReservation reservation)
{
   return platform::reserveMemory(mVirtualBase + reservation.start.getAddress(),
                                  (reservation.end - reservation.start) + 1);
}


bool
MemoryMap::releaseReservation(VirtualReservation reservation)
{
   return platform::freeMemory(mVirtualBase + reservation.start.getAddress(),
                               (reservation.end - reservation.start) + 1);
}


uintptr_t
MemoryMap::reserveBaseAddress()
{
   for (auto n = 32; n < 64; n++) {
      auto baseAddress = 1ull << n;

      if (platform::reserveMemory(baseAddress, 0x100000000ull)) {
         return static_cast<uintptr_t>(baseAddress);
      }
   }

   return 0;
}


void *
MemoryMap::getPhysicalPointer(PhysicalAddress physicalAddress)
{
   return reinterpret_cast<void *>(mPhysicalBase + physicalAddress.getAddress());
}


void *
MemoryMap::getVirtualPointer(VirtualAddress virtualAddress)
{
   return reinterpret_cast<void *>(mVirtualBase + virtualAddress.getAddress());
}

} // namespace cpu
