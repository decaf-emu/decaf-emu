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

struct TinyHeap
{
   //! Pointer to the start of the data heap
   be2_virt_ptr<void> dataHeapStart;

   //! Pointer to the end of the data heap
   be2_virt_ptr<void> dataHeapEnd;

   //! Index of first tracking block
   be2_val<int32_t> firstBlockIdx;

   //! Index of last tracking block
   be2_val<int32_t> lastBlockIdx;

   //! Index of first unused tracking block
   be2_val<int32_t> nextFreeBlockIdx;
};
CHECK_OFFSET(TinyHeap, 0x00, dataHeapStart);
CHECK_OFFSET(TinyHeap, 0x04, dataHeapEnd);
CHECK_OFFSET(TinyHeap, 0x08, firstBlockIdx);
CHECK_OFFSET(TinyHeap, 0x0C, lastBlockIdx);
CHECK_OFFSET(TinyHeap, 0x10, nextFreeBlockIdx);
CHECK_SIZE(TinyHeap, 0x14);

TinyHeapError
TinyHeap_Setup(virt_ptr<TinyHeap> heap,
               int32_t trackingHeapSize,
               virt_ptr<void> dataHeap,
               int32_t dataHeapSize);

TinyHeapError
TinyHeap_Alloc(virt_ptr<TinyHeap> heap,
               int32_t size,
               int32_t align,
               virt_ptr<void> *outPtr);

TinyHeapError
TinyHeap_Alloc(virt_ptr<TinyHeap> heap,
               int32_t size,
               int32_t align,
               virt_ptr<virt_ptr<void>> outPtr);

TinyHeapError
TinyHeap_AllocAt(virt_ptr<TinyHeap> heap,
                 virt_ptr<void> ptr,
                 int32_t size);

void
TinyHeap_Free(virt_ptr<TinyHeap> heap,
              virt_ptr<void> ptr);

int32_t
TinyHeap_GetLargestFree(virt_ptr<TinyHeap> heap);

} // namespace cafe
