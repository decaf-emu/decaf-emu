#include "coreinit.h"
#include "coreinit_allocator.h"
#include "coreinit_expheap.h"
#include "coreinit_frameheap.h"
#include "coreinit_memheap.h"
#include "coreinit_unitheap.h"
#include "ppcutils/wfunc_call.h"
#include "common/decaf_assert.h"

namespace coreinit
{

static AllocatorFunctions *
sDefaultHeapFunctions = nullptr;

static AllocatorFunctions *
sBlockHeapFunctions = nullptr;

static AllocatorFunctions *
sExpHeapFunctions = nullptr;

static AllocatorFunctions *
sFrameHeapFunctions = nullptr;

static AllocatorFunctions *
sUnitHeapFunctions = nullptr;

static AllocatorAllocFn sAllocatorDefaultHeapAlloc;
static AllocatorFreeFn sAllocatorDefaultHeapFree;
static AllocatorAllocFn sAllocatorBlockHeapAlloc;
static AllocatorFreeFn sAllocatorBlockHeapFree;
static AllocatorAllocFn sAllocatorExpHeapAlloc;
static AllocatorFreeFn sAllocatorExpHeapFree;
static AllocatorAllocFn sAllocatorFrmHeapAlloc;
static AllocatorFreeFn sAllocatorFrmHeapFree;
static AllocatorAllocFn sAllocatorUnitHeapAlloc;
static AllocatorFreeFn sAllocatorUnitHeapFree;


/**
 * Initialise an Allocator struct for the default heap.
 */
void
MEMInitAllocatorForDefaultHeap(Allocator *allocator)
{
   decaf_check(sDefaultHeapFunctions);
   allocator->heap = MEMGetBaseHeapHandle(MEMBaseHeapType::MEM2);
   allocator->align = 0;
   allocator->funcs = sDefaultHeapFunctions;
}


/**
 * Initialise an Allocator struct for a block heap.
 */
void
MEMInitAllocatorForBlockHeap(Allocator *allocator,
                             BlockHeap *heap,
                             int alignment)
{
   decaf_check(sBlockHeapFunctions);
   allocator->heap = heap;
   allocator->align = alignment;
   allocator->funcs = sBlockHeapFunctions;
}


/**
 * Initialise an Allocator struct for an expanded heap.
 */
void
MEMInitAllocatorForExpHeap(Allocator *allocator,
                           ExpandedHeap *heap,
                           int alignment)
{
   decaf_check(sExpHeapFunctions);
   allocator->heap = heap;
   allocator->align = alignment;
   allocator->funcs = sExpHeapFunctions;
}


/**
 * Initialise an Allocator struct for a frame heap.
 */
void
MEMInitAllocatorForFrmHeap(Allocator *allocator,
                           FrameHeap *heap,
                           int alignment)
{
   decaf_check(sFrameHeapFunctions);
   allocator->heap = heap;
   allocator->align = alignment;
   allocator->funcs = sFrameHeapFunctions;
}


/**
 * Initialise an Allocator struct for a unit heap.
 */
void
MEMInitAllocatorForUnitHeap(Allocator *allocator,
                            UnitHeap *heap)
{
   decaf_check(sUnitHeapFunctions);
   allocator->heap = heap;
   allocator->align = 0;
   allocator->funcs = sUnitHeapFunctions;
}


/**
 * Allocate memory from an Allocator.
 *
 * \return Returns pointer to new allocated memory.
 */
void *
MEMAllocFromAllocator(Allocator *allocator,
                      uint32_t size)
{
   return allocator->funcs->alloc(allocator, size);
}


/**
 * Free memory from an Allocator.
 */
void
MEMFreeToAllocator(Allocator *allocator,
                   void *block)
{
   allocator->funcs->free(allocator, block);
}


static void *
allocatorDefaultHeapAlloc(Allocator *allocator,
                          uint32_t size)
{
   return (*pMEMAllocFromDefaultHeap)(size);
}

static void
allocatorDefaultHeapFree(Allocator *allocator,
                         void *block)
{
   (*pMEMFreeToDefaultHeap)(block);
}

static void *
allocatorBlockHeapAlloc(Allocator *allocator,
                        uint32_t size)
{
   // TODO: Call MEMAllocFromBlockHeap
   return nullptr;
}

static void
allocatorBlockHeapFree(Allocator *allocator,
                       void *block)
{
   // TODO: Call MEMFreeToBlockHeap
}

static void *
allocatorExpHeapAlloc(Allocator *allocator,
                      uint32_t size)
{
   return MEMAllocFromExpHeapEx(reinterpret_cast<ExpandedHeap *>(allocator->heap.get()),
                                size,
                                allocator->align);
}

static void
allocatorExpHeapFree(Allocator *allocator,
                     void *block)
{
   MEMFreeToExpHeap(reinterpret_cast<ExpandedHeap *>(allocator->heap.get()),
                    block);
}

static void *
allocatorFrmHeapAlloc(Allocator *allocator,
                      uint32_t size)
{
   return MEMAllocFromFrmHeapEx(reinterpret_cast<FrameHeap *>(allocator->heap.get()),
                                size,
                                allocator->align);
}

static void
allocatorFrmHeapFree(Allocator *allocator,
                     void *block)
{
   /* Woooowwww I sure hope no one uses frame heap in an allocator...
    *
    * coreinit.rpl does not actually free memory here, probably because
    * using a frame heap for an allocator where you do not know the exact
    * order of alloc and free is a really dumb idea
    */
   gLog->warn("Allocator did not free memory allocated from frame heap");
}

static void *
allocatorUnitHeapAlloc(Allocator *allocator,
                       uint32_t size)
{
   return MEMAllocFromUnitHeap(reinterpret_cast<MEMUnitHeap *>(allocator->heap.get()));
}

static void
allocatorUnitHeapFree(Allocator *allocator,
                      void *block)
{
   MEMFreeToUnitHeap(reinterpret_cast<MEMUnitHeap *>(allocator->heap.get()),
                     block);
}

void
Module::initialiseAllocatorFunctions()
{
   sDefaultHeapFunctions->alloc = sAllocatorDefaultHeapAlloc;
   sDefaultHeapFunctions->free = sAllocatorDefaultHeapFree;

   sBlockHeapFunctions->alloc = sAllocatorBlockHeapAlloc;
   sBlockHeapFunctions->free = sAllocatorBlockHeapFree;

   sExpHeapFunctions->alloc = sAllocatorExpHeapAlloc;
   sExpHeapFunctions->free = sAllocatorExpHeapFree;

   sFrameHeapFunctions->alloc = sAllocatorFrmHeapAlloc;
   sFrameHeapFunctions->free = sAllocatorFrmHeapFree;

   sUnitHeapFunctions->alloc = sAllocatorUnitHeapAlloc;
   sUnitHeapFunctions->free = sAllocatorUnitHeapFree;
}

void
Module::registerAllocatorFunctions()
{
   RegisterKernelFunction(MEMInitAllocatorForDefaultHeap);
   RegisterKernelFunction(MEMInitAllocatorForBlockHeap);
   RegisterKernelFunction(MEMInitAllocatorForExpHeap);
   RegisterKernelFunction(MEMInitAllocatorForFrmHeap);
   RegisterKernelFunction(MEMInitAllocatorForUnitHeap);
   RegisterKernelFunction(MEMAllocFromAllocator);
   RegisterKernelFunction(MEMFreeToAllocator);

   RegisterInternalFunction(allocatorDefaultHeapAlloc, sAllocatorDefaultHeapAlloc);
   RegisterInternalFunction(allocatorDefaultHeapFree, sAllocatorDefaultHeapFree);
   RegisterInternalFunction(allocatorBlockHeapAlloc, sAllocatorBlockHeapAlloc);
   RegisterInternalFunction(allocatorBlockHeapFree, sAllocatorBlockHeapFree);
   RegisterInternalFunction(allocatorExpHeapAlloc, sAllocatorExpHeapAlloc);
   RegisterInternalFunction(allocatorExpHeapFree, sAllocatorExpHeapFree);
   RegisterInternalFunction(allocatorFrmHeapAlloc, sAllocatorFrmHeapAlloc);
   RegisterInternalFunction(allocatorFrmHeapFree, sAllocatorFrmHeapFree);
   RegisterInternalFunction(allocatorUnitHeapAlloc, sAllocatorUnitHeapAlloc);
   RegisterInternalFunction(allocatorUnitHeapFree, sAllocatorUnitHeapFree);

   RegisterInternalData(sDefaultHeapFunctions);
   RegisterInternalData(sBlockHeapFunctions);
   RegisterInternalData(sExpHeapFunctions);
   RegisterInternalData(sFrameHeapFunctions);
   RegisterInternalData(sUnitHeapFunctions);
}

} // namespace coreinit
