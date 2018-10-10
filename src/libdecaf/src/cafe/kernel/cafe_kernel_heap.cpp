#include "cafe_kernel_heap.h"
#include "cafe_kernel_mmu.h"
#include "cafe/cafe_tinyheap.h"

#include <common/frameallocator.h>
#include <libcpu/be2_struct.h>

namespace cafe::kernel::internal
{

static FrameAllocator
sStaticDataHeap;

static virt_ptr<TinyHeap>
sWorkAreaHeap;

static constexpr auto
WorkAreaHeapTrackingBlockCount = 0x80;

void
initialiseStaticDataHeap()
{
   auto staticDataMapping = getVirtualMemoryMap(VirtualMemoryRegion::Kernel_0xFFE00000);
   sStaticDataHeap = FrameAllocator {
      virt_cast<void *>(staticDataMapping.vaddr).get(),
      staticDataMapping.size,
   };
}

bool
initialiseWorkAreaHeap()
{
   auto trackingSize = WorkAreaHeapTrackingBlockCount * TinyHeapBlockSize + TinyHeapHeaderSize;
   auto workAreaMapping = getVirtualMemoryMap(VirtualMemoryRegion::KernelWorkAreaHeap);
   sWorkAreaHeap = virt_cast<TinyHeap *>(workAreaMapping.vaddr);
   auto error = TinyHeap_Setup(sWorkAreaHeap,
                               trackingSize,
                               virt_cast<void *>(workAreaMapping.vaddr + trackingSize),
                               workAreaMapping.size - trackingSize);
   return error == TinyHeapError::OK;
}

virt_ptr<void>
allocStaticData(size_t size,
                size_t alignment)
{
   return virt_cast<void *>(cpu::translate(sStaticDataHeap.allocate(size, alignment)));
}

virt_ptr<void>
allocFromWorkArea(int32_t size,
                  int32_t alignment)
{
   virt_ptr<void> ptr;
   if (TinyHeap_Alloc(sWorkAreaHeap, size, alignment, &ptr) != TinyHeapError::OK) {
      return nullptr;
   }

   return ptr;
}

void
freeToWorkArea(virt_ptr<void> ptr)
{
   TinyHeap_Free(sWorkAreaHeap, ptr);
}

} // namespace cafe::kernel::internal
