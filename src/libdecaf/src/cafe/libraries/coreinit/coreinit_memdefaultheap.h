#pragma once
#include "coreinit_memheap.h"
#include <libcpu/be2_struct.h>

namespace cafe::coreinit
{

void
CoreInitDefaultHeap(virt_ptr<MEMHeapHandle> outHeapHandleMEM1,
                    virt_ptr<MEMHeapHandle> outHeapHandleFG,
                    virt_ptr<MEMHeapHandle> outHeapHandleMEM2);

virt_ptr<void>
MEMAllocFromDefaultHeap(uint32_t size);

virt_ptr<void>
MEMAllocFromDefaultHeapEx(uint32_t size,
                          int32_t align);

void
MEMFreeToDefaultHeap(virt_ptr<void> ptr);

} // namespace cafe::coreinit
