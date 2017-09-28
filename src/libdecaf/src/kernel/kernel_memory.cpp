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

struct PhysicalRange
{
   cpu::PhysicalAddress start;
   cpu::PhysicalAddress end;

   uint32_t size() const
   {
      return static_cast<uint32_t>(end - start + 1);
   }
};

struct VirtualMap
{
   cpu::VirtualAddress start;
   cpu::VirtualAddress end;
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

static std::map<PhysicalRegion, PhysicalRange>
sPhysicalMemoryMap {
   // MEM1 - 32 MB
   { PhysicalRegion::MEM1,                   { 0x00000000_paddr , 0x01FFFFFF_paddr } },

   // LockedCache (not present on hardware) - 16 KB per core
   // Note we have to use the page size, 128kb, as that is the minimum we can map.
   { PhysicalRegion::LockedCache,            { 0x02000000_paddr, 0x0201FFFF_paddr } },

   // MEM0 - 2.68 MB / 2,752 KB
   // unused                                   0x08000000   --   0x0811FFFF
   { PhysicalRegion::MEM0,                   { 0x08000000_paddr, 0x082DFFFF_paddr } },
   { PhysicalRegion::MEM0IosKernel,          { 0x08120000_paddr, 0x081BFFFF_paddr } },
   { PhysicalRegion::MEM0IosMcp,             { 0x081C0000_paddr, 0x0827FFFF_paddr } },
   { PhysicalRegion::MEM0IosCrypto,          { 0x08280000_paddr, 0x082BFFFF_paddr } },
   // IOS BSP unknown                          0x082C0000   --   0x082DFFFF

   // MMIO / Registers
   //  0x0C000000  --  0x0C??????
   //  0x0D000000  --  0x0D??????

   // MEM2 - 2 GB
   { PhysicalRegion::MEM2,                   { 0x10000000_paddr, 0x8FFFFFFF_paddr } },
   // IOS MCP unknown                          0x10000000   --   0x100FFFFF
   { PhysicalRegion::MEM2IosUsb,             { 0x10100000_paddr, 0x106FFFFF_paddr } },
   { PhysicalRegion::MEM2IosFs,              { 0x10700000_paddr, 0x11EFFFFF_paddr } },
   { PhysicalRegion::MEM2IosPad,             { 0x11F00000_paddr, 0x122FFFFF_paddr } },
   { PhysicalRegion::MEM2IosNet,             { 0x12300000_paddr, 0x128FFFFF_paddr } },
   { PhysicalRegion::MEM2IosAcp,             { 0x12900000_paddr, 0x12BBFFFF_paddr } },
   { PhysicalRegion::MEM2IosNsec,            { 0x12BC0000_paddr, 0x12EBFFFF_paddr } },
   { PhysicalRegion::MEM2IosNim,             { 0x12EC0000_paddr, 0x1363FFFF_paddr } },
   { PhysicalRegion::MEM2IosFpd,             { 0x13640000_paddr, 0x13A3FFFF_paddr } },
   { PhysicalRegion::MEM2IosTest,            { 0x13A40000_paddr, 0x13BFFFFF_paddr } },
   { PhysicalRegion::MEM2IosAuxil,           { 0x13C00000_paddr, 0x13CBFFFF_paddr } },
   { PhysicalRegion::MEM2IosBsp,             { 0x13CC0000_paddr, 0x13D7FFFF_paddr } },
   // IOS MCP unknown                          0x13D80000   --   0x13DBFFFF
   { PhysicalRegion::MEM2ForegroundBucket,   { 0x14000000_paddr, 0x17FFFFFF_paddr } },
   { PhysicalRegion::MEM2SharedData,         { 0x18000000_paddr, 0x1AFFFFFF_paddr } },
   { PhysicalRegion::MEM2LoaderHeap,         { 0x1B000000_paddr, 0x1CFFFFFF_paddr } },
   { PhysicalRegion::MEM2IosSharedHeap,      { 0x1D000000_paddr, 0x1FAFFFFF_paddr } },
   // IOS NET iobuf                            0x1FB00000   --   0x1FDFFFFF
   // IOS MCP unknown                          0x1FE00000   --   0x1FE3FFFF
   // IOS MCP unknown                          0x1FE40000   --   0x1FFFFFFF
   // IOS FS ramdisk                           0x20000000   --   0x27FFFFFF
   { PhysicalRegion::MEM2TilingApertures,    { 0x20000000_paddr, 0x2FFFFFFF_paddr } }, // TODO: Move tiling apertures away from ramdisk memory.
   // root.rpx                                 0x30000000   --   0x31FFFFFF
   { PhysicalRegion::MEM2SystemHeap,         { 0x32000000_paddr, 0x327FFFFF_paddr } },
   // unknown                                  0x32800000   --   0x32FFFFFF
   // Error Display                            0x33000000   --   0x33FFFFFF
   { PhysicalRegion::MEM2OverlayArena,       { 0x34000000_paddr, 0x4FFFFFFF_paddr } },
   { PhysicalRegion::MEM2AppHeap,            { 0x50000000_paddr, 0x8FFFFFFF_paddr } },

   // SRAM1 - 32 KB
   //0xFFF00000   --   0xFFF07FFF = IOS MCP sram1
   { PhysicalRegion::SRAM1,                  { 0xFFF00000_paddr, 0xFFF07FFF_paddr } },
   { PhysicalRegion::SRAM1C2W,               { 0xFFF00000_paddr, 0xFFF07FFF_paddr } },

   // SRAM0 - 64 KB
   //0xFFFF0000   --   0xFFFFFFFF = IOS kernel sram0
   { PhysicalRegion::SRAM0,                  { 0xFFFF0000_paddr, 0xFFFFFFFF_paddr } },
   { PhysicalRegion::SRAM0IosKernel,         { 0xFFFF0000_paddr, 0xFFFFFFFF_paddr } },
};

static std::map<VirtualRegion, VirtualMap>
sVirtualMemoryMap {
   { VirtualRegion::SystemHeap,              { 0x01000000_vaddr, 0x017FFFFF_vaddr, PhysicalRegion::MEM2SystemHeap } },

   { VirtualRegion::AppHeapCode,             { 0x02000000_vaddr, 0x0FFFFFFF_vaddr, PhysicalRegion::MEM2AppHeap } },
   { VirtualRegion::AppHeapData,             { 0x10000000_vaddr, 0x4FFFFFFF_vaddr, PhysicalRegion::MEM2AppHeap } },

   { VirtualRegion::OverlayArena,            { 0x60000000_vaddr, 0x7BFFFFFF_vaddr, PhysicalRegion::MEM2OverlayArena } },

   { VirtualRegion::TilingApertures,         { 0x80000000_vaddr, 0x8FFFFFFF_vaddr, PhysicalRegion::MEM2TilingApertures } },

   { VirtualRegion::VirtualMapRange,         { 0xA0000000_vaddr, 0xDFFFFFFF_vaddr, PhysicalRegion::Invalid } },
   { VirtualRegion::ForegroundBucket,        { 0xE0000000_vaddr, 0xE3FFFFFF_vaddr, PhysicalRegion::MEM2ForegroundBucket } },

   { VirtualRegion::MEM1,                    { 0xF4000000_vaddr, 0xF5FFFFFF_vaddr, PhysicalRegion::MEM1 } },

   { VirtualRegion::SharedData,              { 0xF8000000_vaddr, 0xFAFFFFFF_vaddr, PhysicalRegion::MEM2SharedData } },

   { VirtualRegion::LockedCache,             { 0xFFC00000_vaddr, 0xFFC1FFFF_vaddr, PhysicalRegion::LockedCache } },
};

// Maybe this is 1 page per core?
// We're using the results from a Mii Maker rpx loading environment.
static auto sAvailPhysicalBase = cpu::PhysicalAddress { 0 };
static const auto AvailPhysicalSize = uint32_t { 3 * 128 * 1024 };

static bool
map(VirtualMap &map)
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
unmap(VirtualMap &map)
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

   freeOverlayArena();
   freeTilingApertures();
   freeAppMemory();
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
