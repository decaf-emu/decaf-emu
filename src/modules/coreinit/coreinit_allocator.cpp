#include "coreinit.h"
#include "coreinit_allocator.h"
#include "coreinit_expheap.h"
#include "coreinit_frameheap.h"
#include "coreinit_memheap.h"
#include "coreinit_unitheap.h"
#include "ppcutils/wfunc_call.h"

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


/**
 * Initialise an Allocator struct for the default heap.
 */
void
MEMInitAllocatorForDefaultHeap(Allocator *allocator)
{
   assert(sDefaultHeapFunctions);
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
   assert(sBlockHeapFunctions);
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
   assert(sExpHeapFunctions);
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
   assert(sFrameHeapFunctions);
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
   assert(sUnitHeapFunctions);
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
   return MEMAllocFromUnitHeap(reinterpret_cast<UnitHeap *>(allocator->heap.get()));
}

static void
allocatorUnitHeapFree(Allocator *allocator,
                          void *block)
{
   MEMFreeToUnitHeap(reinterpret_cast<UnitHeap *>(allocator->heap.get()),
                     block);
}

void
Module::initialiseAllocatorFunctions()
{
   sDefaultHeapFunctions = internal::sysAlloc<AllocatorFunctions>();
   sDefaultHeapFunctions->alloc = findExportAddress("internal_allocatorDefaultHeapAlloc");
   sDefaultHeapFunctions->free = findExportAddress("internal_allocatorDefaultHeapFree");

   sBlockHeapFunctions = internal::sysAlloc<AllocatorFunctions>();
   sBlockHeapFunctions->alloc = findExportAddress("internal_allocatorBlockHeapAlloc");
   sBlockHeapFunctions->free = findExportAddress("internal_allocatorBlockHeapFree");

   sExpHeapFunctions = internal::sysAlloc<AllocatorFunctions>();
   sExpHeapFunctions->alloc = findExportAddress("internal_allocatorExpHeapAlloc");
   sExpHeapFunctions->free = findExportAddress("internal_allocatorExpHeapFree");

   sFrameHeapFunctions = internal::sysAlloc<AllocatorFunctions>();
   sFrameHeapFunctions->alloc = findExportAddress("internal_allocatorFrmHeapAlloc");
   sFrameHeapFunctions->free = findExportAddress("internal_allocatorFrmHeapFree");

   sUnitHeapFunctions = internal::sysAlloc<AllocatorFunctions>();
   sUnitHeapFunctions->alloc = findExportAddress("internal_allocatorUnitHeapAlloc");
   sUnitHeapFunctions->free = findExportAddress("internal_allocatorUnitHeapFree");
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

   RegisterKernelFunctionName("internal_allocatorDefaultHeapAlloc", allocatorDefaultHeapAlloc);
   RegisterKernelFunctionName("internal_allocatorDefaultHeapFree", allocatorDefaultHeapFree);
   RegisterKernelFunctionName("internal_allocatorBlockHeapAlloc", allocatorBlockHeapAlloc);
   RegisterKernelFunctionName("internal_allocatorBlockHeapFree", allocatorBlockHeapFree);
   RegisterKernelFunctionName("internal_allocatorExpHeapAlloc", allocatorExpHeapAlloc);
   RegisterKernelFunctionName("internal_allocatorExpHeapFree", allocatorExpHeapFree);
   RegisterKernelFunctionName("internal_allocatorFrmHeapAlloc", allocatorFrmHeapAlloc);
   RegisterKernelFunctionName("internal_allocatorFrmHeapFree", allocatorFrmHeapFree);
   RegisterKernelFunctionName("internal_allocatorUnitHeapAlloc", allocatorUnitHeapAlloc);
   RegisterKernelFunctionName("internal_allocatorUnitHeapFree", allocatorUnitHeapFree);
}

} // namespace coreinit
