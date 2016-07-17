#pragma once
#include "common/types.h"
#include "coreinit_enum.h"
#include "coreinit_memheap.h"

namespace coreinit
{

/**
 * \defgroup coreinit_unitheap Unit Heap
 * \ingroup coreinit
 *
 * A unit heap is a memory heap where every allocation is of a fixed size
 * determined at heap creation.
 * @{
 */

#pragma pack(push, 1)

struct MEMUnitHeapFreeBlock
{
   be_ptr<MEMUnitHeapFreeBlock> next;
};
CHECK_OFFSET(MEMUnitHeapFreeBlock, 0x00, next);
CHECK_SIZE(MEMUnitHeapFreeBlock, 0x04);

struct MEMUnitHeap
{
   MEMHeapHeader header;
   be_ptr<MEMUnitHeapFreeBlock> freeBlocks;
   be_val<uint32_t> blockSize;
};
CHECK_OFFSET(MEMUnitHeap, 0x00, header);
CHECK_OFFSET(MEMUnitHeap, 0x40, freeBlocks);
CHECK_OFFSET(MEMUnitHeap, 0x44, blockSize);
CHECK_SIZE(MEMUnitHeap, 0x48);

#pragma pack(pop)

MEMUnitHeap *
MEMCreateUnitHeapEx(ppcaddr_t base,
                    uint32_t size,
                    uint32_t blockSize,
                    int32_t alignment,
                    uint32_t flags);

void *
MEMDestroyUnitHeap(MEMUnitHeap *heap);

void *
MEMAllocFromUnitHeap(MEMUnitHeap *heap);

void
MEMFreeToUnitHeap(MEMUnitHeap *heap,
                  void *block);

uint32_t
MEMCountFreeBlockForUnitHeap(MEMUnitHeap *heap);

uint32_t
MEMCalcHeapSizeForUnitHeap(uint32_t blockSize,
                           uint32_t count,
                           int alignment);

namespace internal
{

void
dumpUnitHeap(MEMUnitHeap *heap);

} // namespace internal

/** @} */

} // namespace coreinit
