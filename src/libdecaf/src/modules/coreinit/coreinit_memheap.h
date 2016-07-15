#pragma once
#include "common/types.h"
#include "coreinit_enum.h"
#include "coreinit_memlist.h"
#include "coreinit_spinlock.h"
#include "common/structsize.h"
#include "ppcutils/wfunc_ptr.h"

namespace coreinit
{

/**
 * \defgroup coreinit_memheap Memory Heap
 * \ingroup coreinit
 * @{
 */

static const uint32_t MEM_MAX_HEAP_TABLE = 0x20;

#pragma pack(push, 1)

struct MEMHeapHeader
{
   be_val<MEMHeapTag> tag;
   MEMListLink link;
   MEMList list;
   be_val<uint32_t> dataStart;
   be_val<uint32_t> dataEnd;
   OSSpinLock lock;
   be_val<uint32_t> flags;
};

CHECK_OFFSET(MEMHeapHeader, 0x0, tag);
CHECK_OFFSET(MEMHeapHeader, 0x4, link);
CHECK_OFFSET(MEMHeapHeader, 0xc, list);
CHECK_OFFSET(MEMHeapHeader, 0x18, dataStart);
CHECK_OFFSET(MEMHeapHeader, 0x1c, dataEnd);
CHECK_OFFSET(MEMHeapHeader, 0x20, lock);
CHECK_OFFSET(MEMHeapHeader, 0x30, flags);
CHECK_SIZE(MEMHeapHeader, 0x34);

#pragma pack(pop)

extern be_wfunc_ptr<void*, uint32_t>*
pMEMAllocFromDefaultHeap;

extern be_wfunc_ptr<void*, uint32_t, int>*
pMEMAllocFromDefaultHeapEx;

extern be_wfunc_ptr<void, void*>*
pMEMFreeToDefaultHeap;

void
MEMDumpHeap(MEMHeapHeader *heap);

MEMHeapHeader *
MEMFindContainHeap(void *block);

MEMBaseHeapType
MEMGetArena(MEMHeapHeader *heap);

MEMHeapHeader *
MEMGetBaseHeapHandle(MEMBaseHeapType type);

MEMHeapHeader *
MEMSetBaseHeapHandle(MEMBaseHeapType type,
                     MEMHeapHeader *heap);

/** @} */

namespace internal
{

void
initialiseDefaultHeaps();

void
registerHeap(MEMHeapHeader *heap,
             MEMHeapTag tag,
             uint32_t dataStart,
             uint32_t dataEnd,
             uint32_t flags);

void
unregisterHeap(MEMHeapHeader *heap);

void *
sysAlloc(size_t size,
         int alignment = 4);

void
sysFree(void *addr);

template<typename Type>
Type *
sysAlloc(int alignment = 4)
{
   return new (sysAlloc(sizeof(Type), alignment)) Type();
}

char *
sysStrDup(const std::string &src);

void *
allocFromDefaultHeap(uint32_t size);

void *
allocFromDefaultHeapEx(uint32_t size,
                       int32_t alignment);

void
freeToDefaultHeap(void *block);

} // namespace internal

} // namespace coreinit
