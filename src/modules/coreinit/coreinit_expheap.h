#pragma once
#include "types.h"
#include "coreinit_enum.h"
#include "coreinit_memheap.h"

struct ExpandedHeap;

ExpandedHeap *
MEMCreateExpHeap(ExpandedHeap *heap, uint32_t size);

ExpandedHeap *
MEMCreateExpHeapEx(ExpandedHeap *heap, uint32_t size, uint16_t flags);

ExpandedHeap *
MEMDestroyExpHeap(ExpandedHeap *heap);

void
MEMiDumpExpHeap(ExpandedHeap *heap);

void *
MEMAllocFromExpHeap(ExpandedHeap *heap, uint32_t size);

void *
MEMAllocFromExpHeapEx(ExpandedHeap *heap, uint32_t size, int alignment);

void
MEMFreeToExpHeap(ExpandedHeap *heap, uint8_t *block);

MEMExpHeapMode
MEMSetAllocModeForExpHeap(ExpandedHeap *heap, MEMExpHeapMode mode);

MEMExpHeapMode
MEMGetAllocModeForExpHeap(ExpandedHeap *heap);

uint32_t
MEMAdjustExpHeap(ExpandedHeap *heap);

uint32_t
MEMResizeForMBlockExpHeap(ExpandedHeap *heap, uint8_t *address, uint32_t size);

uint32_t
MEMGetTotalFreeSizeForExpHeap(ExpandedHeap *heap);

uint32_t
MEMGetAllocatableSizeForExpHeap(ExpandedHeap *heap);

uint32_t
MEMGetAllocatableSizeForExpHeapEx(ExpandedHeap *heap, int alignment);

uint16_t
MEMSetGroupIDForExpHeap(ExpandedHeap *heap, uint16_t id);

uint16_t
MEMGetGroupIDForExpHeap(ExpandedHeap *heap);

uint32_t
MEMGetSizeForMBlockExpHeap(uint8_t *addr);

uint16_t
MEMGetGroupIDForMBlockExpHeap(uint8_t *addr);

MEMExpHeapDirection
MEMGetAllocDirForMBlockExpHeap(uint8_t *addr);
