#include "kernel_enum.h"
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

struct PhysicalMemoryMap
{
   cpu::PhysicalAddress start;
   cpu::PhysicalAddress end;
   PhysicalRegion id;

   uint32_t size() const
   {
      return static_cast<uint32_t>(end - start + 1);
   }
};

struct VirtualMemoryMap
{
   cpu::VirtualAddress start;
   cpu::VirtualAddress end;
   VirtualRegion id;
   PhysicalRegion physicalRegion;
   bool mapped = false;

   uint32_t size() const
   {
      return static_cast<uint32_t>(end - start + 1);
   }
};

static cpu::PhysicalAddress operator "" _paddr(unsigned long long int x)
{
   return cpu::PhysicalAddress { static_cast<uint32_t>(x) };
}

static cpu::VirtualAddress operator "" _vaddr(unsigned long long int x)
{
   return cpu::VirtualAddress { static_cast<uint32_t>(x) };
}

static PhysicalMemoryMap
sPhysicalMemoryMap[] = {
   // MEM1 - 32 MB
   { 0x00000000_paddr , 0x01FFFFFF_paddr, PhysicalRegion::MEM1 },

   // LockedCache (not present on hardware) - 16 KB per core
   // Note we have to use the page size, 128kb, as that is the minimum we can map.
   { 0x02000000_paddr, 0x0201FFFF_paddr, PhysicalRegion::LockedCache },

   // MEM0 - 2.68 MB / 2,752 KB
   //0x08000000   --   0x0811FFFF = unused
   { 0x08000000_paddr, 0x082DFFFF_paddr, PhysicalRegion::MEM0 },
   { 0x08120000_paddr, 0x081BFFFF_paddr, PhysicalRegion::MEM0IosKernel },         // 640 KB
   { 0x081C0000_paddr, 0x0827FFFF_paddr, PhysicalRegion::MEM0IosMcp },            // 768 KB
   { 0x08280000_paddr, 0x082AFFFF_paddr, PhysicalRegion::MEM0IosCrypto },         // 192 KB
   //0x082C0000   --   0x082DFFFF = unused

   // MMIO / Registers
   //  0x0C000000  --  0x0C?????? = unused
   //  0x0D000000  --  0x0D?????? = unused

   // MEM2 - 2 GB
   { 0x10000000_paddr, 0x8FFFFFFF_paddr, PhysicalRegion::MEM2 },
   { 0x10000000_paddr, 0x13FFFFFF_paddr, PhysicalRegion::MEM2IosHeap },           // 64 MB
   { 0x14000000_paddr, 0x17FFFFFF_paddr, PhysicalRegion::MEM2ForegroundBucket },  // 64 MB
   { 0x18000000_paddr, 0x1AFFFFFF_paddr, PhysicalRegion::MEM2SharedData },        // 48 MB
   { 0x1B000000_paddr, 0x1FFFFFFF_paddr, PhysicalRegion::MEM2LoaderHeap },        // 80 MB
   { 0x20000000_paddr, 0x2FFFFFFF_paddr, PhysicalRegion::MEM2TilingApertures },   // 256 MB
   //0x30000000   --   0x31FFFFFF = unused { root.rpx }
   { 0x32000000_paddr, 0x327FFFFF_paddr, PhysicalRegion::MEM2SystemHeap },        // 8 MB
   //0x32800000   --   0x32ffffff = unused { unknown }
   //0x33000000   --   0x33FFFFFF = unused { error display }
   { 0x34000000_paddr, 0x4FFFFFFF_paddr, PhysicalRegion::MEM2OverlayArena },      // 448 MB
   { 0x50000000_paddr, 0x8FFFFFFF_paddr, PhysicalRegion::MEM2AppHeap },           // 1 GB

   // SRAM1 - 32 KB
   { 0xFFF00000_paddr, 0xFFF07FFF_paddr, PhysicalRegion::SRAM1 },
   { 0xFFF00000_paddr, 0xFFF07FFF_paddr, PhysicalRegion::SRAM1C2W },              // 32 KB

   // SRAM0 - 64 KB
   { 0xFFFF0000_paddr, 0xFFFFFFFF_paddr, PhysicalRegion::SRAM0 },
   { 0xFFFF0000_paddr, 0xFFFFFFFF_paddr, PhysicalRegion::SRAM0IosKernel },        // 64 KB
};

static VirtualMemoryMap
sVirtualMemoryMap[] = {
   { 0x01000000_vaddr, 0x017FFFFF_vaddr, VirtualRegion::SystemHeap,         PhysicalRegion::MEM2SystemHeap },         // 8 MB
   //0x01800000   --   0x01FFFFFF = unused
   { 0x02000000_vaddr, 0x0FFFFFFF_vaddr, VirtualRegion::AppHeapCode,        PhysicalRegion::MEM2AppHeap },            // AppHeap 1GB
   { 0x10000000_vaddr, 0x4FFFFFFF_vaddr, VirtualRegion::AppHeapData,        PhysicalRegion::MEM2AppHeap },
   //0x50000000   --   0x5FFFFFFF = unused
   { 0x60000000_vaddr, 0x7BFFFFFF_vaddr, VirtualRegion::OverlayArena,       PhysicalRegion::MEM2OverlayArena },       // 448 MB
   //0x7C000000   --   0x7FFFFFFF = unused
   { 0x80000000_vaddr, 0x8FFFFFFF_vaddr, VirtualRegion::TilingApertures,    PhysicalRegion::MEM2TilingApertures },    // 256 MB
   //0x90000000   --   0x9FFFFFFF = unused
   { 0xA0000000_vaddr, 0xDFFFFFFF_vaddr, VirtualRegion::VirtualMapRange,    PhysicalRegion::Invalid },                // 1024 MB
   { 0xE0000000_vaddr, 0xE3FFFFFF_vaddr, VirtualRegion::ForegroundBucket,   PhysicalRegion::MEM2ForegroundBucket },   // 64 MB
   //0xE4000000   --   0xE5FFFFFF = unused
   { 0xE6000000_vaddr, 0xEAFFFFFF_vaddr, VirtualRegion::LoaderHeap,         PhysicalRegion::MEM2LoaderHeap },         // 80 MB
   //0xEB000000   --   0xF3FFFFFF = unused
   { 0xF4000000_vaddr, 0xF5FFFFFF_vaddr, VirtualRegion::MEM1,               PhysicalRegion::MEM1 },                   // 32 MB
   //0xF6000000   --   0xF7FFFFFF = unused
   { 0xF8000000_vaddr, 0xFAFFFFFF_vaddr, VirtualRegion::SharedData,         PhysicalRegion::MEM2SharedData },         // 48 MB
   //0xFB000000   --   0xFFBFFFFF = unused
   { 0xFFC00000_vaddr, 0xFFC1FFFF_vaddr, VirtualRegion::LockedCache,        PhysicalRegion::LockedCache },            // 128 KB
};

// Maybe this is 1 page per core?
// We're using the results from a Mii Maker rpx loading environment.
static auto sAvailPhysicalBase = cpu::PhysicalAddress { 0 };
static const auto AvailPhysicalSize = uint32_t { 3 * 128 * 1024 };

static bool
map(VirtualMemoryMap &map)
{
   auto &physicalRegion = sPhysicalMemoryMap[map.physicalRegion];
   auto physSize = static_cast<uint32_t>(physicalRegion.end - physicalRegion.start + 1);
   auto size = static_cast<uint32_t>(map.end - map.start + 1);
   decaf_check(physSize == size);

   if (!map.mapped) {
      if (!cpu::allocateVirtualAddress(map.start, size)) {
         gLog->error("Unexpected failure allocating virtual address 0x{:08X} - 0x{:08X}",
                     map.start.getAddress(), map.end.getAddress());
         return false;
      }

      if (!cpu::mapMemory(map.start, physicalRegion.start, size, cpu::MapPermission::ReadWrite)) {
         gLog->error("Unexpected failure allocating mapping virtual address 0x{:08X} to physical address 0x{:08X}",
                     map.start.getAddress(), physicalRegion.start.getAddress());
         return false;
      }

      map.mapped = true;
   }

   return true;
}

static void
unmap(VirtualMemoryMap &map)
{
   if (map.mapped) {
      auto size = static_cast<uint32_t>(map.end - map.start + 1);
      if (!cpu::unmapMemory(map.start, size)) {
         gLog->error("Unexpected failure unmapping virtual address 0x{:08X} - 0x{:08X}",
                     map.start.getAddress(), map.end.getAddress());
      }

      if (!cpu::freeVirtualAddress(map.start, size)) {
         gLog->error("Unexpected failure freeing virtual address 0x{:08X} - 0x{:08X}",
                     map.start.getAddress(), map.end.getAddress());
      }

      map.mapped = false;
   }
}

void
initialiseVirtualMemory()
{
   map(sVirtualMemoryMap[VirtualRegion::SystemHeap]);
   map(sVirtualMemoryMap[VirtualRegion::ForegroundBucket]);
   map(sVirtualMemoryMap[VirtualRegion::MEM1]);
   map(sVirtualMemoryMap[VirtualRegion::SharedData]);
   map(sVirtualMemoryMap[VirtualRegion::LockedCache]);
}

void
freeVirtualMemory()
{
   unmap(sVirtualMemoryMap[VirtualRegion::SystemHeap]);
   unmap(sVirtualMemoryMap[VirtualRegion::ForegroundBucket]);
   unmap(sVirtualMemoryMap[VirtualRegion::MEM1]);
   unmap(sVirtualMemoryMap[VirtualRegion::SharedData]);
   unmap(sVirtualMemoryMap[VirtualRegion::LockedCache]);

   freeLoaderMemory();
   freeOverlayArena();
   freeTilingApertures();
   freeAppMemory();
}

cpu::VirtualAddressRange
initialiseLoaderMemory()
{
   auto &region = sVirtualMemoryMap[VirtualRegion::LoaderHeap];
   map(region);
   return { region.start, region.end };
}

void
freeLoaderMemory()
{
   unmap(sVirtualMemoryMap[VirtualRegion::LoaderHeap]);
}

cpu::VirtualAddressRange
initialiseOverlayArena()
{
   auto &region = sVirtualMemoryMap[VirtualRegion::OverlayArena];
   map(region);
   return { region.start, region.end };
}

void
freeOverlayArena()
{
   unmap(sVirtualMemoryMap[VirtualRegion::OverlayArena]);
}

cpu::VirtualAddressRange
initialiseTilingApertures()
{
   auto &region = sVirtualMemoryMap[VirtualRegion::TilingApertures];
   map(region);
   return { region.start, region.end };
}

void
freeTilingApertures()
{
   unmap(sVirtualMemoryMap[VirtualRegion::TilingApertures]);
}

bool
initialiseAppMemory(uint32_t codeSize)
{
   auto &appHeapCode = sVirtualMemoryMap[VirtualRegion::AppHeapCode];
   auto &appHeapData = sVirtualMemoryMap[VirtualRegion::AppHeapData];
   auto &appHeap = sPhysicalMemoryMap[PhysicalRegion::MEM2AppHeap];

   // Align code size to page size
   codeSize = align_up(codeSize, cpu::PageSize);
   auto dataSize = appHeap.size() - codeSize - AvailPhysicalSize;

   appHeapData.end = appHeapData.start + dataSize - 1;
   appHeapCode.end = appHeapCode.start + codeSize - 1;

   // Place data at start of physical memory
   auto physDataStart = appHeap.start;

   // Place code at end of physical memory
   sAvailPhysicalBase = appHeap.start + dataSize;
   auto physCodeStart = sAvailPhysicalBase + AvailPhysicalSize;

   if (!cpu::allocateVirtualAddress(appHeapCode.start, codeSize)) {
      gLog->error("Unexpected failure allocating code virtual address 0x{:08X} - 0x{:08X}",
                  appHeapCode.start.getAddress(), appHeapCode.end.getAddress());
      return false;
   }

   if (!cpu::allocateVirtualAddress(appHeapData.start, dataSize)) {
      gLog->error("Unexpected failure allocating data virtual address 0x{:08X} - 0x{:08X}",
                  appHeapData.start.getAddress(), appHeapData.end.getAddress());
      return false;
   }

   if (!cpu::mapMemory(appHeapCode.start,
                       physCodeStart,
                       codeSize,
                       cpu::MapPermission::ReadWrite)) {
      gLog->error("Unexpected failure mapping data virtual address 0x{:08X} - 0x{:08X} to physical address 0x{:08X}",
                  appHeapCode.start.getAddress(), appHeapCode.end.getAddress(), physCodeStart.getAddress());
      return false;
   }

   appHeapCode.mapped = true;

   if (!cpu::mapMemory(appHeapData.start,
                       physDataStart,
                       dataSize,
                       cpu::MapPermission::ReadWrite)) {
      gLog->error("Unexpected failure mapping data virtual address 0x{:08X} - 0x{:08X} to physical address 0x{:08X}",
                  appHeapData.start.getAddress(), appHeapData.end.getAddress(), physDataStart.getAddress());
      return false;
   }

   appHeapData.mapped = true;
   return true;
}

void
freeAppMemory()
{
   auto &appHeapCode = sVirtualMemoryMap[VirtualRegion::AppHeapCode];
   auto &appHeapData = sVirtualMemoryMap[VirtualRegion::AppHeapData];

   if (appHeapCode.mapped) {
      cpu::unmapMemory(appHeapCode.start, appHeapCode.size());
      cpu::freeVirtualAddress(appHeapCode.start, appHeapCode.size());
      appHeapCode.mapped = false;
   }

   if (appHeapData.mapped) {
      cpu::unmapMemory(appHeapData.start, appHeapData.size());
      cpu::freeVirtualAddress(appHeapData.start, appHeapData.size());
      appHeapData.mapped = false;
   }
}

cpu::VirtualAddressRange
getVirtualRange(VirtualRegion region)
{
   return { sVirtualMemoryMap[region].start, sVirtualMemoryMap[region].end };
}

cpu::PhysicalAddressRange
getPhysicalRange(PhysicalRegion region)
{
   return { sPhysicalMemoryMap[region].start, sPhysicalMemoryMap[region].end };
}

cpu::PhysicalAddressRange
getAvailPhysicalRange()
{
   return { sAvailPhysicalBase, AvailPhysicalSize };
}

cpu::PhysicalAddressRange
getDataPhysicalRange()
{
   return { sPhysicalMemoryMap[PhysicalRegion::MEM2AppHeap].start,
            sVirtualMemoryMap[VirtualRegion::AppHeapData].size() };
}

} // namespace kernel
