#pragma once
#include "types.h"
#include "common/be_val.h"
#include "common/structsize.h"
#include "ppcutils/wfunc_ptr.h"

namespace coreinit
{

struct Allocator;
struct BlockHeap;
struct ExpandedHeap;
struct FrameHeap;
struct UnitHeap;

/**
 * \defgroup coreinit_allocator Allocator
 * \ingroup coreinit
 * @{
 */

using AllocatorAllocFn = wfunc_ptr<void *, Allocator *, uint32_t>;
using be_AllocatorAllocFn = be_wfunc_ptr<void *, Allocator *, uint32_t>;

using AllocatorFreeFn = wfunc_ptr<void, Allocator *, void *>;
using be_AllocatorFreeFn = be_wfunc_ptr<void, Allocator *, void *>;

struct AllocatorFunctions
{
   be_AllocatorAllocFn alloc;
   be_AllocatorFreeFn free;
};
CHECK_OFFSET(AllocatorFunctions, 0x0, alloc);
CHECK_OFFSET(AllocatorFunctions, 0x4, free);
CHECK_SIZE(AllocatorFunctions, 0x8);

struct Allocator
{
   be_ptr<AllocatorFunctions> funcs;
   be_ptr<void> heap;
   be_val<uint32_t> align;
   UNKNOWN(4);
};
CHECK_OFFSET(Allocator, 0x0, funcs);
CHECK_OFFSET(Allocator, 0x4, heap);
CHECK_OFFSET(Allocator, 0x8, align);
CHECK_SIZE(Allocator, 0x10);

void
MEMInitAllocatorForDefaultHeap(Allocator *allocator);

void
MEMInitAllocatorForBlockHeap(Allocator *allocator,
                             BlockHeap *heap,
                             int alignment);

void
MEMInitAllocatorForExpHeap(Allocator *allocator,
                           ExpandedHeap *heap,
                           int alignment);

void
MEMInitAllocatorForFrmHeap(Allocator *allocator,
                           FrameHeap *heap,
                           int alignment);

void
MEMInitAllocatorForUnitHeap(Allocator *allocator,
                            UnitHeap *heap);

void *
MEMAllocFromAllocator(Allocator *allocator,
                      uint32_t size);

void
MEMFreeToAllocator(Allocator *allocator,
                   void *block);

/** @} */

} // namespace coreinit
