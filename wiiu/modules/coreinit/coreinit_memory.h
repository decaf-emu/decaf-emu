#pragma once
#include "systemtypes.h"
#include "heapmanager.h"

enum class BaseHeapType : uint32_t
{
   Min = 0,
   MEM1 = 0,
   MEM2 = 1,
   FG = 8,
   Max = 9,
   Invalid = 10
};

enum class OSMemoryType : uint32_t
{
   MEM1 = 1,
   MEM2 = 2,
   System = 1234567
};

void
CoreInitDefaultHeap();

void
CoreFreeDefaultHeap();

WHeapHandle
MEMGetBaseHeapHandle(BaseHeapType arena);

WHeapHandle
MEMSetBaseHeapHandle(BaseHeapType arena, WHeapHandle handle);

BaseHeapType
MEMGetArena(WHeapHandle handle);

p32<void>
OSAllocFromSystem(uint32_t size, int alignment = 4);

void
OSFreeToSystem(p32<void> addr);

p32<void>
OSBlockMove(void *dst, const void *src, size_t size, BOOL flush);

p32<void>
OSBlockSet(void *dst, uint8_t val, size_t size);

BOOL
OSGetForegroundBucket(uint32_t *addr, uint32_t *size);

BOOL
OSGetForegroundBucketFreeArea(uint32_t *addr, uint32_t *size);

int
OSGetMemBound(OSMemoryType type, uint32_t *addr, uint32_t *size);

extern p32<be_val<uint32_t>>
pMEMAllocFromDefaultHeap;

extern p32<be_val<uint32_t>>
pMEMAllocFromDefaultHeapEx;

extern p32<be_val<uint32_t>>
pMEMFreeToDefaultHeap;
