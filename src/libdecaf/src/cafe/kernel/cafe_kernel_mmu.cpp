#include "cafe_kernel_mmu.h"
#include "cafe_kernel_process.h"

#include <array>
#include <common/log.h>
#include <libcpu/be2_struct.h>
#include <libcpu/cpu.h>
#include <libcpu/mmu.h>

namespace cafe::kernel
{

constexpr auto MaxCodeGenSize = 0x2000000u;
constexpr auto MaxTotalCodeSize = 0xE800000u;

static std::array<internal::AddressSpace *, 3>
sCoreActiveAddressSpace = { nullptr, nullptr, nullptr };

enum MemoryMapFlags
{
   MapUnknown1 = 1 << 1,
   MapUnknown2 = 1 << 2,
   MapUnknown4 = 1 << 4,
   MapUnknown6 = 1 << 6,
   MapCodeGen = 1 << 9,
   MapUnknown10 = 1 << 10,
   MapUnknown11 = 1 << 11,
   MapMainApplication = 1 << 13,
   MapUncached = 1 << 30,
};

// This must be kept in order with enum class VirtualMemoryRegion
MemoryMap sMemoryMap[] = {
   // CafeOS
   { virt_addr { 0x01000000 },   0x800000, phys_addr { 0x32000000 }, 0x2CE08002 },

   // CodeGen area
   { virt_addr { 0x01800000 },    0x20000, phys_addr {          0 }, 0x28101200 },

   // App Code
   { virt_addr { 0x02000000 },          0, phys_addr {          0 }, 0x2CF09400 },

   // App Data
   { virt_addr { 0x10000000 },          0, phys_addr {          0 }, 0x28305800 },

   // Overlay
   { virt_addr { 0xA0000000 }, 0x40000000, phys_addr {          0 }, 0x00002000 },

   // Foreground bucket
   { virt_addr { 0xE0000000 },  0x4000000, phys_addr { 0x14000000 }, 0x28204004 },

   // Tiling Apertures
   { virt_addr { 0xE8000000 },  0x2000000, phys_addr { 0xD0000000 }, 0x78200004 },

   // Loader globals which are also read/write by kernel
   { virt_addr { 0xEFE00000 },    0x80000, phys_addr { 0x1B900000 }, 0x28109010 },

   // MEM1
   { virt_addr { 0xF4000000 },  0x2000000, phys_addr {          0 }, 0x28204004 },

   // Loader bounce buffer
   { virt_addr { 0xF6000000 },   0x800000, phys_addr { 0x1B000000 }, 0x3CA08002 },

   // Shared data
   { virt_addr { 0xF8000000 },  0x3000000, phys_addr { 0x18000000 }, 0x2CA08002 },

   // Unknown
   { virt_addr { 0xFB000000 },   0x800000, phys_addr { 0x1C800000 }, 0x28200002 },

   // Registers
   { virt_addr { 0xFC000000 },    0xC0000, phys_addr {  0xC000000 }, 0x70100022 },
   { virt_addr { 0xFC0C0000 },   0x120000, phys_addr {  0xC0C0000 }, 0x70100022 },
   { virt_addr { 0xFC1E0000 },    0x20000, phys_addr {  0xC1E0000 }, 0x78100024 },
   { virt_addr { 0xFC200000 },    0x80000, phys_addr {  0xC200000 }, 0x78100024 },
   { virt_addr { 0xFC280000 },    0x20000, phys_addr {  0xC280000 }, 0x78100024 },

   // Write gather memory, 0x20000 per core
   { virt_addr { 0xFC2A0000 },    0x20000, phys_addr {  0xC2A0000 }, 0x78100023 },

   // Registers
   { virt_addr { 0xFC300000 },    0x20000, phys_addr {  0xC300000 }, 0x78100024 },
   { virt_addr { 0xFC320000 },    0xE0000, phys_addr {  0xC320000 }, 0x70100022 },
   { virt_addr { 0xFD000000 },   0x400000, phys_addr {  0xD000000 }, 0x70100022 },

   // Unknown
   { virt_addr { 0xFE000000 },   0x800000, phys_addr { 0x1C000000 }, 0x20200002 },

   // Kernel "work area" heap
   { virt_addr { 0xFF200000 },    0x80000, phys_addr { 0x1B800000 }, 0x20100040 },

   // Unknown
   { virt_addr { 0xFF280000 },    0x80000, phys_addr { 0x1B880000 }, 0x20100040 },

   // Locked Cache
   { virt_addr { 0xFFC00000 },    0x20000, phys_addr { 0xFFC00000 }, 0x08100004 },
   { virt_addr { 0xFFC40000 },    0x20000, phys_addr { 0xFFC40000 }, 0x08100004 },
   { virt_addr { 0xFFC80000 },    0x20000, phys_addr { 0xFFC80000 }, 0x0810000C },

   // Unknown
   { virt_addr { 0xFFCE0000 },    0x20000, phys_addr {          0 }, 0x50100004 },

   // Kernel data
   { virt_addr { 0xFFE00000 },    0x20000, phys_addr { 0xFFE00000 }, 0x20100040 },
   { virt_addr { 0xFFE40000 },    0x20000, phys_addr { 0xFFE40000 }, 0x20100040 },
   { virt_addr { 0xFFE80000 },    0x60000, phys_addr { 0xFFE80000 }, 0x20100040 },

   // Unknown
   { virt_addr { 0xFFF60000 },    0x20000, phys_addr { 0xFFE20000 }, 0x20100080 },
   { virt_addr { 0xFFF80000 },    0x20000, phys_addr { 0xFFE60000 }, 0x2C100040 },
   { virt_addr { 0xFFFA0000 },    0x20000, phys_addr { 0xFFE60000 }, 0x20100080 },
   { virt_addr { 0xFFFC0000 },    0x20000, phys_addr { 0x1BFE0000 }, 0x24100002 },
   { virt_addr { 0xFFFE0000 },    0x20000, phys_addr { 0x1BF80000 }, 0x28100102 },
};

constexpr auto sMemoryMapSize = sizeof(sMemoryMap) / sizeof(sMemoryMap[0]);
static_assert(static_cast<size_t>(VirtualMemoryRegion::Max) == sMemoryMapSize);

MemoryMap &
getVirtualMemoryMap(VirtualMemoryRegion region)
{
   return sMemoryMap[static_cast<size_t>(region)];
}

std::pair<virt_addr, uint32_t>
getForegroundBucket()
{
   // TODO: Ensure process is in foreground, else return { 0, 0 }
   return { virt_addr { 0xE0000000 },  0x4000000 };
}

std::pair<virt_addr, uint32_t>
enableOverlayArena()
{
   auto partitionData = internal::getCurrentRamPartitionData();

   // TODO: In a multi process environment we will have to exit the overlay
   // process.

   if (!partitionData->addressSpace.overlayArenaEnabled) {
      partitionData->addressSpace.overlayArenaEnabled = true;
   }

   return { virt_addr { 0xA0000000 }, 0x1C000000 };
}

void
disableOverlayArena()
{
   auto partitionData = internal::getCurrentRamPartitionData();
   partitionData->addressSpace.overlayArenaEnabled = false;
}

std::pair<phys_addr, uint32_t>
getAvailablePhysicalAddressRange()
{
   auto partitionData = internal::getCurrentRamPartitionData();
   return { partitionData->addressSpace.availStart,
            partitionData->addressSpace.availSize };
}

std::pair<phys_addr, uint32_t>
getDataPhysicalAddressRange()
{
   auto partitionData = internal::getCurrentRamPartitionData();
   return { partitionData->addressSpace.dataStart,
            partitionData->addressSpace.dataSize };
}

std::pair<virt_addr, uint32_t>
getVirtualMapAddressRange()
{
   auto partitionData = internal::getCurrentRamPartitionData();
   return { partitionData->addressSpace.virtualMapStart,
            partitionData->addressSpace.virtualMapSize };
}

static bool
checkInVirtualMapRange(internal::AddressSpace *addressSpace,
                       virt_addr addr,
                       uint32_t size)
{
   return
      addr >= addressSpace->virtualMapStart &&
      (addr + size) < (addressSpace->virtualMapStart + addressSpace->virtualMapSize);
}

virt_addr
allocateVirtualAddress(virt_addr addr,
                       uint32_t size,
                       uint32_t align)
{
   auto partitionData = internal::getCurrentRamPartitionData();
   if (addr && !checkInVirtualMapRange(&partitionData->addressSpace, addr, size)) {
      return virt_addr { 0 };
   }

   if (!align) {
      align = PageSize;
   }

   if (align & (align - 1)) {
      return virt_addr { 0 };
   }

   if (align < PageSize) {
      return virt_addr { 0 };
   }

   if (!addr) {
      auto range = cpu::findFreeVirtualAddressInRange(
         { partitionData->addressSpace.virtualMapStart, partitionData->addressSpace.virtualMapSize },
         size, align);
      addr = range.start;
      size = range.size;
   }

   if (!cpu::allocateVirtualAddress(addr, size)) {
      return virt_addr { 0 };
   }

   return addr;
}

bool
freeVirtualAddress(virt_addr addr,
                   uint32_t size)
{
   auto partitionData = internal::getCurrentRamPartitionData();
   if (!checkInVirtualMapRange(&partitionData->addressSpace, addr, size)) {
      return false;
   }

   return cpu::freeVirtualAddress(addr, size);
}

QueryMemoryResult
queryVirtualAddress(cpu::VirtualAddress addr)
{
   auto partitionData = internal::getCurrentRamPartitionData();
   if (!checkInVirtualMapRange(&partitionData->addressSpace, addr, 0)) {
      return QueryMemoryResult::Invalid;
   }

   switch (cpu::queryVirtualAddress(addr)) {
   case cpu::VirtualMemoryType::MappedReadOnly:
      return QueryMemoryResult::MappedReadOnly;
   case cpu::VirtualMemoryType::MappedReadWrite:
      return QueryMemoryResult::MappedReadWrite;
   case cpu::VirtualMemoryType::Free:
      return QueryMemoryResult::Free;
   case cpu::VirtualMemoryType::Allocated:
      return QueryMemoryResult::Allocated;
   default:
      return QueryMemoryResult::Invalid;
   }
}

bool
mapMemory(virt_addr virtAddr,
          phys_addr physAddr,
          uint32_t size,
          MapMemoryPermission permission)
{
   auto partitionData = internal::getCurrentRamPartitionData();
   if (!checkInVirtualMapRange(&partitionData->addressSpace, virtAddr, size)) {
      return false;
   }

   auto cpuPermission = cpu::MapPermission { };

   if (permission == MapMemoryPermission::ReadOnly) {
      cpuPermission = cpu::MapPermission::ReadOnly;
   } else if (permission == MapMemoryPermission::ReadWrite) {
      cpuPermission = cpu::MapPermission::ReadWrite;
   } else {
      gLog->error("Unexpected mapMemory permission: {}",
                  static_cast<unsigned>(permission));
      return false;
   }

   return cpu::mapMemory(virtAddr, physAddr, size, cpuPermission);
}

bool
unmapMemory(cpu::VirtualAddress addr,
            uint32_t size)
{
   auto partitionData = internal::getCurrentRamPartitionData();
   if (!checkInVirtualMapRange(&partitionData->addressSpace, addr, size)) {
      return false;
   }

   return cpu::unmapMemory(addr, size);
}

bool
validateAddressRange(virt_addr address,
                     uint32_t size)
{
   auto addressSpace = internal::getActiveAddressSpace();
   auto view = addressSpace->perCoreView[cpu::this_core::id()];

   for (auto i = 0u; i < view->numMappings; ++i) {
      if (address >= view->mappings[i].vaddr &&
         (address + size) <= (view->mappings[i].vaddr + view->mappings[i].size)) {
         return true;
      }
   }

   return false;
}

phys_addr
effectiveToPhysical(virt_addr address)
{
   auto addressSpace = internal::getActiveAddressSpace();
   auto view = addressSpace->perCoreView[cpu::this_core::id()];

   // Lookup in our mappings
   for (auto i = 0u; i < view->numMappings; ++i) {
      if (address < view->mappings[i].vaddr ||
          address >= view->mappings[i].vaddr + view->mappings[i].size) {
         continue;
      }

      return view->mappings[i].paddr +
         static_cast<uint32_t>(address - view->mappings[i].vaddr);
   }

   // Lookup in user mappings
   if (checkInVirtualMapRange(addressSpace, address, 0)) {
      auto result = phys_addr { 0 };
      if (cpu::virtualToPhysicalAddress(address, result)) {
         return result;
      }
   }

   return phys_addr { 0 };
}

virt_addr
physicalToEffectiveCached(phys_addr address)
{
   auto addressSpace = internal::getActiveAddressSpace();
   auto view = addressSpace->perCoreView[cpu::this_core::id()];

   // Lookup in our mappings only for memory regions NOT marked as uncached
   for (auto i = 0u; i < view->numMappings; ++i) {
      if ((view->mappings[i].flags & MapUncached) ||
          address < view->mappings[i].paddr ||
          address >= view->mappings[i].paddr + view->mappings[i].size) {
         continue;
      }

      return view->mappings[i].vaddr +
         static_cast<uint32_t>(address - view->mappings[i].paddr);
   }

   return virt_addr { 0 };
}

virt_addr
physicalToEffectiveUncached(phys_addr address)
{
   auto addressSpace = internal::getActiveAddressSpace();
   auto view = addressSpace->perCoreView[cpu::this_core::id()];

   // Lookup in our mappings only for memory regions marked as uncached
   for (auto i = 0u; i < view->numMappings; ++i) {
      if (!(view->mappings[i].flags & MapUncached) ||
          address < view->mappings[i].paddr ||
          address >= view->mappings[i].paddr + view->mappings[i].size) {
         continue;
      }

      return view->mappings[i].vaddr +
         static_cast<uint32_t>(address - view->mappings[i].paddr);
   }

   return virt_addr { 0 };
}

namespace internal
{

/**
 * KiInitAddressSpace
 */

static void
setAddressSpaceView(AddressSpaceView *view,
                    MemoryMap *mappings,
                    uint32_t numMappings,
                    uint32_t flags)
{
   view->numMappings = 0u;

   for (auto i = 0u; i < numMappings; ++i) {
      auto &mapping = mappings[i];
      if ((mapping.flags >> 16) & flags) {
         view->mappings[view->numMappings++] = mapping;
      }
   }
}

AddressSpace *
getActiveAddressSpace()
{
   return sCoreActiveAddressSpace[cpu::this_core::id()];
}

void
setActiveAddressSpace(AddressSpace *addressSpace)
{
   sCoreActiveAddressSpace[cpu::this_core::id()] = addressSpace;
}

bool
initialiseAddressSpace(AddressSpace *addressSpace,
                       RamPartitionId rampid,
                       phys_addr codeStart,
                       uint32_t codeSize,
                       phys_addr dataStart,
                       uint32_t dataSize,
                       uint32_t a7,
                       uint32_t a8,
                       phys_addr availStart,
                       uint32_t availSize,
                       phys_addr codeGenStart,
                       uint32_t codeGenSize,
                       uint32_t codeGenCore,
                       bool overlayArenaEnabled)
{
   auto baseFlags = 0u;
   if (codeGenSize > MaxCodeGenSize) {
      return false;
   }

   if (codeGenSize + codeSize > MaxTotalCodeSize) {
      return false;
   }

   if (rampid == RamPartitionId::MainApplication) {
      auto overlayArenaSize = 0u;
      if (overlayArenaEnabled) {
         overlayArenaSize = 0x20000000u;
      }

      addressSpace->virtualMapStart = virt_addr { 0xA0000000 } + overlayArenaSize;
      addressSpace->virtualMapSize = 0x40000000u - overlayArenaSize;
      addressSpace->virtualPageMap.fill(0x7FFF);

      addressSpace->dataStart = dataStart;
      addressSpace->dataSize = dataSize;

      addressSpace->availStart = availStart;
      addressSpace->availSize = availSize;

      baseFlags |= MapMainApplication;
   }

   sMemoryMap[2].vaddr = virt_addr { 0x10000000u } - codeSize;
   sMemoryMap[2].paddr = codeStart;
   sMemoryMap[2].size = codeSize;

   sMemoryMap[3].paddr = dataStart;
   sMemoryMap[3].size = dataSize;

   if (codeGenSize) {
      auto codeGenMap = sMemoryMap[1];
      codeGenMap.size = codeGenSize;
      codeGenMap.paddr = codeGenStart;
      setAddressSpaceView(&addressSpace->codeGenView,
                          &codeGenMap,
                          1,
                          MapCodeGen | MapUnknown10);
   }

   setAddressSpaceView(&addressSpace->viewKernel,
                       sMemoryMap,
                       sMemoryMapSize,
                       baseFlags | MapUnknown1 |
                       MapUnknown6 | MapUnknown10 |
                       MapUnknown11 | MapUnknown2);

   setAddressSpaceView(&addressSpace->viewUnknownKernelProcess1,
                       sMemoryMap,
                       sMemoryMapSize,
                       baseFlags | MapUnknown1 |
                       MapUnknown6 | MapUnknown10 |
                       MapUnknown11);

   setAddressSpaceView(&addressSpace->viewLoader,
                       sMemoryMap,
                       sMemoryMapSize,
                       baseFlags | MapUnknown1 |
                       MapUnknown6 | MapUnknown10 |
                       MapUnknown11 | MapUnknown4);

   // TODO: For now we hardcode to viewLoader because we do not change views
   addressSpace->perCoreView.fill(&addressSpace->viewLoader);
   return true;
}

static bool
loadMapping(MemoryMap &mapping)
{
   if (!cpu::allocateVirtualAddress(mapping.vaddr, mapping.size)) {
      gLog->error("Unexpected failure allocating virtual address {} - {}",
                  mapping.vaddr, mapping.vaddr + mapping.size);
      return false;
   }

   if (!cpu::mapMemory(mapping.vaddr, mapping.paddr, mapping.size, cpu::MapPermission::ReadWrite)) {
      gLog->error("Unexpected failure allocating mapping virtual address {} to physical address {}",
                  mapping.vaddr, mapping.paddr);
      return false;
   }

   return true;
}

bool
loadAddressSpace(AddressSpace *addressSpace)
{
   if (!cpu::resetVirtualMemory()) {
      decaf_abort("Unexpected failure resetting virtual memory");
   }

   auto coreId = cpu::this_core::id();
   if (coreId == cpu::InvalidCoreId) {
      // We have to load the kernel address space before we are running on a cpu
      // core, so in that case load the address space for core 1
      coreId = 1;
   }

   auto view = addressSpace->perCoreView[coreId];
   for (auto i = 0u; i < view->numMappings; ++i) {
      auto &mapping = view->mappings[i];
      if (!mapping.vaddr || !mapping.size) {
         continue;
      }

      // FIXME: Because we do not fully emulate the mappings yet we need to
      // check if paddr != 0, except paddr == 0 is valid for MEM1 @ 0xF4000000
      if (!mapping.paddr && mapping.vaddr != virt_addr { 0xF4000000 }) {
         continue;
      }

      loadMapping(mapping);
   }

   return true;
}

} // namespace internal

} // namespace cafe::kernel
