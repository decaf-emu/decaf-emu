#pragma once
#include "types.h"
#include "coreinit_enum.h"
#include "coreinit_memory.h"

namespace coreinit
{

/**
 * \defgroup coreinit_frameheap Frame Heap
 * \ingroup coreinit
 * @{
 */

struct FrameHeap;

FrameHeap *
MEMCreateFrmHeap(FrameHeap *heap,
                 uint32_t size);

FrameHeap *
MEMCreateFrmHeapEx(FrameHeap *heap,
                   uint32_t size,
                   uint16_t flags);

void *
MEMDestroyFrmHeap(FrameHeap *heap);

void *
MEMAllocFromFrmHeap(FrameHeap *heap,
                    uint32_t size);

void *
MEMAllocFromFrmHeapEx(FrameHeap *heap,
                      uint32_t size,
                      int alignment);

void
MEMFreeToFrmHeap(FrameHeap *heap,
                 MEMFrameHeapFreeMode mode);

BOOL
MEMRecordStateForFrmHeap(FrameHeap *heap,
                         uint32_t tag);

BOOL
MEMFreeByStateToFrmHeap(FrameHeap *heap,
                        uint32_t tag);

uint32_t
MEMAdjustFrmHeap(FrameHeap *heap);

uint32_t
MEMResizeForMBlockFrmHeap(FrameHeap *heap,
                          uint32_t addr,
                          uint32_t size);

uint32_t
MEMGetAllocatableSizeForFrmHeap(FrameHeap *heap);

uint32_t
MEMGetAllocatableSizeForFrmHeapEx(FrameHeap *heap,
                                  int alignment);

/** @} */

} // namespace coreinit
