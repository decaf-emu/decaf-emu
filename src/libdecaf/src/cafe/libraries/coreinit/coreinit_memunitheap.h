#pragma once
#include "coreinit_enum.h"
#include "coreinit_memheap.h"

#include <libcpu/be2_struct.h>

namespace cafe::coreinit
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
   be2_ptr<MEMUnitHeapFreeBlock> next;
};
CHECK_OFFSET(MEMUnitHeapFreeBlock, 0x00, next);
CHECK_SIZE(MEMUnitHeapFreeBlock, 0x04);

struct MEMUnitHeap
{
   be2_struct<MEMHeapHeader> header;
   be2_ptr<MEMUnitHeapFreeBlock> freeBlocks;
   be2_val<uint32_t> blockSize;
};
CHECK_OFFSET(MEMUnitHeap, 0x00, header);
CHECK_OFFSET(MEMUnitHeap, 0x40, freeBlocks);
CHECK_OFFSET(MEMUnitHeap, 0x44, blockSize);
CHECK_SIZE(MEMUnitHeap, 0x48);

#pragma pack(pop)

MEMHeapHandle
MEMCreateUnitHeapEx(virt_ptr<void> base,
                    uint32_t size,
                    uint32_t blockSize,
                    int32_t alignment,
                    uint32_t flags);

virt_ptr<void>
MEMDestroyUnitHeap(MEMHeapHandle handle);

virt_ptr<void>
MEMAllocFromUnitHeap(MEMHeapHandle handle);

void
MEMFreeToUnitHeap(MEMHeapHandle handle,
                  virt_ptr<void> block);

uint32_t
MEMCountFreeBlockForUnitHeap(MEMHeapHandle handle);

uint32_t
MEMCalcHeapSizeForUnitHeap(uint32_t blockSize,
                           uint32_t count,
                           int alignment);

namespace internal
{

void
dumpUnitHeap(virt_ptr<MEMUnitHeap> heap);

} // namespace internal

/** @} */

} // namespace cafe::coreinit
