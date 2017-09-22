#pragma once
#include "coreinit_enum.h"
#include "coreinit_memheap.h"

#include <common/be_ptr.h>
#include <common/be_val.h>
#include <common/bitfield.h>
#include <common/cbool.h>
#include <cstdint>

namespace coreinit
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
   be_ptr<MEMBlockHeapBlock> blocks;

   //! Number of blocks in this tracking heap
   be_val<uint32_t> blockCount;
};
CHECK_OFFSET(MEMBlockHeapTracking, 0x08, blocks);
CHECK_OFFSET(MEMBlockHeapTracking, 0x0C, blockCount);
CHECK_SIZE(MEMBlockHeapTracking, 0x10);

struct MEMBlockHeapBlock
{
   //! First address of the data region this block has allocated
   be_ptr<uint8_t> start;

   //! End address of the data region this block has allocated
   be_ptr<uint8_t> end;

   //! TRUE if the block is free, FALSE if allocated
   be_val<BOOL> isFree;

   //! Link to previous block, note that this is only set for allocated blocks
   be_ptr<MEMBlockHeapBlock> prev;

   //! Link to next block, always set
   be_ptr<MEMBlockHeapBlock> next;
};
CHECK_OFFSET(MEMBlockHeapBlock, 0x00, start);
CHECK_OFFSET(MEMBlockHeapBlock, 0x04, end);
CHECK_OFFSET(MEMBlockHeapBlock, 0x08, isFree);
CHECK_OFFSET(MEMBlockHeapBlock, 0x0c, prev);
CHECK_OFFSET(MEMBlockHeapBlock, 0x10, next);
CHECK_SIZE(MEMBlockHeapBlock, 0x14);

struct MEMBlockHeap
{
   MEMHeapHeader header;

   //! Default tracking heap, tracks only defaultBlock
   MEMBlockHeapTracking defaultTrack;

   //! Default block, used so we don't have an empty block list
   MEMBlockHeapBlock defaultBlock;

   //! First block in this heap
   be_ptr<MEMBlockHeapBlock> firstBlock;

   //! Last block in this heap
   be_ptr<MEMBlockHeapBlock> lastBlock;

   //! First free block
   be_ptr<MEMBlockHeapBlock> firstFreeBlock;

   //! Free block count
   be_val<uint32_t> numFreeBlocks;
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

/*
TODO: MEMInitAllocatorForBlockHeap
*/

MEMBlockHeap *
MEMInitBlockHeap(MEMBlockHeap *heap,
                 void *start,
                 void *end,
                 MEMBlockHeapTracking *blocks,
                 uint32_t size,
                 uint32_t flags);

void *
MEMDestroyBlockHeap(MEMBlockHeap *heap);

int
MEMAddBlockHeapTracking(MEMBlockHeap *heap,
                        MEMBlockHeapTracking *tracking,
                        uint32_t size);

void *
MEMAllocFromBlockHeapAt(MEMBlockHeap *heap,
                        void *addr,
                        uint32_t size);

void *
MEMAllocFromBlockHeapEx(MEMBlockHeap *heap,
                        uint32_t size,
                        int32_t align);

void
MEMFreeToBlockHeap(MEMBlockHeap *heap,
                   void *data);

uint32_t
MEMGetAllocatableSizeForBlockHeapEx(MEMBlockHeap *heap,
                                    int32_t align);

uint32_t
MEMGetTrackingLeftInBlockHeap(MEMBlockHeap *heap);

uint32_t
MEMGetTotalFreeSizeForBlockHeap(MEMBlockHeap *heap);

/** @} */

} // namespace coreinit
