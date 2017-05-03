#include "kernel_memory.h"

#include <algorithm>
#include <common/align.h>
#include <common/decaf_assert.h>
#include <common/log.h>
#include <libcpu/address.h>
#include <libcpu/mem.h>
#include <libcpu/mmu.h>
#include <libcpu/pointer.h>
#include <vector>

namespace kernel
{

struct MemoryMap
{
   cpu::VirtualAddress virt;
   cpu::PhysicalAddress phys;
   uint32_t size;
   bool mapped = false;
};

/*
 * Physical Address Space
 * 0x00000000 - 0x01ffffff = MEM1 (32 MB)
 *
 * 0x08000000 - 0x0811FFFF = unused { MEM0 }
 * 0x0C000000 - 0x0C?????? = unused { registers }
 * 0x0D000000 - 0x0D?????? = unused { registers }
 *
 * 0x10000000 - 0x1001ffff = LockedCacheCore (only 16 KB per core, but 128KB is page size)
 * 0x10020000 - 0x13ffffff = unused { ios data }
 * 0x14000000 - 0x17ffffff = ForegroundBucket (64 MB)
 * 0x18000000 - 0x1affffff = SharedData (48 MB)
 * 0x1b000000 - 0x1fffffff = LoaderHeap (80 MB)
 * 0x20000000 - 0x2FFFFFFF = TilingApertures (256 MB)
 * 0x30000000 - 0x31FFFFFF = unused { root.rpx }
 * 0x32000000 - 0x327FFFFF = SystemHeap (8 MB)
 * 0x32800000 - 0x32ffffff = unused { unknown }
 * 0x33000000 - 0x33FFFFFF = unused { error display }
 * 0x34000000 - 0x4fffffff = OverlayArena (448 MB)
 * 0x50000000 - 0x8FFFFFFF = AppHeap (1 GB)
 */

/*
 * Virtual Address Space
 * 0x01000000 - 0x017FFFFF = SystemHeap
 *
 * 0x02000000 - 0x0FFFFFFF = AppHeap.code
 * 0x10000000 - 0x4FFFFFFF = AppHeap.data
 *
 * 0x60000000 - 0x7BFFFFFF = OverlayArena
 *
 * 0x80000000 - 0x8FFFFFFF = TilingApertures
 *
 * 0xA0000000 - 0xDFFFFFFF = VirtualMapRange used by coreinit virtual mapping
 *
 * 0xE0000000 - 0xE3FFFFFF = ForegroundBucket
 *
 * 0xE6000000 - 0xE9FFFFFF = LoaderHeap
 *
 * 0xF4000000 - 0xF5FFFFFF = MEM1
 *
 * 0xF8000000 - 0xFAFFFFFF = SharedData
 *
 * 0xFFC00000 - 0xFFC20000 = Locked Cache
 */

// cute.
static constexpr uint32_t
operator""_kb(unsigned long long value)
{
   return static_cast<uint32_t>(value * 1024);
}

static constexpr uint32_t
operator""_mb(unsigned long long value)
{
   return static_cast<uint32_t>(value * 1024 * 1024);
}

static auto SystemHeap = MemoryMap {
   cpu::VirtualAddress { 0x01000000 },
   cpu::PhysicalAddress { 0x32000000 },
   8_mb
};

static auto ForegroundBucket = MemoryMap {
   cpu::VirtualAddress { 0xE0000000 },
   cpu::PhysicalAddress { 0x14000000 },
   64_mb
};

static auto Mem1 = MemoryMap {
   cpu::VirtualAddress { 0xF4000000 },
   cpu::PhysicalAddress { 0 },
   32_mb
};

static auto SharedData = MemoryMap {
   cpu::VirtualAddress { 0xF8000000 },
   cpu::PhysicalAddress { 0x18000000 },
   48_mb
};

static auto LoaderHeap = MemoryMap {
   cpu::VirtualAddress { 0xE6000000 },
   cpu::PhysicalAddress { 0x1B000000 },
   80_mb
};

static auto OverlayArena = MemoryMap {
   cpu::VirtualAddress { 0xA0000000 },
   cpu::PhysicalAddress { 0x34000000 },
   448_mb
};

static auto LockedCache = MemoryMap {
   cpu::VirtualAddress { 0xFFC00000 },
   cpu::PhysicalAddress { 0x10000000 },
   cpu::PageSize
};

static auto TilingApertures = MemoryMap {
   cpu::VirtualAddress { 0x80000000 },
   cpu::PhysicalAddress { 0x20000000 },
   256_mb
};

static auto VirtualMapRange = MemoryMap {
   cpu::VirtualAddress { 0xA0000000 },
   cpu::PhysicalAddress { 0 },
   1024_mb
};

static const auto AppHeapPhysicalBase = cpu::PhysicalAddress { 0x50000000 };
static const auto AppHeapPhysicalSize = 1024_mb;
static const auto CodeVirtualBase = cpu::VirtualAddress { 0x02000000 };
static const auto DataVirtualBase = cpu::VirtualAddress { 0x10000000 };
static auto sCodeSize = uint32_t { 0 };
static auto sDataSize = uint32_t { 0 };

// Maybe this is 1 page per core? cpu::PageSize = 128kb.
// We're using the results from a Mii Maker rpx loading environment.
static auto sAvailPhysicalBase = cpu::PhysicalAddress { 0 };
static const auto AvailPhysicalSize = uint32_t { 128_kb * 3 };

static bool
map(MemoryMap &map)
{
   if (!map.mapped) {
      if (!cpu::allocateVirtualAddress(map.virt, map.size)) {
         gLog->error("Unexpected failure allocating virtual address 0x{:08X} - 0x{:08X}",
                     map.virt.getAddress(), map.virt.getAddress() + map.size);
         return false;
      }

      if (!cpu::mapMemory(map.virt, map.phys, map.size, cpu::MapPermission::ReadWrite)) {
         gLog->error("Unexpected failure allocating mapping virtual address 0x{:08X} to physical address 0x{:08X}",
                     map.virt.getAddress(), map.phys.getAddress());
         return false;
      }

      map.mapped = true;
   }

   return true;
}

static void
unmap(MemoryMap &map)
{
   if (map.mapped) {
      if (!cpu::unmapMemory(map.virt, map.size)) {
         gLog->error("Unexpected failure unmapping virtual address 0x{:08X} - 0x{:08X}",
                     map.virt.getAddress(), map.virt.getAddress() + map.size);
      }

      if (!cpu::freeVirtualAddress(map.virt, map.size)) {
         gLog->error("Unexpected failure freeing virtual address 0x{:08X} - 0x{:08X}",
                     map.virt.getAddress(), map.virt.getAddress() + map.size);
      }

      map.mapped = false;
   }
}

void
initialiseVirtualMemory()
{
   map(SystemHeap);
   map(ForegroundBucket);
   map(Mem1);
   map(SharedData);
   map(LockedCache);
}

void
freeVirtualMemory()
{
   unmap(SystemHeap);
   unmap(ForegroundBucket);
   unmap(Mem1);
   unmap(SharedData);
   unmap(LockedCache);

   freeLoaderMemory();
   freeOverlayArena();
   freeTilingApertures();
   freeAppMemory();
}

cpu::VirtualAddressRange
initialiseLoaderMemory()
{
   map(LoaderHeap);
   return { LoaderHeap.virt, LoaderHeap.size };
}

void
freeLoaderMemory()
{
   unmap(LoaderHeap);
}

cpu::VirtualAddressRange
initialiseOverlayArena()
{
   map(OverlayArena);
   return { OverlayArena.virt, OverlayArena.size };
}

void
freeOverlayArena()
{
   unmap(OverlayArena);
}

cpu::VirtualAddressRange
initialiseTilingApertures()
{
   map(TilingApertures);
   return { TilingApertures.virt, TilingApertures.size };
}

void
freeTilingApertures()
{
   unmap(TilingApertures);
}

bool
initialiseAppMemory(uint32_t codeSize)
{
   // Align code size to page size
   sCodeSize = align_up(codeSize, cpu::PageSize);
   sDataSize = AppHeapPhysicalSize - sCodeSize - AvailPhysicalSize;

   // Place data at start of physical memory
   auto physDataStart = AppHeapPhysicalBase;

   // Place code at end of physical memory
   sAvailPhysicalBase = AppHeapPhysicalBase + sDataSize;
   auto physCodeStart = sAvailPhysicalBase + AvailPhysicalSize;


   if (!cpu::allocateVirtualAddress(CodeVirtualBase, sCodeSize)) {
      gLog->error("Unexpected failure allocating code virtual address 0x{:08X} - 0x{:08X}",
                  CodeVirtualBase.getAddress(), CodeVirtualBase.getAddress() + sCodeSize);
      return false;
   }

   if (!cpu::allocateVirtualAddress(DataVirtualBase, sDataSize)) {
      gLog->error("Unexpected failure allocating data virtual address 0x{:08X} - 0x{:08X}",
                  DataVirtualBase.getAddress(), DataVirtualBase.getAddress() + sDataSize);
      return false;
   }

   if (!cpu::mapMemory(CodeVirtualBase,
                       physCodeStart,
                       sCodeSize,
                       cpu::MapPermission::ReadWrite)) {
      gLog->error("Unexpected failure mapping data virtual address 0x{:08X} - 0x{:08X} to physical address 0x{:08X}",
                  CodeVirtualBase.getAddress(),
                  CodeVirtualBase.getAddress() + sCodeSize,
                  physCodeStart.getAddress());
      return false;
   }

   if (!cpu::mapMemory(DataVirtualBase,
                       physDataStart,
                       sDataSize,
                       cpu::MapPermission::ReadWrite)) {
      gLog->error("Unexpected failure mapping data virtual address 0x{:08X} - 0x{:08X} to physical address 0x{:08X}",
                  DataVirtualBase.getAddress(),
                  DataVirtualBase.getAddress() + sDataSize,
                  physDataStart.getAddress());
      return false;
   }

   return true;
}

void
freeAppMemory()
{
   if (sCodeSize) {
      cpu::unmapMemory(CodeVirtualBase, sCodeSize);
      cpu::freeVirtualAddress(CodeVirtualBase, sCodeSize);
      sCodeSize = 0;
   }

   if (sDataSize) {
      cpu::unmapMemory(DataVirtualBase, sDataSize);
      cpu::freeVirtualAddress(DataVirtualBase, sDataSize);
      sDataSize = 0;
   }
}

cpu::VirtualAddressRange
getCodeBounds()
{
   return { CodeVirtualBase, sCodeSize };
}

cpu::VirtualAddressRange
getForegroundBucketRange()
{
   return { ForegroundBucket.virt, ForegroundBucket.size };
}

cpu::VirtualAddressRange
getLockedCacheBounds()
{
   return { LockedCache.virt, LockedCache.size };
}

cpu::VirtualAddressRange
getMEM1Bound()
{
   return { Mem1.virt, Mem1.size };
}

cpu::VirtualAddressRange
getMEM2Bound()
{
   return { DataVirtualBase, sDataSize };
}

cpu::VirtualAddressRange
getSharedDataBounds()
{
   return { SharedData.virt, SharedData.size };
}

cpu::VirtualAddressRange
getSystemHeapBounds()
{
   return { SystemHeap.virt, SystemHeap.size };
}

cpu::VirtualAddressRange
getVirtualMapRange()
{
   return { VirtualMapRange.virt, VirtualMapRange.size };
}

cpu::PhysicalAddressRange
getAvailPhysicalRange()
{
   return { sAvailPhysicalBase, AvailPhysicalSize };
}

cpu::PhysicalAddressRange
getDataPhysicalRange()
{
   return { AppHeapPhysicalBase, sDataSize };
}

} // namespace kernel
