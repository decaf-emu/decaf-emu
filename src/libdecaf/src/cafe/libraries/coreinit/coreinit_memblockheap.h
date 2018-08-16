#pragma once
#include "coreinit_enum.h"
#include "coreinit_memheap.h"

#include <libcpu/be2_struct.h>

namespace cafe::coreinit
{

/**
 * \defgroup coreinit_blockheap Block Heap
 * \ingroup coreinit
 * @{
 */

#pragma pack(push, 1)

struct MEMBlockHeapBlock;

struct MEMBlockHeapTracking
{
   UNKNOWN(0x8);

   //! Pointer to first memory block
   be2_virt_ptr<MEMBlockHeapBlock> blocks;

   //! Number of blocks in this tracking heap
   be2_val<uint32_t> blockCount;
};
CHECK_OFFSET(MEMBlockHeapTracking, 0x08, blocks);
CHECK_OFFSET(MEMBlockHeapTracking, 0x0C, blockCount);
CHECK_SIZE(MEMBlockHeapTracking, 0x10);

struct MEMBlockHeapBlock
{
   //! First address of the data region this block has allocated
   be2_virt_ptr<uint8_t> start;

   //! End address of the data region this block has allocated
   be2_virt_ptr<uint8_t> end;

   //! TRUE if the block is free, FALSE if allocated
   be2_val<BOOL> isFree;

   //! Link to previous block, note that this is only set for allocated blocks
   be2_virt_ptr<MEMBlockHeapBlock> prev;

   //! Link to next block, always set
   be2_virt_ptr<MEMBlockHeapBlock> next;
};
CHECK_OFFSET(MEMBlockHeapBlock, 0x00, start);
CHECK_OFFSET(MEMBlockHeapBlock, 0x04, end);
CHECK_OFFSET(MEMBlockHeapBlock, 0x08, isFree);
CHECK_OFFSET(MEMBlockHeapBlock, 0x0c, prev);
CHECK_OFFSET(MEMBlockHeapBlock, 0x10, next);
CHECK_SIZE(MEMBlockHeapBlock, 0x14);

struct MEMBlockHeap
{
   be2_struct<MEMHeapHeader> header;

   //! Default tracking heap, tracks only defaultBlock
   be2_struct<MEMBlockHeapTracking> defaultTrack;

   //! Default block, used so we don't have an empty block list
   be2_struct<MEMBlockHeapBlock> defaultBlock;

   //! First block in this heap
   be2_virt_ptr<MEMBlockHeapBlock> firstBlock;

   //! Last block in this heap
   be2_virt_ptr<MEMBlockHeapBlock> lastBlock;

   //! First free block
   be2_virt_ptr<MEMBlockHeapBlock> firstFreeBlock;

   //! Free block count
   be2_val<uint32_t> numFreeBlocks;
};
CHECK_OFFSET(MEMBlockHeap, 0x00, header);
CHECK_OFFSET(MEMBlockHeap, 0x40, defaultTrack);
CHECK_OFFSET(MEMBlockHeap, 0x50, defaultBlock);
CHECK_OFFSET(MEMBlockHeap, 0x64, firstBlock);
CHECK_OFFSET(MEMBlockHeap, 0x68, lastBlock);
CHECK_OFFSET(MEMBlockHeap, 0x6C, firstFreeBlock);
CHECK_OFFSET(MEMBlockHeap, 0x70, numFreeBlocks);
CHECK_SIZE(MEMBlockHeap, 0x74);

#pragma pack(pop)

MEMHeapHandle
MEMInitBlockHeap(virt_ptr<void> base,
                 virt_ptr<void> start,
                 virt_ptr<void> end,
                 virt_ptr<MEMBlockHeapTracking> tracking,
                 uint32_t size,
                 uint32_t flags);

virt_ptr<void>
MEMDestroyBlockHeap(MEMHeapHandle handle);

int
MEMAddBlockHeapTracking(MEMHeapHandle handle,
                        virt_ptr<MEMBlockHeapTracking> tracking,
                        uint32_t size);

virt_ptr<void>
MEMAllocFromBlockHeapAt(MEMHeapHandle handle,
                        virt_ptr<void> ptr,
                        uint32_t size);

virt_ptr<void>
MEMAllocFromBlockHeapEx(MEMHeapHandle handle,
                        uint32_t size,
                        int32_t align);

void
MEMFreeToBlockHeap(MEMHeapHandle handle,
                   virt_ptr<void> data);

uint32_t
MEMGetAllocatableSizeForBlockHeapEx(MEMHeapHandle handle,
                                    int32_t align);

uint32_t
MEMGetTrackingLeftInBlockHeap(MEMHeapHandle handle);

uint32_t
MEMGetTotalFreeSizeForBlockHeap(MEMHeapHandle handle);

/** @} */

} // namespace cafe::coreinit
