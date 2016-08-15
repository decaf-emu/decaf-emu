#pragma once
#include "common/bitfield.h"
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

#pragma pack(push, 1)

BITFIELD(MEMHeapAttribs, uint32_t)
   BITFIELD_ENTRY(0, 1, bool, zeroAllocated);
   BITFIELD_ENTRY(1, 1, bool, debugMode);
   BITFIELD_ENTRY(2, 1, bool, useLock);
BITFIELD_END

struct MEMHeapHeader
{
   //! Tag indicating which type of heap this is
   be_val<MEMHeapTag> tag;

   //! Link for list this heap is in
   MEMListLink link;

   //! List of all child heaps in this heap
   MEMList list;

   //! Start address of allocatable memory
   be_ptr<uint8_t> dataStart;

   //! End address of allocatable memory
   be_ptr<uint8_t> dataEnd;

   //! Lock used when MEM_HEAP_FLAG_USE_LOCK is set.
   OSSpinLock lock;

   //! Flags set during heap creation.
   be_val<MEMHeapAttribs> attribs;

   UNKNOWN(0x0C);
};
CHECK_OFFSET(MEMHeapHeader, 0x00, tag);
CHECK_OFFSET(MEMHeapHeader, 0x04, link);
CHECK_OFFSET(MEMHeapHeader, 0x0C, list);
CHECK_OFFSET(MEMHeapHeader, 0x18, dataStart);
CHECK_OFFSET(MEMHeapHeader, 0x1C, dataEnd);
CHECK_OFFSET(MEMHeapHeader, 0x20, lock);
CHECK_OFFSET(MEMHeapHeader, 0x30, attribs);
CHECK_SIZE(MEMHeapHeader, 0x40);

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

MEMHeapHeader *
MEMCreateUserHeapHandle(MEMHeapHeader *heap,
                        uint32_t size);

uint32_t
MEMGetFillValForHeap(MEMHeapFillType type);

void
MEMSetFillValForHeap(MEMHeapFillType type,
                     uint32_t value);

/** @} */

namespace internal
{

class HeapLock
{
public:
   HeapLock(MEMHeapHeader *header)
   {
      auto attribs = header->attribs.value();

      if (attribs.useLock()) {
         OSUninterruptibleSpinLock_Acquire(&header->lock);
         mHeap = header;
      } else {
         mHeap = nullptr;
      }
   }

   ~HeapLock()
   {
      if (mHeap) {
         OSUninterruptibleSpinLock_Release(&mHeap->lock);
         mHeap = nullptr;
      }
   }

   void unlock()
   {
      if (mHeap) {
         OSUninterruptibleSpinLock_Release(&mHeap->lock);
         mHeap = nullptr;
      }
   }


private:
   MEMHeapHeader *mHeap;
};

void
initialiseDefaultHeaps();

void
registerHeap(MEMHeapHeader *heap,
             MEMHeapTag tag,
             uint8_t *dataStart,
             uint8_t *dataEnd,
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
