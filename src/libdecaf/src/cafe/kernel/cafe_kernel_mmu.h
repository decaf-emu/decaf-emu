#pragma once
#include "cafe_kernel_processid.h"

#include <array>
#include <common/log.h>
#include <libcpu/be2_struct.h>
#include <libcpu/mmu.h>
#include <utility>

namespace cafe::kernel
{

static constexpr auto PageSize = 128 * 1024u;

struct MemoryMap
{
   virt_addr vaddr;
   uint32_t size;
   phys_addr paddr;
   uint32_t flags;
};

// This must be kept in order with cafe_kernel_mmu.cpp:sMemoryMap
enum class VirtualMemoryRegion
{
   CafeOS,
   CodeGenArea,
   AppCode,
   AppData,
   Overlay,
   ForegroundBucket,
   TilingApertures,
   LoaderGlobals,
   MEM1,
   LoaderBounceBuffer,
   SharedData,
   Unknown_0xFB000000,
   Registers_0xFC000000,
   Registers_0xFC0C0000,
   Registers_0xFC1E0000,
   Registers_0xFC200000,
   Registers_0xFC280000,
   WriteGatherMemory_0xFC2A0000,
   Registers_0xFC300000,
   Registers_0xFC320000,
   Registers_0xFD000000,
   Unknown_0xFE000000,
   KernelWorkAreaHeap,
   Unknown_0xFF280000,
   LockedCacheCore0,
   LockedCacheCore1,
   LockedCacheCore2,
   Unknown_0xFFCE0000,
   Kernel_0xFFE00000,
   Kernel_0xFFE40000,
   Kernel_0xFFE80000,
   Unknown_0xFFF60000,
   Unknown_0xFFF80000,
   Unknown_0xFFFA0000,
   Unknown_0xFFFC0000,
   Unknown_0xFFFE0000,
   Max,
};

enum class MapMemoryPermission
{
   ReadOnly    = 1,
   ReadWrite   = 2,
};

enum class QueryMemoryResult
{
   Invalid = 0,
   MappedReadOnly = 1,
   MappedReadWrite = 2,
   Free = 3,
   Allocated = 4,
};

MemoryMap &
getVirtualMemoryMap(VirtualMemoryRegion region);

std::pair<virt_addr, uint32_t>
getForegroundBucket();

std::pair<virt_addr, uint32_t>
enableOverlayArena();

void
disableOverlayArena();

std::pair<phys_addr, uint32_t>
getAvailablePhysicalAddressRange();

std::pair<phys_addr, uint32_t>
getDataPhysicalAddressRange();

std::pair<virt_addr, uint32_t>
getVirtualMapAddressRange();

virt_addr
allocateVirtualAddress(virt_addr addr,
                       uint32_t size,
                       uint32_t align);

bool
freeVirtualAddress(virt_addr addr,
                   uint32_t size);

QueryMemoryResult
queryVirtualAddress(cpu::VirtualAddress addr);

bool
mapMemory(virt_addr virtAddr,
          phys_addr physAddr,
          uint32_t size,
          MapMemoryPermission permission);

bool
unmapMemory(cpu::VirtualAddress addr,
            uint32_t size);

bool
validateAddressRange(virt_addr address,
                     uint32_t size);

phys_addr
effectiveToPhysical(virt_addr address);

template<typename Type>
inline phys_ptr<Type>
effectiveToPhysical(virt_ptr<Type> ptr)
{
   return phys_cast<Type *>(effectiveToPhysical(virt_cast<virt_addr>(ptr)));
}

template<typename Type>
inline phys_ptr<Type>
effectiveToPhysical(be2_virt_ptr<Type> ptr)
{
   return phys_cast<Type *>(effectiveToPhysical(virt_cast<virt_addr>(ptr)));
}

virt_addr
physicalToEffectiveCached(phys_addr address);

virt_addr
physicalToEffectiveUncached(phys_addr address);

namespace internal
{

struct AddressSpaceView
{
   std::array<MemoryMap, 40> mappings;
   uint32_t numMappings;
};

struct AddressSpace
{
   bool overlayArenaEnabled;
   virt_addr virtualMapStart;
   uint32_t virtualMapSize;
   phys_addr dataStart;
   uint32_t dataSize;
   phys_addr availStart;
   uint32_t availSize;

   std::array<uint16_t, 64> virtualPageMap;

   std::array<AddressSpaceView *, 3> perCoreView;
   AddressSpaceView codeGenView;
   AddressSpaceView view0;
   AddressSpaceView view1;
   AddressSpaceView view2;
};

AddressSpace *
getActiveAddressSpace();

bool
initialiseAddressSpace(AddressSpace *map,
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
                       bool overlayArenaEnabled);

bool
loadAddressSpace(AddressSpace *info);

} // namespace internal

} // namespace cafe::kernel
