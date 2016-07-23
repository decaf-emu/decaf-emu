#pragma once
#include "common/bitfield.h"
#include "common/types.h"
#include "coreinit_enum.h"
#include "coreinit_memheap.h"

namespace coreinit
{

/**
 * \defgroup coreinit_expheap Expanded Heap
 * \ingroup coreinit
 * @{
 */

BITFIELD(MEMExpHeapAttribs, uint16_t)
   BITFIELD_ENTRY(0, 1, MEMExpHeapMode, allocMode);
   BITFIELD_ENTRY(1, 1, bool, reuseAlignSpace);
BITFIELD_END

BITFIELD(MEMExpHeapBlockAttribs, uint32_t)
   BITFIELD_ENTRY(0, 8, uint8_t, groupId);
   BITFIELD_ENTRY(8, 23, uint32_t, alignment);
   BITFIELD_ENTRY(31, 1, MEMExpHeapDirection, allocDir);
BITFIELD_END

#pragma pack(push, 1)

struct MEMExpHeapBlock
{
   be_val<MEMExpHeapBlockAttribs> attribs;

   be_val<uint32_t> blockSize;

   be_ptr<MEMExpHeapBlock> prev;

   be_ptr<MEMExpHeapBlock> next;

   be_val<uint16_t> tag;

   UNKNOWN(0x02);
};
CHECK_OFFSET(MEMExpHeapBlock, 0x00, attribs);
CHECK_OFFSET(MEMExpHeapBlock, 0x04, blockSize);
CHECK_OFFSET(MEMExpHeapBlock, 0x08, prev);
CHECK_OFFSET(MEMExpHeapBlock, 0x0c, next);
CHECK_OFFSET(MEMExpHeapBlock, 0x10, tag);
CHECK_SIZE(MEMExpHeapBlock, 0x14);

struct MEMExpHeapBlockList
{
   be_ptr<MEMExpHeapBlock> head;
   be_ptr<MEMExpHeapBlock> tail;
};
CHECK_OFFSET(MEMExpHeapBlockList, 0x00, head);
CHECK_OFFSET(MEMExpHeapBlockList, 0x04, tail);
CHECK_SIZE(MEMExpHeapBlockList, 0x08);

struct MEMExpHeap
{
   MEMHeapHeader header;

   MEMExpHeapBlockList freeList;
   MEMExpHeapBlockList usedList;

   be_val<uint16_t> groupId;

   be_val<MEMExpHeapAttribs> attribs;
};
CHECK_OFFSET(MEMExpHeap, 0x00, header);
CHECK_OFFSET(MEMExpHeap, 0x40, freeList);
CHECK_OFFSET(MEMExpHeap, 0x48, usedList);
CHECK_OFFSET(MEMExpHeap, 0x50, groupId);
CHECK_OFFSET(MEMExpHeap, 0x52, attribs);
CHECK_SIZE(MEMExpHeap, 0x54);

#pragma pack(pop)

MEMExpHeap *
MEMCreateExpHeapEx(void *base,
                   uint32_t size,
                   uint32_t flags);

void *
MEMDestroyExpHeap(MEMExpHeap *heap);

void *
MEMAllocFromExpHeapEx(MEMExpHeap *heap,
                      uint32_t size,
                      int32_t alignment);

void
MEMFreeToExpHeap(MEMExpHeap *heap,
                 void *block);

MEMExpHeapMode
MEMSetAllocModeForExpHeap(MEMExpHeap *heap,
                          MEMExpHeapMode mode);

MEMExpHeapMode
MEMGetAllocModeForExpHeap(MEMExpHeap *heap);

uint32_t
MEMAdjustExpHeap(MEMExpHeap *heap);

uint32_t
MEMResizeForMBlockExpHeap(MEMExpHeap *heap,
                          uint8_t *address,
                          uint32_t size);

uint32_t
MEMGetTotalFreeSizeForExpHeap(MEMExpHeap *heap);

uint32_t
MEMGetAllocatableSizeForExpHeapEx(MEMExpHeap *heap,
                                  int32_t alignment);

uint16_t
MEMSetGroupIDForExpHeap(MEMExpHeap *heap,
                        uint16_t id);

uint16_t
MEMGetGroupIDForExpHeap(MEMExpHeap *heap);

uint32_t
MEMGetSizeForMBlockExpHeap(uint8_t *addr);

uint16_t
MEMGetGroupIDForMBlockExpHeap(uint8_t *addr);

MEMExpHeapDirection
MEMGetAllocDirForMBlockExpHeap(uint8_t *addr);

namespace internal
{

void
dumpExpandedHeap(MEMExpHeap *heap);

} // namespace internal

/** @} */

} // namespace coreinit
