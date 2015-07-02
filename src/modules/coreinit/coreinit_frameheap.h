#pragma once
#include "systemtypes.h"
#include "coreinit_memory.h"

namespace FrameHeapFreeMode
{
enum Flags
{
   Bottom = 1 << 0,
   Top = 1 << 1,
   All = Bottom | Top
};
}

struct FrameHeap;

FrameHeap *
MEMCreateFrmHeap(FrameHeap *heap, uint32_t size);

FrameHeap *
MEMCreateFrmHeapEx(FrameHeap *heap, uint32_t size, uint16_t flags);

void *
MEMDestroyFrmHeap(FrameHeap *heap);

void *
MEMAllocFromFrmHeap(FrameHeap *heap, uint32_t size);

void *
MEMAllocFromFrmHeapEx(FrameHeap *heap, uint32_t size, int alignment);

void
MEMFreeToFrmHeap(FrameHeap *heap, FrameHeapFreeMode::Flags mode);

BOOL
MEMRecordStateForFrmHeap(FrameHeap *heap, uint32_t tag);

BOOL
MEMFreeByStateToFrmHeap(FrameHeap *heap, uint32_t tag);

uint32_t
MEMAdjustFrmHeap(FrameHeap *heap);

uint32_t
MEMResizeForMBlockFrmHeap(FrameHeap *heap, uint32_t addr, uint32_t size);

uint32_t
MEMGetAllocatableSizeForFrmHeap(FrameHeap *heap);

uint32_t
MEMGetAllocatableSizeForFrmHeapEx(FrameHeap *heap, int alignment);
