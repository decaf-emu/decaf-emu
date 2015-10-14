#pragma once
#include "coreinit_memlist.h"
#include "coreinit_spinlock.h"
#include "structsize.h"
#include "wfunc_ptr.h"

enum class BaseHeapType : uint32_t
{
   Min = 0,
   MEM1 = 0,
   MEM2 = 1,
   FG = 8,
   Max = 9,
   Invalid = 10
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

struct CommonHeap
{
   be_val<HeapType> tag;
   MemoryLink link;
   MemoryList list;
   be_val<uint32_t> dataStart;
   be_val<uint32_t> dataEnd;
   OSSpinLock lock;
   be_val<uint32_t> flags;
};

CHECK_OFFSET(CommonHeap, 0x0, tag);
CHECK_OFFSET(CommonHeap, 0x4, link);
CHECK_OFFSET(CommonHeap, 0xc, list);
CHECK_OFFSET(CommonHeap, 0x18, dataStart);
CHECK_OFFSET(CommonHeap, 0x1c, dataEnd);
CHECK_OFFSET(CommonHeap, 0x20, lock);
CHECK_OFFSET(CommonHeap, 0x30, flags);
CHECK_SIZE(CommonHeap, 0x34);

#pragma pack(pop)

extern be_wfunc_ptr<void*, uint32_t>*
pMEMAllocFromDefaultHeap;

extern be_wfunc_ptr<void*, uint32_t, int>*
pMEMAllocFromDefaultHeapEx;

extern be_wfunc_ptr<void, void*>*
pMEMFreeToDefaultHeap;

void
MEMiInitHeapHead(CommonHeap *heap, HeapType type, uint32_t dataStart, uint32_t dataEnd);

void
MEMiFinaliseHeap(CommonHeap *heap);

void
MEMDumpHeap(CommonHeap *heap);

CommonHeap *
MEMFindContainHeap(void *block);

BaseHeapType
MEMGetArena(CommonHeap *heap);

CommonHeap *
MEMGetBaseHeapHandle(BaseHeapType arena);

CommonHeap *
MEMSetBaseHeapHandle(BaseHeapType arena, CommonHeap *heap);

char *
OSSprintfFromSystem(const char *format, ...);

void
CoreFreeDefaultHeap();

void
CoreInitDefaultHeap();

// TODO: Move These!
void *
OSAllocFromSystem(uint32_t size, int alignment = 4);

template<typename Type>
Type *
OSAllocFromSystem(int alignment = 4)
{
   return new (OSAllocFromSystem(sizeof(Type), alignment)) Type();
}

void
OSFreeToSystem(void *addr);
