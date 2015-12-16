#pragma once
#include "types.h"
#include "coreinit_enum.h"
#include "coreinit_memheap.h"

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
MEMFreeToUnitHeap(UnitHeap *heap, void *block);

void
MEMiDumpUnitHeap(UnitHeap *heap);

uint32_t
MEMCountFreeBlockForUnitHeap(UnitHeap *heap);

uint32_t
MEMCalcHeapSizeForUnitHeap(uint32_t blockSize,
                           uint32_t count,
                           int alignment);
