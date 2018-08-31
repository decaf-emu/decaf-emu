#pragma once
#include "coreinit_enum.h"
#include "coreinit_memlist.h"
#include "coreinit_spinlock.h"

#include <common/bitfield.h>
#include <libcpu/be2_struct.h>

namespace cafe::coreinit
{

/**
 * \defgroup coreinit_memheap Memory Heap
 * \ingroup coreinit
 * @{
 */

#pragma pack(push, 1)

struct MEMHeapHeader
{
   //! Tag indicating which type of heap this is
   be2_val<MEMHeapTag> tag;

   //! Link for list this heap is in
   be2_struct<MEMListLink> link;

   //! List of all child heaps in this heap
   be2_struct<MEMList> list;

   //! Start address of allocatable memory
   be2_virt_ptr<uint8_t> dataStart;

   //! End address of allocatable memory
   be2_virt_ptr<uint8_t> dataEnd;

   //! Lock used when MEM_HEAP_FLAG_USE_LOCK is set.
   be2_struct<OSSpinLock> lock;

   //! Flags set during heap creation.
   be2_val<MEMHeapFlags> flags;

   UNKNOWN(0x0C);
};
CHECK_OFFSET(MEMHeapHeader, 0x00, tag);
CHECK_OFFSET(MEMHeapHeader, 0x04, link);
CHECK_OFFSET(MEMHeapHeader, 0x0C, list);
CHECK_OFFSET(MEMHeapHeader, 0x18, dataStart);
CHECK_OFFSET(MEMHeapHeader, 0x1C, dataEnd);
CHECK_OFFSET(MEMHeapHeader, 0x20, lock);
CHECK_OFFSET(MEMHeapHeader, 0x30, flags);
CHECK_SIZE(MEMHeapHeader, 0x40);

using MEMHeapHandle = virt_ptr<MEMHeapHeader>;

#pragma pack(pop)

void
MEMDumpHeap(virt_ptr<MEMHeapHeader> heap);

virt_ptr<MEMHeapHeader>
MEMFindContainHeap(virt_ptr<void> block);

MEMBaseHeapType
MEMGetArena(virt_ptr<MEMHeapHeader> heap);

MEMHeapHandle
MEMGetBaseHeapHandle(MEMBaseHeapType type);

MEMHeapHandle
MEMSetBaseHeapHandle(MEMBaseHeapType type,
                     MEMHeapHandle handle);

MEMHeapHandle
MEMCreateUserHeapHandle(virt_ptr<MEMHeapHeader> heap,
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
   HeapLock(virt_ptr<MEMHeapHeader> header)
   {
      if (header->flags & MEMHeapFlags::ThreadSafe) {
         OSUninterruptibleSpinLock_Acquire(virt_addrof(header->lock));
         mHeap = header;
      } else {
         mHeap = nullptr;
      }
   }

   ~HeapLock()
   {
      if (mHeap) {
         OSUninterruptibleSpinLock_Release(virt_addrof(mHeap->lock));
         mHeap = nullptr;
      }
   }

   void unlock()
   {
      if (mHeap) {
         OSUninterruptibleSpinLock_Release(virt_addrof(mHeap->lock));
         mHeap = nullptr;
      }
   }


private:
   virt_ptr<MEMHeapHeader> mHeap;
};

void
registerHeap(virt_ptr<MEMHeapHeader> heap,
             MEMHeapTag tag,
             virt_ptr<uint8_t> dataStart,
             virt_ptr<uint8_t> dataEnd,
             MEMHeapFlags flags);

void
unregisterHeap(virt_ptr<MEMHeapHeader> heap);

void
initialiseMemHeap();

} // namespace internal

} // namespace cafe::coreinit
