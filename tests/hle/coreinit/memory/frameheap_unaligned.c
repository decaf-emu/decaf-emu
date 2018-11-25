#include <hle_test.h>
#include <coreinit/memheap.h>
#include <coreinit/memexpheap.h>
#include <coreinit/memfrmheap.h>

static const uint32_t
HeapSize = (1024 * 1024) + 1;

int main(int argc, char **argv)
{
   MEMHeapHandle mem2 = MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM2);
   test_assert(mem2);

   test_report("Allocating %d bytes from default heap", HeapSize);
   void *heapAddr = MEMAllocFromExpHeapEx(mem2, HeapSize, 4);
   test_assert(heapAddr);

   // Unalign the frame heap base address
   void *frameHeapAddr = heapAddr + 1;
   test_report("Creating frame heap at %p", frameHeapAddr);
   MEMHeapHandle frameHeap = MEMCreateFrmHeapEx(frameHeapAddr, HeapSize, 0);
   test_report("Frame heap created at %p", frameHeap);
   test_assert(frameHeap);
   test_assert((((uint32_t)frameHeap) % 4) == 0);

   uint32_t freeSize1 = MEMGetAllocatableSizeForFrmHeapEx(frameHeap, 4);
   test_report("Free Size before allocation: %d", freeSize1);
   test_assert((freeSize1 % 4) == 0);

   {
      // Allocate an unaligned block from head with an unaligned size
      void *unalignedBlock = MEMAllocFromFrmHeapEx(frameHeap, 5, 1);
      uint32_t unalignedBlockAddress = (uint32_t)unalignedBlock;
      test_report("Unaligned head block allocated at %p", unalignedBlock);
      test_assert(unalignedBlock);

      // Allocate an aligned block from head and ensure it is aligned
      void *alignedBlock = MEMAllocFromFrmHeapEx(frameHeap, 1024, 512);
      uint32_t alignedBlockAddress = (uint32_t)alignedBlock;
      test_report("Aligned head block allocated at %p", alignedBlock);
      test_assert(alignedBlock);
      test_assert((((uint32_t)alignedBlock) % 512) == 0);
      test_assert(alignedBlockAddress - unalignedBlockAddress >= 5);
   }

   {
      // Allocate an unaligned block from tail with an unaligned size
      void *unalignedBlock = MEMAllocFromFrmHeapEx(frameHeap, 5, -1);
      uint32_t unalignedBlockAddress = (uint32_t)unalignedBlock;
      test_report("Unaligned tail block allocated at %p", unalignedBlock);
      test_assert(unalignedBlock);

      // Allocate an aligned block from tail and ensure it is aligned
      void *alignedBlock = MEMAllocFromFrmHeapEx(frameHeap, 1024, -512);
      uint32_t alignedBlockAddress = (uint32_t)alignedBlock;
      test_report("Aligned tail block allocated at %p", alignedBlock);
      test_assert(alignedBlock);
      test_assert((((uint32_t)alignedBlock) % 512) == 0);
      test_assert(unalignedBlockAddress - alignedBlockAddress >= 512);
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
