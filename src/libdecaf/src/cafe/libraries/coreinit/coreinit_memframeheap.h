#pragma once
#include "coreinit_enum.h"
#include "coreinit_memheap.h"

#include <libcpu/be2_val.h>

namespace cafe::coreinit
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
   be2_val<uint32_t> tag;

   //! Saved head address for frame heap.
   be2_virt_ptr<uint8_t> head;

   //! Saved tail address for frame heap.
   be2_virt_ptr<uint8_t> tail;

   //! Pointer to the previous recorded frame heap state.
   be2_virt_ptr<MEMFrameHeapState> previous;
};
CHECK_OFFSET(MEMFrameHeapState, 0x00, tag);
CHECK_OFFSET(MEMFrameHeapState, 0x04, head);
CHECK_OFFSET(MEMFrameHeapState, 0x08, tail);
CHECK_OFFSET(MEMFrameHeapState, 0x0C, previous);
CHECK_SIZE(MEMFrameHeapState, 0x10);

struct MEMFrameHeap
{
   be2_struct<MEMHeapHeader> header;

   //! Current address of the head of the frame heap.
   be2_virt_ptr<uint8_t> head;

   //! Current address of the tail of the frame heap.
   be2_virt_ptr<uint8_t> tail;

   //! Pointer to the previous recorded frame heap state.
   be2_virt_ptr<MEMFrameHeapState> previousState;
};
CHECK_OFFSET(MEMFrameHeap, 0x00, header);
CHECK_OFFSET(MEMFrameHeap, 0x40, head);
CHECK_OFFSET(MEMFrameHeap, 0x44, tail);
CHECK_OFFSET(MEMFrameHeap, 0x48, previousState);
CHECK_SIZE(MEMFrameHeap, 0x4C);

#pragma pack(pop)

MEMHeapHandle
MEMCreateFrmHeapEx(virt_ptr<void> base,
                   uint32_t size,
                   uint32_t flags);

virt_ptr<void>
MEMDestroyFrmHeap(MEMHeapHandle heap);

virt_ptr<void>
MEMAllocFromFrmHeapEx(MEMHeapHandle heap,
                      uint32_t size,
                      int alignment);

void
MEMFreeToFrmHeap(MEMHeapHandle heap,
                 MEMFrameHeapFreeMode mode);

BOOL
MEMRecordStateForFrmHeap(MEMHeapHandle heap,
                         uint32_t tag);

BOOL
MEMFreeByStateToFrmHeap(MEMHeapHandle heap,
                        uint32_t tag);

uint32_t
MEMAdjustFrmHeap(MEMHeapHandle heap);

uint32_t
MEMResizeForMBlockFrmHeap(MEMHeapHandle heap,
                          virt_ptr<void> address,
                          uint32_t size);

uint32_t
MEMGetAllocatableSizeForFrmHeapEx(MEMHeapHandle heap,
                                  int alignment);

/** @} */

} // namespace cafe::coreinit
