#include "common/decaf_assert.h"
#include "kernel_memory.h"
#include "libcpu/mem.h"

namespace kernel
{

uint32_t
sCodeHeapSize = 0;

TeenyHeap *
sCodeHeap = nullptr;

void
initialiseCodeHeap(uint32_t size)
{
   sCodeHeap = new TeenyHeap(mem::translate(mem::MEM2Base), size);
   sCodeHeapSize = size;
}

TeenyHeap *
getCodeHeap()
{
   decaf_check(sCodeHeap);
   return sCodeHeap;
}

void
getMEM1Bound(uint32_t *addr, uint32_t *size)
{
   *addr = mem::MEM1Base;
   *size = mem::MEM1Size;
}

void
getMEM2Bound(uint32_t *addr, uint32_t *size)
{
   decaf_check(sCodeHeapSize > 0);

   *addr = mem::MEM2Base + sCodeHeapSize;
   *size = mem::MEM2Size - sCodeHeapSize;
}

} // namespace kernel