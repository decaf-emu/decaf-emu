#pragma once
#include "coreinit_memheap.h"
#include <libcpu/be2_struct.h>

namespace cafe::coreinit
{

struct MEMAllocator;
struct MEMBlockHeap;
struct MEMExpandedHeap;
struct MEMFrameHeap;
struct MEMUnitHeap;

/**
 * \defgroup coreinit_allocator Allocator
 * \ingroup coreinit
 * @{
 */

using MEMAllocatorAllocFn = virt_func_ptr<virt_ptr<void>(virt_ptr<MEMAllocator>, uint32_t)>;
using MEMAllocatorFreeFn = virt_func_ptr<void(virt_ptr<MEMAllocator>, virt_ptr<void>)>;

struct MEMAllocatorFunctions
{
   be2_val<MEMAllocatorAllocFn> alloc;
   be2_val<MEMAllocatorFreeFn> free;
};
CHECK_OFFSET(MEMAllocatorFunctions, 0x0, alloc);
CHECK_OFFSET(MEMAllocatorFunctions, 0x4, free);
CHECK_SIZE(MEMAllocatorFunctions, 0x8);

struct MEMAllocator
{
   be2_virt_ptr<MEMAllocatorFunctions> funcs;
   be2_val<MEMHeapHandle> heap;
   be2_val<int32_t> align;
   UNKNOWN(4);
};
CHECK_OFFSET(MEMAllocator, 0x0, funcs);
CHECK_OFFSET(MEMAllocator, 0x4, heap);
CHECK_OFFSET(MEMAllocator, 0x8, align);
CHECK_SIZE(MEMAllocator, 0x10);

void
MEMInitAllocatorForDefaultHeap(virt_ptr<MEMAllocator> allocator);

void
MEMInitAllocatorForBlockHeap(virt_ptr<MEMAllocator> allocator,
                             MEMHeapHandle handle,
                             int32_t alignment);

void
MEMInitAllocatorForExpHeap(virt_ptr<MEMAllocator> allocator,
                           MEMHeapHandle handle,
                           int32_t alignment);

void
MEMInitAllocatorForFrmHeap(virt_ptr<MEMAllocator> allocator,
                           MEMHeapHandle handle,
                           int32_t alignment);

void
MEMInitAllocatorForUnitHeap(virt_ptr<MEMAllocator> allocator,
                            MEMHeapHandle handle);

virt_ptr<void>
MEMAllocFromAllocator(virt_ptr<MEMAllocator> allocator,
                      uint32_t size);

void
MEMFreeToAllocator(virt_ptr<MEMAllocator> allocator,
                   virt_ptr<void> block);

/** @} */

} // namespace cafe::coreinit
