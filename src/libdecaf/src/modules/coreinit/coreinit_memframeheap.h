#pragma once
#include "common/types.h"
#include "coreinit_enum.h"
#include "coreinit_memory.h"

namespace coreinit
{

/**
 * \defgroup coreinit_frameheap Frame Heap
 * \ingroup coreinit
 * @{
 */

#pragma pack(push, 1)

struct MEMFrameHeapState
{
   be_val<uint32_t> tag;
   be_val<ppcaddr_t> head;
   be_val<ppcaddr_t> tail;
   be_ptr<MEMFrameHeapState> previous;
};
CHECK_OFFSET(MEMFrameHeapState, 0x00, tag);
CHECK_OFFSET(MEMFrameHeapState, 0x04, head);
CHECK_OFFSET(MEMFrameHeapState, 0x08, tail);
CHECK_OFFSET(MEMFrameHeapState, 0x0C, previous);
CHECK_SIZE(MEMFrameHeapState, 0x10);

struct MEMFrameHeap
{
   MEMHeapHeader header;
   be_val<ppcaddr_t> head;
   be_val<ppcaddr_t> tail;
   be_ptr<MEMFrameHeapState> previousState;
};
CHECK_OFFSET(MEMFrameHeap, 0x00, header);
CHECK_OFFSET(MEMFrameHeap, 0x40, head);
CHECK_OFFSET(MEMFrameHeap, 0x44, tail);
CHECK_OFFSET(MEMFrameHeap, 0x48, previousState);
CHECK_SIZE(MEMFrameHeap, 0x4C);

#pragma pack(pop)

MEMFrameHeap *
MEMCreateFrmHeapEx(ppcaddr_t base,
                   uint32_t size,
                   uint32_t flags);

void *
MEMDestroyFrmHeap(MEMFrameHeap *heap);

void *
MEMAllocFromFrmHeapEx(MEMFrameHeap *heap,
                      uint32_t size,
                      int alignment);

void
MEMFreeToFrmHeap(MEMFrameHeap *heap,
                 MEMFrameHeapFreeMode mode);

BOOL
MEMRecordStateForFrmHeap(MEMFrameHeap *heap,
                         uint32_t tag);

BOOL
MEMFreeByStateToFrmHeap(MEMFrameHeap *heap,
                        uint32_t tag);

uint32_t
MEMAdjustFrmHeap(MEMFrameHeap *heap);

uint32_t
MEMResizeForMBlockFrmHeap(MEMFrameHeap *heap,
                          uint32_t addr,
                          uint32_t size);

uint32_t
MEMGetAllocatableSizeForFrmHeapEx(MEMFrameHeap *heap,
                                  int alignment);

/** @} */

} // namespace coreinit
