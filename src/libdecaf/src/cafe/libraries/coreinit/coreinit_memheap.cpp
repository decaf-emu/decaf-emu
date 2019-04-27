#include "coreinit.h"
#include "coreinit_memexpheap.h"
#include "coreinit_memheap.h"
#include "coreinit_memlist.h"
#include "coreinit_memory.h"
#include "coreinit_memunitheap.h"
#include "coreinit_spinlock.h"
#include "cafe/cafe_stackobject.h"

#include <algorithm>
#include <array>
#include <common/log.h>

namespace cafe::coreinit
{

struct StaticMemHeapData
{
   be2_val<BOOL> initialisedLock;
   be2_struct<OSSpinLock> lock;

   be2_val<BOOL> initialisedLists;
   be2_struct<MEMList> foregroundList;
   be2_struct<MEMList> mem1List;
   be2_struct<MEMList> mem2List;

   be2_array<MEMHeapHandle, MEMBaseHeapType::Max> arenas;
   be2_array<uint32_t, MEMHeapFillType::Max> fillValues;
};

static virt_ptr<StaticMemHeapData>
sMemHeapData = nullptr;

static virt_ptr<MEMList>
findListContainingHeap(virt_ptr<MEMHeapHeader> heap)
{
   auto start = StackObject<virt_addr> { };
   auto end = StackObject<virt_addr> { };
   auto size = StackObject<uint32_t> { };
   OSGetForegroundBucket(start, size);
   *end = *start + *size;

   if (virt_cast<virt_addr>(heap->dataStart) >= *start &&
       virt_cast<virt_addr>(heap->dataEnd) <= *end) {
      return virt_addrof(sMemHeapData->foregroundList);
   }

   OSGetMemBound(OSMemoryType::MEM1, start, size);
   *end = *start + *size;

   if (virt_cast<virt_addr>(heap->dataStart) >= *start &&
       virt_cast<virt_addr>(heap->dataEnd) <= *end) {
      return virt_addrof(sMemHeapData->mem1List);
   } else {
      return virt_addrof(sMemHeapData->mem2List);
   }
}

static virt_ptr<MEMList>
findListContainingBlock(virt_ptr<void> block)
{
   auto start = StackObject<virt_addr> { };
   auto end = StackObject<virt_addr> { };
   auto size = StackObject<uint32_t> { };
   OSGetForegroundBucket(start, size);
   *end = *start + *size;

   if (virt_cast<virt_addr>(block) >= *start &&
       virt_cast<virt_addr>(block) <= *end) {
      return virt_addrof(sMemHeapData->foregroundList);
   }

   OSGetMemBound(OSMemoryType::MEM1, start, size);
   *end = *start + *size;

   if (virt_cast<virt_addr>(block) >= *start &&
       virt_cast<virt_addr>(block) <= *end) {
      return virt_addrof(sMemHeapData->mem1List);
   } else {
      return virt_addrof(sMemHeapData->mem2List);
   }
}

static virt_ptr<MEMHeapHeader>
findHeapContainingBlock(virt_ptr<MEMList> list,
                        virt_ptr<void> block)
{
   virt_ptr<MEMHeapHeader> heap = nullptr;

   while ((heap = virt_cast<MEMHeapHeader *>(MEMGetNextListObject(list, heap)))) {
      if (virt_cast<virt_addr>(block) >= virt_cast<virt_addr>(heap->dataStart) &&
          virt_cast<virt_addr>(block) < virt_cast<virt_addr>(heap->dataEnd)) {
         auto child = findHeapContainingBlock(virt_addrof(heap->list), block);
         return child ? child : heap;
      }
   }

   return nullptr;
}

void
MEMDumpHeap(virt_ptr<MEMHeapHeader> heap)
{
   switch (heap->tag) {
   case MEMHeapTag::ExpandedHeap:
      internal::dumpExpandedHeap(virt_cast<MEMExpHeap *>(heap));
      break;
   case MEMHeapTag::UnitHeap:
      internal::dumpUnitHeap(virt_cast<MEMUnitHeap *>(heap));
      break;
   case MEMHeapTag::FrameHeap:
   case MEMHeapTag::UserHeap:
   case MEMHeapTag::BlockHeap:
      gLog->warn("Unimplemented MEMDumpHeap for tag {:08x}", heap->tag);
   }
}

virt_ptr<MEMHeapHeader>
MEMFindContainHeap(virt_ptr<void> block)
{
   if (auto list = findListContainingBlock(block)) {
      return findHeapContainingBlock(list, block);
   }

   return nullptr;
}

MEMBaseHeapType
MEMGetArena(virt_ptr<MEMHeapHeader> heap)
{
   for (auto i = 0u; i < sMemHeapData->arenas.size(); ++i) {
      if (sMemHeapData->arenas[i] == heap) {
         return static_cast<MEMBaseHeapType>(i);
      }
   }

   return MEMBaseHeapType::Invalid;
}

MEMHeapHandle
MEMGetBaseHeapHandle(MEMBaseHeapType type)
{
   if (type < sMemHeapData->arenas.size()) {
      return sMemHeapData->arenas[type];
   } else {
      return nullptr;
   }
}

MEMHeapHandle
MEMSetBaseHeapHandle(MEMBaseHeapType type,
                     MEMHeapHandle heap)
{
   if (type < sMemHeapData->arenas.size()) {
      auto previous = sMemHeapData->arenas[type];
      sMemHeapData->arenas[type] = heap;
      return previous;
   } else {
      return nullptr;
   }
}

MEMHeapHandle
MEMCreateUserHeapHandle(virt_ptr<MEMHeapHeader> heap,
                        uint32_t size)
{
   auto dataStart = virt_cast<uint8_t *>(heap) + sizeof(MEMHeapHeader);
   auto dataEnd = dataStart + size;

   internal::registerHeap(heap,
                          coreinit::MEMHeapTag::UserHeap,
                          dataStart,
                          dataEnd,
                          MEMHeapFlags::None);

   return heap;
}

uint32_t
MEMGetFillValForHeap(MEMHeapFillType type)
{
   OSUninterruptibleSpinLock_Acquire(virt_addrof(sMemHeapData->lock));
   auto value = sMemHeapData->fillValues[type];
   OSUninterruptibleSpinLock_Release(virt_addrof(sMemHeapData->lock));
   return value;
}

void
MEMSetFillValForHeap(MEMHeapFillType type,
                     uint32_t value)
{
   OSUninterruptibleSpinLock_Acquire(virt_addrof(sMemHeapData->lock));
   sMemHeapData->fillValues[type] = value;
   OSUninterruptibleSpinLock_Release(virt_addrof(sMemHeapData->lock));
}

namespace internal
{

void
registerHeap(virt_ptr<MEMHeapHeader> heap,
             MEMHeapTag tag,
             virt_ptr<uint8_t> dataStart,
             virt_ptr<uint8_t> dataEnd,
             MEMHeapFlags flags)
{
   // Setup heap header
   heap->tag = tag;
   heap->dataStart = dataStart;
   heap->dataEnd = dataEnd;
   heap->flags = flags;

   if (heap->flags & MEMHeapFlags::DebugMode) {
      auto fillVal = MEMGetFillValForHeap(MEMHeapFillType::Unused);
      std::memset(dataStart.get(), fillVal, dataEnd - dataStart);
   }

   MEMInitList(virt_addrof(heap->list), offsetof(MEMHeapHeader, link));

   if (!sMemHeapData->initialisedLock) {
      OSInitSpinLock(virt_addrof(sMemHeapData->lock));
      sMemHeapData->initialisedLock = TRUE;
   }

   if (!sMemHeapData->initialisedLists) {
      MEMInitList(virt_addrof(sMemHeapData->foregroundList), offsetof(MEMHeapHeader, link));
      MEMInitList(virt_addrof(sMemHeapData->mem1List), offsetof(MEMHeapHeader, link));
      MEMInitList(virt_addrof(sMemHeapData->mem2List), offsetof(MEMHeapHeader, link));
      sMemHeapData->initialisedLists = TRUE;
   }

   OSInitSpinLock(virt_addrof(heap->lock));

   // Add to heap list
   OSUninterruptibleSpinLock_Acquire(virt_addrof(sMemHeapData->lock));

   if (auto list = findListContainingHeap(heap)) {
      MEMAppendListObject(list, heap);
   }

   OSUninterruptibleSpinLock_Release(virt_addrof(sMemHeapData->lock));
}

void
unregisterHeap(virt_ptr<MEMHeapHeader> heap)
{
   OSUninterruptibleSpinLock_Acquire(virt_addrof(sMemHeapData->lock));

   if (auto list = findListContainingHeap(heap)) {
      MEMRemoveListObject(list, heap);
   }

   OSUninterruptibleSpinLock_Release(virt_addrof(sMemHeapData->lock));
}

void
initialiseMemHeap()
{
   OSInitSpinLock(virt_addrof(sMemHeapData->lock));
   sMemHeapData->arenas.fill(nullptr);
   sMemHeapData->fillValues[0] = 0xC3C3C3C3u;
   sMemHeapData->fillValues[1] = 0xF3F3F3F3u;
   sMemHeapData->fillValues[2] = 0xD3D3D3D3u;
}

} // namespace internal

void
Library::registerMemHeapSymbols()
{
   RegisterFunctionExport(MEMGetBaseHeapHandle);
   RegisterFunctionExport(MEMSetBaseHeapHandle);
   RegisterFunctionExport(MEMCreateUserHeapHandle);
   RegisterFunctionExport(MEMGetArena);
   RegisterFunctionExport(MEMFindContainHeap);
   RegisterFunctionExport(MEMDumpHeap);
   RegisterFunctionExport(MEMGetFillValForHeap);
   RegisterFunctionExport(MEMSetFillValForHeap);

   RegisterDataInternal(sMemHeapData);
}

} // namespace cafe::coreinit
