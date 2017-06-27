#include "ios_heap.h"
#include <common/teenyheap.h>

namespace ios
{

static TeenyHeap *
sTeenyHeap = nullptr;

IOSError
iosHeapInit(phys_addr base,
            uint32_t size)
{
   if (!sTeenyHeap) {
      sTeenyHeap = new TeenyHeap { phys_ptr<void> { base }.getRawPointer(), size };
   }

   return IOSError::OK;
}

phys_ptr<void>
iosHeapAlloc(uint32_t size,
             uint32_t alignment)
{
   return cpu::translatePhysical(sTeenyHeap->alloc(size, alignment));
}

void
iosHeapFree(phys_ptr<void> ptr)
{
   sTeenyHeap->free(ptr.getRawPointer());
}

} // namespace ios

