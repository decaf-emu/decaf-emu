#include "coreinit.h"
#include "coreinit_expheap.h"
#include "expandedheapmanager.h"
#include "system.h"

/*
void MEMVisitAllocatedForExpHeap(WHeapHandle handle, uint32_t visitorFn, uint32_t param);
BOOL MEMCheckExpHeap(WHeapHandle handle, uint32_t flags);
BOOL MEMCheckForMBlockExpHeap(p32<const void> block, WHeapHandle heap, uint32_t flags);
*/

static ExpandedHeapManager *
getExpandedHeap(WHeapHandle handle)
{
   return reinterpret_cast<ExpandedHeapManager *>(gSystem.getHeap(handle));
}

static ExpandedHeapManager *
getExpandedHeap(p32<void> addr)
{
   return reinterpret_cast<ExpandedHeapManager *>(gSystem.getHeapByAddress(addr.value));
}

WHeapHandle MEMCreateExpHeap(p32<void> address, uint32_t size)
{
   return MEMCreateExpHeapEx(address, size, 0);
}

WHeapHandle MEMCreateExpHeapEx(p32<void> address, uint32_t size, uint16_t flags)
{
   return gSystem.addHeap(new ExpandedHeapManager(address.value, size, static_cast<HeapFlags>(flags)));
}

p32<void>
MEMDestroyExpHeap(WHeapHandle handle)
{
   // Destroy heap
   auto heap = gSystem.getHeap(handle);
   auto base = heap->getAddress();
   delete heap;

   // Remove from heap list
   gSystem.removeHeap(handle);

   return make_p32<void>(base);
}

p32<void>
MEMAllocFromExpHeap(WHeapHandle handle, uint32_t size)
{
   return MEMAllocFromExpHeapEx(handle, size, 4);
}

p32<void>
MEMAllocFromExpHeapEx(WHeapHandle handle, uint32_t size, int alignment)
{
   return make_p32<void>(getExpandedHeap(handle)->alloc(size, alignment));
}

void
MEMFreeToExpHeap(WHeapHandle handle, p32<void> address)
{
   getExpandedHeap(handle)->free(address.value);
}

HeapMode
MEMSetAllocModeForExpHeap(WHeapHandle handle, HeapMode mode)
{
   return getExpandedHeap(handle)->setMode(mode);
}

HeapMode
MEMGetAllocModeForExpHeap(WHeapHandle handle)
{
   return getExpandedHeap(handle)->getMode();
}

uint32_t
MEMAdjustExpHeap(WHeapHandle handle)
{
   return getExpandedHeap(handle)->trimHeap();
}

uint32_t
MEMResizeForMBlockExpHeap(WHeapHandle handle, p32<void> block, uint32_t size)
{
   return getExpandedHeap(handle)->resizeBlock(block.value, size);
}

uint32_t
MEMGetTotalFreeSizeForExpHeap(WHeapHandle handle)
{
   return getExpandedHeap(handle)->getTotalFreeSize();
}

uint32_t
MEMGetAllocatableSizeForExpHeap(WHeapHandle handle)
{
   return MEMGetAllocatableSizeForExpHeapEx(handle, 4);
}

uint32_t
MEMGetAllocatableSizeForExpHeapEx(WHeapHandle handle, int alignment)
{
   return getExpandedHeap(handle)->getLargestFreeBlockSize(alignment);
}

uint16_t
MEMSetGroupIDForExpHeap(WHeapHandle handle, uint16_t id)
{
   return getExpandedHeap(handle)->setGroup(static_cast<uint8_t>(id));
}

uint16_t
MEMGetGroupIDForExpHeap(WHeapHandle handle)
{
   return getExpandedHeap(handle)->getGroup();
}

uint32_t
MEMGetSizeForMBlockExpHeap(p32<void> block)
{
   return getExpandedHeap(block)->getBlockSize(block.value);
}

uint16_t
MEMGetGroupIDForMBlockExpHeap(p32<void> block)
{
   return getExpandedHeap(block)->getBlockGroup(block.value);
}

HeapDirection
MEMGetAllocDirForMBlockExpHeap(p32<void> block)
{
   return getExpandedHeap(block)->getBlockDirection(block.value);
}

void
CoreInit::registerExpHeapFunctions()
{
   RegisterSystemFunction(MEMCreateExpHeap);
   RegisterSystemFunction(MEMCreateExpHeapEx);
   RegisterSystemFunction(MEMDestroyExpHeap);
   RegisterSystemFunction(MEMAllocFromExpHeap);
   RegisterSystemFunction(MEMAllocFromExpHeapEx);
   RegisterSystemFunction(MEMFreeToExpHeap);
   RegisterSystemFunction(MEMSetAllocModeForExpHeap);
   RegisterSystemFunction(MEMGetAllocModeForExpHeap);
   RegisterSystemFunction(MEMAdjustExpHeap);
   RegisterSystemFunction(MEMResizeForMBlockExpHeap);
   RegisterSystemFunction(MEMGetTotalFreeSizeForExpHeap);
   RegisterSystemFunction(MEMGetAllocatableSizeForExpHeap);
   RegisterSystemFunction(MEMGetAllocatableSizeForExpHeapEx);
   RegisterSystemFunction(MEMSetGroupIDForExpHeap);
   RegisterSystemFunction(MEMGetGroupIDForExpHeap);
   RegisterSystemFunction(MEMGetSizeForMBlockExpHeap);
   RegisterSystemFunction(MEMGetGroupIDForMBlockExpHeap);
   RegisterSystemFunction(MEMGetAllocDirForMBlockExpHeap);
}
