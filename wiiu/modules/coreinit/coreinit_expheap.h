#pragma once
#include "systemtypes.h"
#include "coreinit_memory.h"
#include "expandedheapmanager.h"

WHeapHandle
MEMCreateExpHeap(p32<void> address, uint32_t size);

WHeapHandle
MEMCreateExpHeapEx(p32<void> address, uint32_t size, uint16_t flags);

p32<void>
MEMDestroyExpHeap(WHeapHandle handle);

p32<void>
MEMAllocFromExpHeap(WHeapHandle handle, uint32_t size);

p32<void>
MEMAllocFromExpHeapEx(WHeapHandle heap, uint32_t size, int alignment);

void
MEMFreeToExpHeap(WHeapHandle handle, p32<void> address);

HeapMode
MEMSetAllocModeForExpHeap(WHeapHandle handle, HeapMode mode);

HeapMode
MEMGetAllocModeForExpHeap(WHeapHandle handle);

uint32_t
MEMAdjustExpHeap(WHeapHandle handle);

uint32_t
MEMResizeForMBlockExpHeap(WHeapHandle handle, p32<void> block, uint32_t size);

uint32_t
MEMGetTotalFreeSizeForExpHeap(WHeapHandle handle);

uint32_t
MEMGetAllocatableSizeForExpHeap(WHeapHandle handle);

uint32_t
MEMGetAllocatableSizeForExpHeapEx(WHeapHandle handle, int alignment);

uint16_t
MEMSetGroupIDForExpHeap(WHeapHandle handle, uint16_t id);

uint16_t
MEMGetGroupIDForExpHeap(WHeapHandle handle);

uint32_t
MEMGetSizeForMBlockExpHeap(p32<void> block);

uint16_t
MEMGetGroupIDForMBlockExpHeap(p32<void> block);

HeapDirection
MEMGetAllocDirForMBlockExpHeap(p32<void> block);
