#include <hle_test.h>
#include <coreinit/memheap.h>
#include <coreinit/memexpheap.h>
#include <coreinit/memfrmheap.h>

static const uint32_t
HeapSize = 1024 * 1024;

static const uint32_t
AllocationSize = 1024;

static const uint32_t
HeadAllocations = 5;

static const uint32_t
TailAllocations = 3;

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

   uint32_t freeSizeInitial = MEMGetAllocatableSizeForFrmHeapEx(frameHeap, 4);
   uint32_t freeSize = freeSizeInitial;
   test_report("Free Size before allocation: %d", freeSize);

   void *lastHeadBlock = NULL;
   void *lastTailBlock = NULL;

   // Allocate some memory from head
   for (int i = 0; i < HeadAllocations; ++i) {
      // Allocate a block from head
      void *block = MEMAllocFromFrmHeapEx(frameHeap, AllocationSize, 4);
      test_assert(block);

      // Ensure allocation address is correct
      if (lastHeadBlock) {
         test_assert(block > lastHeadBlock);
         test_assert(((uint32_t)block - (uint32_t)lastHeadBlock) == AllocationSize);
      }

      // Ensure size is going down by exact amount
      uint32_t freeSizeAfterHeadAllocation = MEMGetAllocatableSizeForFrmHeapEx(frameHeap, 4);
      freeSize -= AllocationSize;
      test_assert(freeSizeAfterHeadAllocation == freeSize);
   }

   test_report("Free Size after head allocations: %d", freeSize);

   // Allocate some memory from tail
   for (int i = 0; i < TailAllocations; ++i) {
      // Allocate a block from tail
      void *block = MEMAllocFromFrmHeapEx(frameHeap, AllocationSize, -4);
      test_assert(block);

      // Ensure allocation address is correct
      if (lastTailBlock) {
         test_assert(block < lastTailBlock);
         test_assert(((uint32_t)lastTailBlock - (uint32_t)block) == AllocationSize);
      }

      // Ensure size is going down by exact amount
      uint32_t freeSizeAfterTailAllocation = MEMGetAllocatableSizeForFrmHeapEx(frameHeap, 4);
      freeSize -= AllocationSize;
      test_assert(freeSizeAfterTailAllocation == freeSize);
   }

   test_report("Free Size after tail allocations: %d", freeSize);

   // Free head memory
   MEMFreeToFrmHeap(frameHeap, MEM_FRM_HEAP_FREE_HEAD);
   freeSize += AllocationSize * HeadAllocations;

   uint32_t freeSizeAfterHeadFree = MEMGetAllocatableSizeForFrmHeapEx(frameHeap, 4);
   test_report("Free Size after head free: %d", freeSizeAfterHeadFree);
   test_assert(freeSizeAfterHeadFree == freeSize);

   // Free tail memory
   MEMFreeToFrmHeap(frameHeap, MEM_FRM_HEAP_FREE_TAIL);
   freeSize += AllocationSize * TailAllocations;

   uint32_t freeSizeAfterTailFree = MEMGetAllocatableSizeForFrmHeapEx(frameHeap, 4);
   test_report("Free Size after tail free: %d", freeSizeAfterTailFree);
   test_assert(freeSizeAfterTailFree == freeSize);

   // Ensure we are back to the original amount of memory
   test_assert(freeSizeInitial == freeSize);

   MEMDestroyFrmHeap(frameHeap);
   MEMFreeToExpHeap(mem2, heapAddr);
   return 0;
}
