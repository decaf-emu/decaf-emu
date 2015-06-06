#pragma once
#include "systemtypes.h"
#include "coreinit_memory.h"

WHeapHandle
MEMCreateFrmHeap(p32<void> addr, uint32_t size);

WHeapHandle
MEMCreateFrmHeapEx(p32<void> addr, uint32_t size, uint16_t flags);

p32<void>
MEMDestroyFrmHeap(WHeapHandle handle);

p32<void>
MEMAllocFromFrmHeap(WHeapHandle handle, uint32_t size);

p32<void>
MEMAllocFromFrmHeapEx(WHeapHandle handle, uint32_t size, int alignment);

void
MEMFreeToFrmHeap(WHeapHandle handle, int mode);

BOOL
MEMRecordStateForFrmHeap(WHeapHandle handle, uint32_t tag);

BOOL
MEMFreeByStateToFrmHeap(WHeapHandle handle, uint32_t tag);

uint32_t
MEMAdjustFrmHeap(WHeapHandle handle);

uint32_t
MEMResizeForMBlockFrmHeap(WHeapHandle handle, p32<void> addr, uint32_t size);

uint32_t
MEMGetAllocatableSizeForFrmHeap(WHeapHandle handle);

uint32_t
MEMGetAllocatableSizeForFrmHeapEx(WHeapHandle handle, int alignment);
