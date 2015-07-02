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

p32<FrameHeap>
MEMCreateFrmHeap(p32<FrameHeap> heap, uint32_t size);

p32<FrameHeap>
MEMCreateFrmHeapEx(p32<FrameHeap> heap, uint32_t size, uint16_t flags);

p32<void>
MEMDestroyFrmHeap(p32<FrameHeap> heap);

p32<void>
MEMAllocFromFrmHeap(p32<FrameHeap> heap, uint32_t size);

p32<void>
MEMAllocFromFrmHeapEx(p32<FrameHeap> heap, uint32_t size, int alignment);

void
MEMFreeToFrmHeap(p32<FrameHeap> heap, FrameHeapFreeMode::Flags mode);

BOOL
MEMRecordStateForFrmHeap(p32<FrameHeap> heap, uint32_t tag);

BOOL
MEMFreeByStateToFrmHeap(p32<FrameHeap> heap, uint32_t tag);

uint32_t
MEMAdjustFrmHeap(p32<FrameHeap> heap);

uint32_t
MEMResizeForMBlockFrmHeap(p32<FrameHeap> heap, uint32_t addr, uint32_t size);

uint32_t
MEMGetAllocatableSizeForFrmHeap(p32<FrameHeap> heap);

uint32_t
MEMGetAllocatableSizeForFrmHeapEx(p32<FrameHeap> heap, int alignment);
