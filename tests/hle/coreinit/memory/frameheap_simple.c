#include <hle_test.h>
#include <coreinit/memheap.h>
#include <coreinit/memexpheap.h>
#include <coreinit/memfrmheap.h>

static const uint32_t
HeapSize = 1024 * 1024;

int main(int argc, char **argv)
{
   MEMHeapHandle mem2 = MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM2);
   test_assert(mem2);

   test_report("Allocating %d bytes from default heap", HeapSize);
   void *heapAddr = MEMAllocFromExpHeapEx(mem2, HeapSize, 4);
   test_assert(heapAddr);

   test_report("Creating frame heap at %p", heapAddr);
   MEMHeapHandle frameHeap = MEMCreateFrmHeapEx(heapAddr, HeapSize, 0);
   test_assert(frameHeap);

   uint32_t freeSize1 = MEMGetAllocatableSizeForFrmHeapEx(frameHeap, 4);
   test_report("Free Size before allocation: %d", freeSize1);

   {
      // Allocate some memory from head
      void *block = MEMAllocFromFrmHeapEx(frameHeap, 1024, 4);
      test_assert(block);

      uint32_t freeSize2 = MEMGetAllocatableSizeForFrmHeapEx(frameHeap, 4);
      test_report("Free Size after head allocation: %d", freeSize2);
      test_assert(freeSize2 < freeSize1);
   }

   {
      // Allocate some memory from tail
      void *block = MEMAllocFromFrmHeapEx(frameHeap, 1024, -4);
      test_assert(block);

      uint32_t freeSize2 = MEMGetAllocatableSizeForFrmHeapEx(frameHeap, 4);
      test_report("Free Size after tail allocation: %d", freeSize2);
      test_assert(freeSize2 < freeSize1);
   }

   // Free all memory
   MEMFreeToFrmHeap(frameHeap, MEM_FRM_HEAP_FREE_ALL);

   uint32_t freeSize3 = MEMGetAllocatableSizeForFrmHeapEx(frameHeap, 4);
   test_report("Free Size after free: %d", freeSize3);
   test_assert(freeSize1 == freeSize3);

   MEMDestroyFrmHeap(frameHeap);
   MEMFreeToExpHeap(mem2, heapAddr);
   return 0;
}
