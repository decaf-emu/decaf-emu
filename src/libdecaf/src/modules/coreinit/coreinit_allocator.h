#pragma once
#include "ppcutils/wfunc_ptr.h"

#include <common/be_val.h>
#include <common/structsize.h>

namespace coreinit
{

struct MEMAllocator;
struct MEMBlockHeap;
struct ExpandedHeap;
struct MEMFrameHeap;
struct MEMUnitHeap;

/**
 * \defgroup coreinit_allocator Allocator
 * \ingroup coreinit
 * @{
 */

using MEMAllocatorAllocFn = wfunc_ptr<void *, MEMAllocator *, uint32_t>;
using MEMAllocatorFreeFn = wfunc_ptr<void, MEMAllocator *, void *>;

struct MEMAllocatorFunctions
{
   MEMAllocatorAllocFn::be alloc;
   MEMAllocatorFreeFn::be free;
};
CHECK_OFFSET(MEMAllocatorFunctions, 0x0, alloc);
CHECK_OFFSET(MEMAllocatorFunctions, 0x4, free);
CHECK_SIZE(MEMAllocatorFunctions, 0x8);

struct MEMAllocator
{
   be_ptr<MEMAllocatorFunctions> funcs;
   be_ptr<void> heap;
   be_val<uint32_t> align;
   UNKNOWN(4);
};
CHECK_OFFSET(MEMAllocator, 0x0, funcs);
CHECK_OFFSET(MEMAllocator, 0x4, heap);
CHECK_OFFSET(MEMAllocator, 0x8, align);
CHECK_SIZE(MEMAllocator, 0x10);

void
MEMInitAllocatorForDefaultHeap(MEMAllocator *allocator);

void
MEMInitAllocatorForBlockHeap(MEMAllocator *allocator,
                             MEMBlockHeap *heap,
                             int alignment);

void
MEMInitAllocatorForExpHeap(MEMAllocator *allocator,
                           ExpandedHeap *heap,
                           int alignment);

void
MEMInitAllocatorForFrmHeap(MEMAllocator *allocator,
                           MEMFrameHeap *heap,
                           int alignment);

void
MEMInitAllocatorForUnitHeap(MEMAllocator *allocator,
                            MEMUnitHeap *heap);

void *
MEMAllocFromAllocator(MEMAllocator *allocator,
                      uint32_t size);

void
MEMFreeToAllocator(MEMAllocator *allocator,
                   void *block);

/** @} */

} // namespace coreinit
