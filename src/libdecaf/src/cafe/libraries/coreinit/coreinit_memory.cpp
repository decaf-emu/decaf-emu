#include "coreinit.h"
#include "coreinit_internal_idlock.h"
#include "coreinit_memory.h"
#include "coreinit_systemheap.h"
#include "coreinit_systeminfo.h"

#include "cafe/cafe_stackobject.h"
#include "cafe/kernel/cafe_kernel_mmu.h"
#include "cafe/kernel/cafe_kernel_shareddata.h"

#include <atomic>
#include <cstring>

namespace cafe::coreinit
{

struct StaticMemoryData
{
   internal::IdLock boundsLock;
   be2_val<phys_addr> foregroundPhysicalAddress;
   be2_val<virt_addr> foregroundBaseAddress;
   be2_val<uint32_t> foregroundSize;

   be2_val<virt_addr> mem1BaseAddress;
   be2_val<uint32_t> mem1Size;

   be2_val<virt_addr> mem2BaseAddress;
   be2_val<uint32_t> mem2Size;
};

static virt_ptr<StaticMemoryData>
sMemoryData = nullptr;

enum ForegroundAreaId
{
   Application = 0,
   TransitionAudioBuffer = 1,
   SavedFrameUnk2 = 2,
   SavedFrameUnk3 = 3,
   SavedFrameUnk4 = 4,
   SavedFrameUnk5 = 5,
   Unknown6 = 6,
   CopyArea = 7,
};

struct ForegroundArea
{
   ForegroundAreaId id;
   uint32_t offset;
   uint32_t size;
};

constexpr std::array<ForegroundArea, 8> ForegroundAreas {
   ForegroundArea { ForegroundAreaId::Application,                   0, 0x2800000 },
   ForegroundArea { ForegroundAreaId::CopyArea,              0x2800000,  0x400000 },
   ForegroundArea { ForegroundAreaId::TransitionAudioBuffer, 0x2C00000,  0x900000 },
   ForegroundArea { ForegroundAreaId::SavedFrameUnk2,        0x3500000,  0x3C0000 },
   ForegroundArea { ForegroundAreaId::SavedFrameUnk3,        0x38C0000,  0x1C0000 },
   ForegroundArea { ForegroundAreaId::SavedFrameUnk4,        0x3A80000,  0x3C0000 },
   ForegroundArea { ForegroundAreaId::SavedFrameUnk5,        0x3E40000,  0x1BF000 },
   ForegroundArea { ForegroundAreaId::Unknown6,              0x3FFF000,    0x1000 },
};

static virt_ptr<void>
getForegroundAreaPointer(ForegroundAreaId id)
{
   for (auto &area : ForegroundAreas) {
      if (area.id == id) {
         return virt_cast<void *>(sMemoryData->foregroundBaseAddress + area.offset);
      }
   }

   return nullptr;
}

static uint32_t
getForegroundAreaSize(ForegroundAreaId id)
{
   for (auto &area : ForegroundAreas) {
      if (area.id == id) {
         return area.size;
      }
   }

   return 0;
}

virt_ptr<void>
OSBlockMove(virt_ptr<void> dst,
            virt_ptr<const void> src,
            uint32_t size,
            BOOL flush)
{
   memmove(dst, src, size);
   return dst;
}

virt_ptr<void>
OSBlockSet(virt_ptr<void> dst,
           int val,
           uint32_t size)
{
   memset(dst, val, size);
   return dst;
}


/**
 * Get the foreground memory bucket address and size.
 *
 * \return
 * Returns TRUE if the current process is in the foreground.
 */
BOOL
OSGetForegroundBucket(virt_ptr<virt_addr> addr,
                      virt_ptr<uint32_t> size)
{
   auto range = kernel::getForegroundBucket();

   if (addr) {
      *addr = range.first;
   }

   if (size) {
      *size = range.second;
   }

   return range.first && range.second;
}


/**
 * Get the area of the foreground bucket which the application can use.
 *
 * \return
 * Returns TRUE if the current process is in the foreground.
 */
BOOL
OSGetForegroundBucketFreeArea(virt_ptr<virt_addr> addr,
                              virt_ptr<uint32_t> size)
{
   if (addr) {
      *addr = virt_cast<virt_addr>(getForegroundAreaPointer(ForegroundAreaId::Application));
   }

   if (size) {
      *size = getForegroundAreaSize(ForegroundAreaId::Application);
   }

   return !!sMemoryData->foregroundBaseAddress;
}

int32_t
OSGetMemBound(OSMemoryType type,
              virt_ptr<virt_addr> addr,
              virt_ptr<uint32_t> size)
{
   if (addr) {
      *addr = virt_addr { 0u };
   }

   if (size) {
      *size = 0u;
   }

   internal::acquireIdLockWithCoreId(sMemoryData->boundsLock);
   switch (type) {
   case OSMemoryType::MEM1:
      if (addr) {
         *addr = sMemoryData->mem1BaseAddress;
      }

      if (size) {
         *size = sMemoryData->mem1Size;
      }
      break;
   case OSMemoryType::MEM2:
      if (addr) {
         *addr = sMemoryData->mem2BaseAddress;
      }

      if (size) {
         *size = sMemoryData->mem2Size;
      }
      break;
   default:
      internal::releaseIdLockWithCoreId(sMemoryData->boundsLock);
      return -1;
   }

   internal::releaseIdLockWithCoreId(sMemoryData->boundsLock);
   return 0;
}

void
OSGetAvailPhysAddrRange(virt_ptr<phys_addr> start,
                        virt_ptr<uint32_t> size)
{
   auto range = kernel::getAvailablePhysicalAddressRange();

   if (start) {
      *start = range.first;
   }

   if (size) {
      *size = range.second;
   }
}

void
OSGetDataPhysAddrRange(virt_ptr<phys_addr> start,
                       virt_ptr<uint32_t> size)
{
   auto range = kernel::getDataPhysicalAddressRange();

   if (start) {
      *start = range.first;
   }

   if (size) {
      *size = range.second;
   }
}

void
OSGetMapVirtAddrRange(virt_ptr<virt_addr> start,
                      virt_ptr<uint32_t> size)
{
   auto range = kernel::getVirtualMapAddressRange();

   if (start) {
      *start = range.first;
   }

   if (size) {
      *size = range.second;
   }
}

BOOL
OSGetSharedData(OSSharedDataType type,
                uint32_t unk_r4,
                virt_ptr<virt_ptr<void>> outPtr,
                virt_ptr<uint32_t> outSize)
{
   auto area = kernel::SharedArea { };

   if (!outPtr || !outSize) {
      return FALSE;
   }

   switch (type) {
   case OSSharedDataType::FontChinese:
      area = kernel::getSharedArea(kernel::SharedAreaId::FontChinese);
      break;
   case OSSharedDataType::FontKorean:
      area = kernel::getSharedArea(kernel::SharedAreaId::FontKorean);
      break;
   case OSSharedDataType::FontStandard:
      area = kernel::getSharedArea(kernel::SharedAreaId::FontStandard);
      break;
   case OSSharedDataType::FontTaiwanese:
      area = kernel::getSharedArea(kernel::SharedAreaId::FontTaiwanese);
      break;
   default:
      return FALSE;
   }

   *outPtr = virt_cast<void *>(area.address);
   *outSize = area.size;
   return TRUE;
}

virt_addr
OSAllocVirtAddr(virt_addr address,
                uint32_t size,
                uint32_t alignment)
{
   return kernel::allocateVirtualAddress(address, size, alignment);
}

BOOL
OSFreeVirtAddr(virt_addr address,
               uint32_t size)
{
   return kernel::freeVirtualAddress(address, size) ? TRUE : FALSE;
}

int32_t
OSQueryVirtAddr(virt_addr address)
{
   return static_cast<int32_t>(kernel::queryVirtualAddress(address));
}

BOOL
OSMapMemory(virt_addr virtAddress,
            phys_addr physAddress,
            uint32_t size,
            int permission)
{
   return
      kernel::mapMemory(virtAddress, physAddress, size,
                        static_cast<kernel::MapMemoryPermission>(permission))
      ? TRUE : FALSE;
}

BOOL
OSUnmapMemory(virt_addr virtAddress,
              uint32_t size)
{
   return kernel::unmapMemory(virtAddress, size) ? TRUE : FALSE;
}


/**
 * Translates a virtual (effective) address to a physical address.
 */
phys_addr
OSEffectiveToPhysical(virt_addr address)
{
   if (address >= virt_addr { 0x10000000 } &&
       address < internal::getMem2EndAddress()) {
      return internal::getMem2PhysAddress() +
         static_cast<uint32_t>(address - 0x10000000);
   }

   if (address >= virt_addr { 0xF4000000 } &&
       address <  virt_addr { 0xF6000000 }) {
      return phys_addr { static_cast<uint32_t>(address - 0xF4000000) };
   }

   if (sMemoryData->foregroundBaseAddress &&
       address >= sMemoryData->foregroundBaseAddress &&
       address <  sMemoryData->foregroundBaseAddress + sMemoryData->foregroundSize) {
      return sMemoryData->foregroundPhysicalAddress +
         static_cast<uint32_t>(address - sMemoryData->foregroundBaseAddress);
   }

   return kernel::effectiveToPhysical(address);
}


/**
 * Translates a physical address to a virtual (effective) address.
 */
virt_addr
OSPhysicalToEffectiveCached(phys_addr address)
{
   return kernel::physicalToEffectiveCached(address);
}


/**
 * Translates a physical address to a virtual (effective) address.
 */
virt_addr
OSPhysicalToEffectiveUncached(phys_addr address)
{
   return kernel::physicalToEffectiveUncached(address);
}


/**
 * memcpy for virtual memory.
 */
virt_ptr<void>
memcpy(virt_ptr<void> dst,
       virt_ptr<const void> src,
       uint32_t size)
{
   std::memcpy(dst.get(), src.get(), size);
   return dst;
}


/**
 * memmove for virtual memory.
 */
virt_ptr<void>
memmove(virt_ptr<void> dst,
        virt_ptr<const void> src,
        uint32_t size)
{
   std::memmove(dst.get(), src.get(), size);
   return dst;
}


/**
 * memset for virtual memory.
 */
virt_ptr<void>
memset(virt_ptr<void> dst,
       int value,
       uint32_t size)
{
   std::memset(dst.get(), value, size);
   return dst;
}

namespace internal
{

void
initialiseMemory()
{
   sMemoryData->mem1BaseAddress = virt_addr { 0xF4000000 };
   sMemoryData->mem1Size = 0x2000000u;

   auto mem2BaseAddress = align_up(getMem2BaseAddress(), 4096);
   auto mem2EndAddress = align_down(getMem2EndAddress(), 4096);

   auto systemHeapBaseAddress = mem2BaseAddress;
   auto systemHeapSize = getSystemHeapSize();
   initialiseSystemHeap(virt_cast<void *>(systemHeapBaseAddress),
                        systemHeapSize);

   sMemoryData->mem2BaseAddress = align_up(mem2BaseAddress + systemHeapSize, 4096);
   sMemoryData->mem2Size = static_cast<uint32_t>(mem2EndAddress - sMemoryData->mem2BaseAddress);

   OSGetForegroundBucket(virt_addrof(sMemoryData->foregroundBaseAddress),
                         virt_addrof(sMemoryData->foregroundSize));

   sMemoryData->foregroundPhysicalAddress =
      kernel::effectiveToPhysical(sMemoryData->foregroundBaseAddress);
}

} // namespace internal

void
Library::registerMemorySymbols()
{
   RegisterFunctionExport(OSBlockMove);
   RegisterFunctionExport(OSBlockSet);
   RegisterFunctionExport(OSGetMemBound);
   RegisterFunctionExport(OSGetForegroundBucket);
   RegisterFunctionExport(OSGetForegroundBucketFreeArea);
   RegisterFunctionExport(OSGetAvailPhysAddrRange);
   RegisterFunctionExport(OSGetDataPhysAddrRange);
   RegisterFunctionExport(OSGetMapVirtAddrRange);
   RegisterFunctionExport(OSGetSharedData);
   RegisterFunctionExport(OSAllocVirtAddr);
   RegisterFunctionExport(OSFreeVirtAddr);
   RegisterFunctionExport(OSQueryVirtAddr);
   RegisterFunctionExport(OSMapMemory);
   RegisterFunctionExport(OSUnmapMemory);
   RegisterFunctionExport(OSEffectiveToPhysical);
   RegisterFunctionExportName("__OSPhysicalToEffectiveCached",
                              OSPhysicalToEffectiveCached);
   RegisterFunctionExportName("__OSPhysicalToEffectiveUncached",
                              OSPhysicalToEffectiveUncached);
   RegisterFunctionExport(memcpy);
   RegisterFunctionExport(memmove);
   RegisterFunctionExport(memset);

   RegisterDataInternal(sMemoryData);
}

} // namespace cafe::coreinit
