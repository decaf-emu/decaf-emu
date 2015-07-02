#pragma once
#include "systemtypes.h"
#include "coreinit_spinlock.h"
#include "coreinit_memlist.h"

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

enum class HeapMode : uint8_t
{
   FirstFree,
   NearestSize
};

enum class HeapDirection : uint8_t
{
   FromTop,
   FromBottom
};

static const uint32_t MEM_MAX_HEAP_TABLE = 0x20;

enum class HeapType : uint32_t
{
   ExpandedHeap = 0x45585048,
   FrameHeap = 0x46524D48,
   UnitHeap = 0x554E5448,
   UserHeap = 0x55535248,
   BlockHeap = 0x424C4B48
};

#pragma pack(push, 1)

struct MemoryHeapCommon
{
   be_val<HeapType> tag;
   MemoryLink link;
   MemoryList list;
   be_val<uint32_t> dataStart;
   be_val<uint32_t> dataEnd;
   OSSpinLock lock;
   be_val<uint32_t> flags;
};

CHECK_OFFSET(MemoryHeapCommon, 0x0, tag);
CHECK_OFFSET(MemoryHeapCommon, 0x4, link);
CHECK_OFFSET(MemoryHeapCommon, 0xc, list);
CHECK_OFFSET(MemoryHeapCommon, 0x18, dataStart);
CHECK_OFFSET(MemoryHeapCommon, 0x1c, dataEnd);
CHECK_OFFSET(MemoryHeapCommon, 0x20, lock);
CHECK_OFFSET(MemoryHeapCommon, 0x30, flags);
CHECK_SIZE(MemoryHeapCommon, 0x34);

#pragma pack(pop)

using HeapHandle = p32<void>;

void
CoreInitDefaultHeap();

void
CoreFreeDefaultHeap();

void
MEMiInitHeapHead(MemoryHeapCommon *heap, HeapType type, uint32_t dataStart, uint32_t dataEnd);

HeapHandle
MEMGetBaseHeapHandle(BaseHeapType arena);

HeapHandle
MEMSetBaseHeapHandle(BaseHeapType arena, HeapHandle handle);

BaseHeapType
MEMGetArena(HeapHandle handle);

p32<void>
OSAllocFromSystem(uint32_t size, int alignment = 4);

void
OSFreeToSystem(p32<void> addr);

void *
OSBlockMove(void *dst, const void *src, size_t size, BOOL flush);

void *
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
