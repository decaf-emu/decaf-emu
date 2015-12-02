#pragma once
#include "types.h"
#include "coreinit_enum.h"
#include "coreinit_memlist.h"
#include "coreinit_spinlock.h"
#include "utils/structsize.h"
#include "utils/wfunc_ptr.h"

static const uint32_t MEM_MAX_HEAP_TABLE = 0x20;

#pragma pack(push, 1)

struct CommonHeap
{
   be_val<MEMiHeapTag> tag;
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
MEMiInitHeapHead(CommonHeap *heap, MEMiHeapTag tag, uint32_t dataStart, uint32_t dataEnd);

void
MEMiFinaliseHeap(CommonHeap *heap);

void
MEMDumpHeap(CommonHeap *heap);

CommonHeap *
MEMFindContainHeap(void *block);

MEMBaseHeapType
MEMGetArena(CommonHeap *heap);

CommonHeap *
MEMGetBaseHeapHandle(MEMBaseHeapType type);

CommonHeap *
MEMSetBaseHeapHandle(MEMBaseHeapType type, CommonHeap *heap);

char *
OSStringFromSystem(const std::string &src);

void
CoreFreeDefaultHeap();

void
CoreInitDefaultHeap();

// TODO: Move These!
void *
OSAllocFromSystem(size_t size, int alignment = 4);

template<typename Type>
Type *
OSAllocFromSystem(int alignment = 4)
{
   return new (OSAllocFromSystem(sizeof(Type), alignment)) Type();
}

void
OSFreeToSystem(void *addr);
