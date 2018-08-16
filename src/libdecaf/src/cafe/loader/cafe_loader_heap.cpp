#pragma optimize("", off)
#include "cafe_loader_error.h"
#include "cafe_loader_flush.h"
#include "cafe_loader_heap.h"
#include <common/log.h>

namespace cafe::loader::internal
{

int32_t
LiCacheLineCorrectAllocEx(virt_ptr<TinyHeap> heap,
                          uint32_t textSize,
                          int32_t textAlign,
                          virt_ptr<void> *outPtr,
                          uint32_t /*unused*/,
                          uint32_t *outAllocSize,
                          uint32_t *outLargestFree,
                          ios::mcp::MCPFileType fileType)
{
   auto fromEnd = false;
   textSize = align_up(textSize, 128);
   *outAllocSize = textSize;

   if (textAlign < 0) {
      textAlign = -textAlign;
      fromEnd = true;
   }

   if (textAlign == 0 && textAlign < 64) {
      textAlign = 64;
   }

   if (fromEnd) {
      textAlign = -textAlign;
   }

   auto tinyHeapError = TinyHeap_Alloc(heap, textSize, textAlign, outPtr);
   if (tinyHeapError < TinyHeapError::OK) {
      LiSetFatalError(0x187298, fileType, 0, "LiCacheLineCorrectAllocEx", 0x88);
      *outLargestFree = TinyHeap_GetLargestFree(heap);
      return static_cast<int32_t>(tinyHeapError);
   }

   std::memset(outPtr->getRawPointer(), 0, textSize);
   return 0;
}

void
LiCacheLineCorrectFreeEx(virt_ptr<TinyHeap> heap,
                         virt_ptr<void> ptr,
                         uint32_t size)
{
   TinyHeap_Free(heap, ptr);
   LiSafeFlushCode(virt_cast<virt_addr>(ptr), size);
}

} // namespace cafe::loader::internal
