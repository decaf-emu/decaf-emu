#pragma once
#include "coreinit_enum.h"
#include "coreinit_memory.h"

#include <common/be_ptr.h>
#include <common/be_val.h>
#include <common/cbool.h>
#include <cstdint>

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
   //! Tag used to identify the state for MEMFreeByStateToFrmHeap.
   be_val<uint32_t> tag;

   //! Saved head address for frame heap.
   be_ptr<uint8_t> head;

   //! Saved tail address for frame heap.
   be_ptr<uint8_t> tail;

   //! Pointer to the previous recorded frame heap state.
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

   //! Current address of the head of the frame heap.
   be_ptr<uint8_t> head;

   //! Current address of the tail of the frame heap.
   be_ptr<uint8_t> tail;

   //! Pointer to the previous recorded frame heap state.
   be_ptr<MEMFrameHeapState> previousState;
};
CHECK_OFFSET(MEMFrameHeap, 0x00, header);
CHECK_OFFSET(MEMFrameHeap, 0x40, head);
CHECK_OFFSET(MEMFrameHeap, 0x44, tail);
CHECK_OFFSET(MEMFrameHeap, 0x48, previousState);
CHECK_SIZE(MEMFrameHeap, 0x4C);

#pragma pack(pop)

MEMFrameHeap *
MEMCreateFrmHeapEx(void *base,
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
                          void *address,
                          uint32_t size);

uint32_t
MEMGetAllocatableSizeForFrmHeapEx(MEMFrameHeap *heap,
                                  int alignment);

/** @} */

} // namespace coreinit
