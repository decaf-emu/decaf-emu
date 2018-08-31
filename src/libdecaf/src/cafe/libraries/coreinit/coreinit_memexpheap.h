#pragma once
#include "coreinit_enum.h"
#include "coreinit_memheap.h"

#include <common/bitfield.h>
#include <libcpu/be2_struct.h>

namespace cafe::coreinit
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
   be2_val<MEMExpHeapBlockAttribs> attribs;
   be2_val<uint32_t> blockSize;
   be2_virt_ptr<MEMExpHeapBlock> prev;
   be2_virt_ptr<MEMExpHeapBlock> next;
   be2_val<uint16_t> tag;
   PADDING(0x02);
};
CHECK_OFFSET(MEMExpHeapBlock, 0x00, attribs);
CHECK_OFFSET(MEMExpHeapBlock, 0x04, blockSize);
CHECK_OFFSET(MEMExpHeapBlock, 0x08, prev);
CHECK_OFFSET(MEMExpHeapBlock, 0x0c, next);
CHECK_OFFSET(MEMExpHeapBlock, 0x10, tag);
CHECK_SIZE(MEMExpHeapBlock, 0x14);

struct MEMExpHeapBlockList
{
   be2_virt_ptr<MEMExpHeapBlock> head;
   be2_virt_ptr<MEMExpHeapBlock> tail;
};
CHECK_OFFSET(MEMExpHeapBlockList, 0x00, head);
CHECK_OFFSET(MEMExpHeapBlockList, 0x04, tail);
CHECK_SIZE(MEMExpHeapBlockList, 0x08);

struct MEMExpHeap
{
   be2_struct<MEMHeapHeader> header;
   be2_struct<MEMExpHeapBlockList> freeList;
   be2_struct<MEMExpHeapBlockList> usedList;
   be2_val<uint16_t> groupId;
   be2_val<MEMExpHeapAttribs> attribs;
};
CHECK_OFFSET(MEMExpHeap, 0x00, header);
CHECK_OFFSET(MEMExpHeap, 0x40, freeList);
CHECK_OFFSET(MEMExpHeap, 0x48, usedList);
CHECK_OFFSET(MEMExpHeap, 0x50, groupId);
CHECK_OFFSET(MEMExpHeap, 0x52, attribs);
CHECK_SIZE(MEMExpHeap, 0x54);

#pragma pack(pop)

MEMHeapHandle
MEMCreateExpHeapEx(virt_ptr<void> base,
                   uint32_t size,
                   uint32_t flags);

virt_ptr<void>
MEMDestroyExpHeap(MEMHeapHandle handle);

virt_ptr<void>
MEMAllocFromExpHeapEx(MEMHeapHandle handle,
                      uint32_t size,
                      int32_t alignment);

void
MEMFreeToExpHeap(MEMHeapHandle handle,
                 virt_ptr<void> block);

MEMExpHeapMode
MEMSetAllocModeForExpHeap(MEMHeapHandle handle,
                          MEMExpHeapMode mode);

MEMExpHeapMode
MEMGetAllocModeForExpHeap(MEMHeapHandle handle);

uint32_t
MEMAdjustExpHeap(MEMHeapHandle handle);

uint32_t
MEMResizeForMBlockExpHeap(MEMHeapHandle handle,
                          virt_ptr<void> block,
                          uint32_t size);

uint32_t
MEMGetTotalFreeSizeForExpHeap(MEMHeapHandle handle);

uint32_t
MEMGetAllocatableSizeForExpHeapEx(MEMHeapHandle handle,
                                  int32_t alignment);

uint16_t
MEMSetGroupIDForExpHeap(MEMHeapHandle handle,
                        uint16_t id);

uint16_t
MEMGetGroupIDForExpHeap(MEMHeapHandle handle);

uint32_t
MEMGetSizeForMBlockExpHeap(virt_ptr<void> block);

uint16_t
MEMGetGroupIDForMBlockExpHeap(virt_ptr<void> block);

MEMExpHeapDirection
MEMGetAllocDirForMBlockExpHeap(virt_ptr<void> block);

namespace internal
{

void
dumpExpandedHeap(virt_ptr<MEMExpHeap> handle);

} // namespace internal

/** @} */

} // namespace cafe::coreinit
