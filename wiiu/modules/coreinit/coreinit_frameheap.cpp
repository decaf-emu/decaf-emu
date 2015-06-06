#include "coreinit.h"
#include "coreinit_frameheap.h"
#include "frameheapmanager.h"
#include "system.h"

static FrameHeapManager *
getFrameHeap(WHeapHandle handle)
{
   return reinterpret_cast<FrameHeapManager *>(gSystem.getHeap(handle));
}

WHeapHandle
MEMCreateFrmHeap(p32<void> addr, uint32_t size)
{
   return MEMCreateFrmHeapEx(addr, size, 0);
}

WHeapHandle
MEMCreateFrmHeapEx(p32<void> addr, uint32_t size, uint16_t flags)
{
   auto heap = new FrameHeapManager(addr.value, size, static_cast<HeapFlags>(flags));
   return gSystem.addHeap(heap);
}

p32<void>
MEMDestroyFrmHeap(WHeapHandle handle)
{
   // Destroy heap
   auto heap = getFrameHeap(handle);
   auto base = heap->getAddress();
   delete heap;

   // Remove from heap list
   gSystem.removeHeap(handle);

   return make_p32<void>(base);
}

p32<void>
MEMAllocFromFrmHeap(WHeapHandle handle, uint32_t size)
{
   return MEMAllocFromFrmHeapEx(handle, size, 4);
}

p32<void>
MEMAllocFromFrmHeapEx(WHeapHandle handle, uint32_t size, int alignment)
{
   return make_p32<void>(getFrameHeap(handle)->alloc(size, alignment));
}

void
MEMFreeToFrmHeap(WHeapHandle handle, int mode)
{
   getFrameHeap(handle)->free(handle, static_cast<FrameHeapFreeMode>(mode));
}

BOOL
MEMRecordStateForFrmHeap(WHeapHandle handle, uint32_t tag)
{
   return getFrameHeap(handle)->pushState(tag);
}

BOOL
MEMFreeByStateToFrmHeap(WHeapHandle handle, uint32_t tag)
{
   return getFrameHeap(handle)->popState(tag);
}

uint32_t
MEMAdjustFrmHeap(WHeapHandle handle)
{
   return getFrameHeap(handle)->trimHeap();
}

uint32_t
MEMResizeForMBlockFrmHeap(WHeapHandle handle, p32<void> addr, uint32_t size)
{
   return getFrameHeap(handle)->resizeBlock(addr.value, size);
}

uint32_t
MEMGetAllocatableSizeForFrmHeap(WHeapHandle handle)
{
   return MEMGetAllocatableSizeForFrmHeapEx(handle, 4);
}

uint32_t
MEMGetAllocatableSizeForFrmHeapEx(WHeapHandle handle, int alignment)
{
   return getFrameHeap(handle)->getLargestFreeBlockSize(alignment);
}

void
CoreInit::registerFrameHeapFunctions()
{
   RegisterSystemFunction(MEMCreateFrmHeap);
   RegisterSystemFunction(MEMCreateFrmHeapEx);
   RegisterSystemFunction(MEMDestroyFrmHeap);
   RegisterSystemFunction(MEMAllocFromFrmHeap);
   RegisterSystemFunction(MEMAllocFromFrmHeapEx);
   RegisterSystemFunction(MEMFreeToFrmHeap);
   RegisterSystemFunction(MEMRecordStateForFrmHeap);
   RegisterSystemFunction(MEMFreeByStateToFrmHeap);
   RegisterSystemFunction(MEMAdjustFrmHeap);
   RegisterSystemFunction(MEMResizeForMBlockFrmHeap);
   RegisterSystemFunction(MEMGetAllocatableSizeForFrmHeap);
   RegisterSystemFunction(MEMGetAllocatableSizeForFrmHeapEx);
}
