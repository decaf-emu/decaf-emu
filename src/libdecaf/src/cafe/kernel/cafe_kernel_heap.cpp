#include "cafe_kernel_heap.h"
#include "cafe/cafe_tinyheap.h"
#include "kernel/kernel_memory.h"

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
   auto range = ::kernel::getVirtualRange(::kernel::VirtualRegion::KernelStatic);
   sStaticDataHeap = FrameAllocator {
      virt_cast<uint8_t *>(range.start).getRawPointer(),
      range.size
   };
}

bool
initialiseWorkAreaHeap()
{
   auto range = ::kernel::getVirtualRange(::kernel::VirtualRegion::KernelWorkAreaHeap);
   auto trackingSize = WorkAreaHeapTrackingBlockCount * TinyHeapBlockSize + TinyHeapHeaderSize;

   sWorkAreaHeap = virt_cast<TinyHeap *>(range.start);
   auto error = TinyHeap_Setup(sWorkAreaHeap,
                               trackingSize,
                               virt_cast<void *>(range.start + trackingSize),
                               range.size - trackingSize);
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
