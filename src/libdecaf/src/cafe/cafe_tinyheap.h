#pragma once
#include <libcpu/be2_struct.h>

namespace cafe
{

constexpr auto TinyHeapHeaderSize = 0x30;
constexpr auto TinyHeapBlockSize = 16;

enum class TinyHeapError
{
   OK = 0,
   SetupFailed = -520001,
   InvalidHeap = -520002,
   AllocAtFailed = -520003,
   AllocFailed = -520004,
};

template<typename AddressType>
struct TinyHeapBase
{
   template<typename ValueType>
   using be2_pointer_type = be2_val<cpu::Pointer<ValueType, AddressType>>;

   //! Pointer to the start of the data heap
   be2_pointer_type<void> dataHeapStart;

   //! Pointer to the end of the data heap
   be2_pointer_type<void> dataHeapEnd;

   //! Index of first tracking block
   be2_val<int32_t> firstBlockIdx;

   //! Index of last tracking block
   be2_val<int32_t> lastBlockIdx;

   //! Index of first unused tracking block
   be2_val<int32_t> nextFreeBlockIdx;
};
CHECK_OFFSET(TinyHeapBase<virt_addr>, 0x00, dataHeapStart);
CHECK_OFFSET(TinyHeapBase<virt_addr>, 0x04, dataHeapEnd);
CHECK_OFFSET(TinyHeapBase<virt_addr>, 0x08, firstBlockIdx);
CHECK_OFFSET(TinyHeapBase<virt_addr>, 0x0C, lastBlockIdx);
CHECK_OFFSET(TinyHeapBase<virt_addr>, 0x10, nextFreeBlockIdx);
CHECK_SIZE(TinyHeapBase<virt_addr>, 0x14);

using TinyHeapVirtual = TinyHeapBase<virt_addr>;
using TinyHeapPhysical = TinyHeapBase<phys_addr>;
using TinyHeap = TinyHeapVirtual;

// TinyHeap virtual
TinyHeapError
TinyHeap_Setup(virt_ptr<TinyHeapVirtual> heap,
               int32_t trackingHeapSize,
               virt_ptr<void> dataHeap,
               int32_t dataHeapSize);

TinyHeapError
TinyHeap_Alloc(virt_ptr<TinyHeapVirtual> heap,
               int32_t size,
               int32_t align,
               virt_ptr<void> *outPtr);

TinyHeapError
TinyHeap_AllocAt(virt_ptr<TinyHeapVirtual> heap,
                 virt_ptr<void> ptr,
                 int32_t size);

void
TinyHeap_Free(virt_ptr<TinyHeapVirtual> heap,
              virt_ptr<void> ptr);

int32_t
TinyHeap_GetLargestFree(virt_ptr<TinyHeapVirtual> heap);

virt_ptr<void>
TinyHeap_Enum(virt_ptr<TinyHeapVirtual> heap,
              virt_ptr<void> prevBlockPtr,
              virt_ptr<void> *outPtr,
              uint32_t *outSize);

virt_ptr<void>
TinyHeap_EnumFree(virt_ptr<TinyHeapVirtual> heap,
                  virt_ptr<void> prevBlockPtr,
                  virt_ptr<void> *outPtr,
                  uint32_t *outSize);

// TinyHeap physical
TinyHeapError
TinyHeap_Setup(phys_ptr<TinyHeapPhysical> heap,
               int32_t trackingHeapSize,
               phys_ptr<void> dataHeap,
               int32_t dataHeapSize);

TinyHeapError
TinyHeap_Alloc(phys_ptr<TinyHeapPhysical> heap,
               int32_t size,
               int32_t align,
               phys_ptr<void> *outPtr);

TinyHeapError
TinyHeap_AllocAt(phys_ptr<TinyHeapPhysical> heap,
                 phys_ptr<void> ptr,
                 int32_t size);

void
TinyHeap_Free(phys_ptr<TinyHeapPhysical> heap,
              phys_ptr<void> ptr);

int32_t
TinyHeap_GetLargestFree(phys_ptr<TinyHeapPhysical> heap);

phys_ptr<void>
TinyHeap_Enum(phys_ptr<TinyHeapPhysical> heap,
              phys_ptr<void> prevBlockPtr,
              phys_ptr<void> *outPtr,
              uint32_t *outSize);

phys_ptr<void>
TinyHeap_EnumFree(phys_ptr<TinyHeapPhysical> heap,
                  phys_ptr<void> prevBlockPtr,
                  phys_ptr<void> *outPtr,
                  uint32_t *outSize);

} // namespace cafe
