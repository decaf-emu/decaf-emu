#include "ios_kernel_memory.h"
#include <common/teenyheap.h>

namespace ios::kernel
{

static TeenyHeap *
sMem0Heap = nullptr;

static TeenyHeap *
sSram0Heap = nullptr;

static TeenyHeap *
sSram1Heap = nullptr;

phys_ptr<void>
allocMEM0(size_t size)
{
   return nullptr;
}

void
freeMEM0(phys_ptr<void> ptr)
{
}

phys_ptr<void>
allocSRAM0(size_t size)
{
   return nullptr;
}

void
freeSRAM0(phys_ptr<void> ptr)
{
}

phys_ptr<void>
allocSRAM1(size_t size)
{
   return nullptr;
}

void
freeSRAM1(phys_ptr<void> ptr)
{
}

void
kernelInitialiseMemory()
{
   sMem0Heap = new TeenyHeap { phys_ptr<void> { phys_addr { 0x08120000 } }.getRawPointer(), 0xA0000 };
}

} // namespace ios::kernel
