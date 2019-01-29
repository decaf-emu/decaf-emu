#include "memtrack.h"

#ifdef PLATFORM_WINDOWS

#include "cpu_config.h"
#include "cpu_internal.h"
#include "mmu.h"

#include <common/datahash.h>
#include <common/platform.h>
#include <common/rangecombiner.h>
#include <unordered_map>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace cpu
{

static constexpr uint64_t PhysTrackSetBit = 0x8000000000000000;
static constexpr uint64_t PhysIsMappedBit = 0x4000000000000000;
static constexpr uint32_t VirtTrackSetBit = 0x80000000;
static constexpr uint32_t VirtIsMappedBit = 0x40000000;

struct MappedArea
{
   cpu::VirtualAddress virtAddr;
   cpu::PhysicalAddress physAddr;
   uint32_t size;
};

static uintptr_t sPhysBaseAddress = 0;
static uintptr_t sVirtBaseAddress = 0;
static uint64_t sPageSizeBits = 0;
static std::atomic<uint32_t> *sVirtLookup = nullptr;
static std::atomic<uint64_t> *sTrackCount = nullptr;
static std::vector<MappedArea> sVirtMap;
namespace internal
{

LONG writeExceptionHandler(_EXCEPTION_POINTERS *ExceptionInfo)
{
   auto &ExceptionRecord = ExceptionInfo->ExceptionRecord;
   if (UNLIKELY(ExceptionRecord->ExceptionCode != EXCEPTION_ACCESS_VIOLATION)) {
      return EXCEPTION_CONTINUE_SEARCH;
   }

   // We do not check to see if sTrackCount is initialized here because we can assume
   // that it must be initialized if the vectored exeception handler was registered.

   // We do not verify that the SET bit is set for physical or virtual since another
   // thread may be racing us.  Instead we only check that the region was mapped at
   // least once (via the IsMapped bits), and if its been mapped once we can assume
   // that it was meant to be set up.  We also allow VirtualProtect to return an old
   // protection of READ _OR_ READWRITE for the same reasons.

   auto MemoryAddress = ExceptionRecord->ExceptionInformation[1];
   if (MemoryAddress >= sVirtBaseAddress && MemoryAddress < sVirtBaseAddress + 0x100000000) {
      // This is a virtual address, we need to do a lookup first.
      auto lookupIdx = (MemoryAddress - sVirtBaseAddress) >> sPageSizeBits;

      auto oldLookupValue = sVirtLookup[lookupIdx].fetch_and(~VirtTrackSetBit);
      if (!(oldLookupValue & VirtIsMappedBit)) {
         // This was an unmapped range to begin with.
         return EXCEPTION_CONTINUE_SEARCH;
      }

      // Figure out which page we belong to.
      auto trackIdx = oldLookupValue & ~(VirtTrackSetBit | VirtIsMappedBit);

      // Increment the counter to mark it as having changed
      sTrackCount[trackIdx].fetch_add(1);
   } else if (MemoryAddress >= sPhysBaseAddress && MemoryAddress < sPhysBaseAddress + 0x100000000) {
      // This is a physical address, we can directly convert
      auto trackIdx = static_cast<uint32_t>((MemoryAddress - sPhysBaseAddress) >> sPageSizeBits);

      // Then we remove the tracking bit
      auto oldTrackValue = sTrackCount[trackIdx].fetch_and(~PhysTrackSetBit);
      if (!(oldTrackValue & PhysIsMappedBit)) {
         // There was no protection on this area.  Something else has gone wrong.
         return EXCEPTION_CONTINUE_SEARCH;
      }

      // Increment the counter to mark it as having changed
      sTrackCount[trackIdx].fetch_add(1);
   } else {
      return EXCEPTION_CONTINUE_SEARCH;
   }

   // Finally we reprotect the memory to its normal state
   DWORD oldProtection;
   VirtualProtect(reinterpret_cast<void*>(MemoryAddress), 1, PAGE_READWRITE, &oldProtection);
   if (oldProtection != PAGE_READONLY && oldProtection != PAGE_READWRITE) {
      VirtualProtect(reinterpret_cast<void*>(MemoryAddress), 1, oldProtection, nullptr);
      return EXCEPTION_CONTINUE_SEARCH;
   }

   return EXCEPTION_CONTINUE_EXECUTION;
}

void
initialiseMemtrack()
{
   if (!config()->memory.writeTrackEnabled) {
      return;
   }

   SYSTEM_INFO sysInfo;
   GetSystemInfo(&sysInfo);

   auto pageSize = sysInfo.dwPageSize;
   auto pageSizeBits = 0;
   auto i = pageSize;
   while (i >>= 1) pageSizeBits++;

   sPhysBaseAddress = cpu::getBasePhysicalAddress();
   sVirtBaseAddress = cpu::getBaseVirtualAddress();
   sPageSizeBits = pageSizeBits;

   auto numtrackTableEntries = 0x100000000 >> pageSizeBits;

   // Initialise the lookup table to point everything to 0xFFFFFFFF (invalid)
   sVirtLookup = new std::atomic<uint32_t>[numtrackTableEntries];
   memset(sVirtLookup, 0x00, numtrackTableEntries * sizeof(uint32_t));

   // Initialise the tracking table to all 0's
   sTrackCount = new std::atomic<uint64_t>[numtrackTableEntries];
   memset(sTrackCount, 0x00, numtrackTableEntries * sizeof(uint64_t));

   // Install the exception handler
   AddVectoredExceptionHandler(1, writeExceptionHandler);
}

void
registerTrackedRange(VirtualAddress virtualAddress,
                     PhysicalAddress physicalAddress,
                     uint32_t size)
{
   if (!sTrackCount) {
      return;
   }

   if (size == 0) {
      return;
   }

   // We have to remove any conflicting virtual mappings before we can proceed.  Note
   // that this is technically an incorrect operation, as they may not be overlaying on
   // eachother perfectly, but its challenging to handle this correctly, and this should
   // work for the immediate future.
   // TODO: Correctly unmap regions from the memory tracker.
   for (auto iter = sVirtMap.begin(); iter != sVirtMap.end(); ++iter) {
      if (virtualAddress >= iter->virtAddr && virtualAddress < iter->virtAddr + iter->size) {
         iter = sVirtMap.erase(iter);
      }
   }

   // Add an entry to the mappings list
   sVirtMap.push_back({ virtualAddress, physicalAddress, size });

   // Apply the neccessary changes to the tracking tables
   auto firstPhysPage = physicalAddress.getAddress() >> sPageSizeBits;
   auto firstPage = virtualAddress.getAddress() >> sPageSizeBits;
   auto lastPage = (virtualAddress.getAddress() + (size - 1)) >> sPageSizeBits;

   for (auto pageIdx = firstPage, physPageIdx = firstPhysPage;
        pageIdx <= lastPage;
        ++pageIdx, ++physPageIdx)
   {
      auto oldPhysPage = sVirtLookup[pageIdx].exchange(VirtIsMappedBit | physPageIdx);
      if (oldPhysPage & VirtIsMappedBit) {
         decaf_abort("write tracker attempted to register an already registered page");
      }

      // In order to avoid needing to go searching for all the pages that we
      // need to protect, we simply increment the tracking table to indicate
      // that any of the memory may have changed.  The next time the pages are
      // checked for changes, the correct protections will be applied.
      sTrackCount[physPageIdx].fetch_add(1);
   }
}

void
unregisterTrackedRange(VirtualAddress virtualAddress,
                       uint32_t size)
{
   if (!sTrackCount) {
      return;
   }

   if (size == 0) {
      return;
   }

   // Remove the entry from the mappings list
   for (auto iter = sVirtMap.begin(); iter != sVirtMap.end(); ++iter) {
      if (iter->virtAddr == virtualAddress && iter->size == size) {
         sVirtMap.erase(iter);
         break;
      }
   }

   // Apply the neccessary changes to the tracking tables
   auto firstPage = virtualAddress.getAddress() >> sPageSizeBits;
   auto lastPage = firstPage + ((size - 1) >> sPageSizeBits);

   for (auto pageIdx = firstPage; pageIdx <= lastPage; ++pageIdx) {
      auto oldPhysPage = sVirtLookup[pageIdx].exchange(0x00000000);
      if (!(oldPhysPage & VirtIsMappedBit)) {
         decaf_abort("write tracker attempted to unregister an already unregister page");
      }
   }
}

void
clearTrackedRanges()
{
   if (!sTrackCount) {
      return;
   }

   // Resetting the tracked ranges is as simple as clearing the lookup table
   // we are using to translate virtual addresses to physical pages.
   auto numtrackTableEntries = 0x100000000 >> sPageSizeBits;
   memset(sVirtLookup, 0x00, numtrackTableEntries * sizeof(uint32_t));
}

} // namespace internal

MemtrackState
getMemoryState(PhysicalAddress physicalAddress,
               uint32_t size)
{
   if (!sTrackCount) {
      // If the write tracking system is not enabled, we simply hash.
      auto physPtr = reinterpret_cast<void *>(cpu::getBasePhysicalAddress() + physicalAddress.getAddress());
      auto hashVal = DataHash {}.write(physPtr, size);
      return MemtrackState { hashVal.value() };
   }

   if (size == 0) {
      return MemtrackState { 0 };
   }

   // Calculate the actual return value
   uint64_t pageIndexTotal = 0;

   {
      uintptr_t startAddr = physicalAddress.getAddress();
      uintptr_t endAddr = startAddr + (size - 1);

      auto startPage = startAddr >> sPageSizeBits;
      auto lastPage = endAddr >> sPageSizeBits;

      for (auto i = startPage; i <= lastPage; ++i) {
         auto pageData = sTrackCount[i].load();
         auto pageCount = pageData & ~(PhysTrackSetBit);
         pageIndexTotal += pageCount;
      }
   }

   // Write-protect all these regions for the future
   for (auto &area : sVirtMap) {
      if (physicalAddress < area.physAddr || physicalAddress >= area.physAddr + area.size) {
         continue;
      }

      auto virtualAddress = area.virtAddr + (physicalAddress - area.physAddr);

      uintptr_t startAddr = virtualAddress.getAddress();
      uintptr_t endAddr = startAddr + (size - 1);

      auto startPage = startAddr >> sPageSizeBits;
      auto lastPage = endAddr >> sPageSizeBits;

      auto pagePtr = reinterpret_cast<uint8_t*>(sVirtBaseAddress + (startPage << sPageSizeBits));
      auto pageSize = 1 << sPageSizeBits;

      auto protectCombiner = makeRangeCombiner<void*, uint8_t*, uint64_t>(
         [=](void*, uint8_t* pagePtr, uint64_t pageSize)
         {
            DWORD oldProtection;
            VirtualProtect(pagePtr, pageSize, PAGE_READONLY, &oldProtection);
            if (oldProtection != PAGE_READONLY && oldProtection != PAGE_READWRITE) {
               decaf_abort("Attempted to write-track a weird page");
            }
         });

      for (auto i = startPage; i <= lastPage; ++i) {
         auto oldTrackValue = sVirtLookup[i].fetch_or(VirtTrackSetBit | VirtIsMappedBit);
         if (!(oldTrackValue & VirtIsMappedBit)) {
            decaf_abort("Attempted to write-track unmapped memory");
         }

         if (!(oldTrackValue & VirtTrackSetBit)) {
            // If we weren't previous tracking this memory, we need to add it.
            protectCombiner.push(nullptr, pagePtr, pageSize);
         }

         pagePtr += pageSize;
      }

      protectCombiner.flush();
   }

   return MemtrackState { pageIndexTotal };
}

} // namespace cpu

#endif // PLATFORM_WINDOWS
