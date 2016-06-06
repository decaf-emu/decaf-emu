#pragma once
#include "types.h"
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

struct UnitHeap;

UnitHeap *
MEMCreateUnitHeapEx(UnitHeap *heap,
                    uint32_t size,
                    uint32_t blockSize,
                    int32_t alignment,
                    uint16_t flags);

void *
MEMDestroyUnitHeap(UnitHeap *heap);

void *
MEMAllocFromUnitHeap(UnitHeap *heap);

void
MEMFreeToUnitHeap(UnitHeap *heap,
                  void *block);

void
MEMiDumpUnitHeap(UnitHeap *heap);

uint32_t
MEMCountFreeBlockForUnitHeap(UnitHeap *heap);

uint32_t
MEMCalcHeapSizeForUnitHeap(uint32_t blockSize,
                           uint32_t count,
                           int alignment);

/** @} */

} // namespace coreinit
