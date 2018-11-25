#include <hle_test.h>
#include <coreinit/memheap.h>
#include <coreinit/memblockheap.h>
#include <coreinit/memexpheap.h>

static const uint32_t
TrackSize = 1024;

static const uint32_t
HeapSize = 1024 * 1024;

int main(int argc, char **argv)
{
   MEMBlockHeap blockHeapStorage;
   MEMHeapHandle mem2 = MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM2);
   test_assert(mem2);

   test_report("Allocating %d bytes from default heap", HeapSize);
   void *heapStart = MEMAllocFromExpHeapEx(mem2, HeapSize, 4);
   void *heapEnd = (uint8_t*)heapStart + HeapSize;
   test_assert(heapStart);

   test_report("Allocating %d bytes from default heap", TrackSize);
   void *trackAddr = MEMAllocFromExpHeapEx(mem2, TrackSize, 4);
   test_assert(trackAddr);

   test_report("Creating block heap at %p", heapStart);
   MEMHeapHandle blockHeap = MEMInitBlockHeap(&blockHeapStorage, heapStart, heapEnd, trackAddr, TrackSize, 0);
   test_assert(blockHeap);

   uint32_t freeSize1 = MEMGetTotalFreeSizeForBlockHeap(blockHeap);
   test_report("Free Size before allocation: %d", freeSize1);

   // Allocate some memory from head
   void *block1 = MEMAllocFromBlockHeapEx(blockHeap, 1024, 4);
   test_assert(block1);

   uint32_t freeSize2 = MEMGetTotalFreeSizeForBlockHeap(blockHeap);
   test_report("Free Size after head allocation: %d", freeSize2);
   test_assert(freeSize2 < freeSize1);

   // Allocate some memory from tail
   void *block2 = MEMAllocFromBlockHeapEx(blockHeap, 1024, -4);
   test_assert(block2);

   uint32_t freeSize3 = MEMGetTotalFreeSizeForBlockHeap(blockHeap);
   test_report("Free Size after tail allocation: %d", freeSize3);
   test_assert(freeSize3 < freeSize2);

   // Free all memory
   MEMFreeToBlockHeap(blockHeap, block1);
   MEMFreeToBlockHeap(blockHeap, block2);

   uint32_t freeSize4 = MEMGetTotalFreeSizeForBlockHeap(blockHeap);
   test_report("Free Size after free: %d", freeSize4);
   test_assert(freeSize1 == freeSize4);

   MEMDestroyBlockHeap(blockHeap);
   MEMFreeToExpHeap(mem2, heapStart);
   return 0;
}
