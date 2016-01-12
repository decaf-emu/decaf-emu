#include <algorithm>
#include "coreinit.h"
#include "coreinit_dynload.h"
#include "coreinit_memheap.h"
#include "coreinit_expheap.h"
#include "utils/wfunc_call.h"
#include "memory_translate.h"
#include "system.h"
#include "utils/be_val.h"
#include "utils/virtual_ptr.h"
#include "utils/wfunc_ptr.h"

static wfunc_ptr<int, int, int, be_val<uint32_t>*>
gMemAlloc;

static wfunc_ptr<void, void *>
gMemFree;


/**
 * Default implementation for gMemAlloc
 */
static int
MEM_DynLoad_DefaultAlloc(int size, int alignment, be_val<uint32_t> *outPtr)
{
   auto heap = MEMGetBaseHeapHandle(MEMBaseHeapType::MEM2);
   auto memory = MEMAllocFromExpHeapEx(reinterpret_cast<ExpandedHeap*>(heap), size, alignment);
   *outPtr = memory_untranslate(memory);
   return 0;
}


/**
 * Default implementation for gMemFree
 */
static void
MEM_DynLoad_DefaultFree(uint8_t *addr)
{
   auto heap = MEMGetBaseHeapHandle(MEMBaseHeapType::MEM2);
   MEMFreeToExpHeap(reinterpret_cast<ExpandedHeap*>(heap), addr);
}


/**
 * Set the allocator to be used for allocating data sections for dynamically loaded libraries.
 */
int
OSDynLoad_SetAllocator(ppcaddr_t allocFn, ppcaddr_t freeFn)
{
   if (!allocFn || !freeFn) {
      return 0xBAD10017;
   }

   gMemAlloc = allocFn;
   gMemFree = freeFn;
   return 0;
}


/**
 * Return the allocators set by OSDynLoad_SetAllocator.
 */
int
OSDynLoad_GetAllocator(be_val<ppcaddr_t> *outAllocFn, be_val<ppcaddr_t> *outFreeFn)
{
   *outAllocFn = static_cast<ppcaddr_t>(gMemAlloc);
   *outFreeFn = static_cast<ppcaddr_t>(gMemFree);
   return 0;
}


/**
 * Load and return the handle of a dynamic library.
 *
 * If the library is already loaded then increase ref count.
 */
int
OSDynLoad_Acquire(char const *name, be_ptr<LoadedModuleHandleData> *outHandle)
{
   auto* module = gLoader.loadRPL(name);

   if (!module) {
      gLog->debug("OSDynLoad_Acquire {} failed", name);
      *outHandle = nullptr;
      return 0xBAD10001;
   }

   *outHandle = module->handle;
   return 0;
}


/**
 * Find the export from a library handle.
 */
int
OSDynLoad_FindExport(LoadedModuleHandleData *handle, int isData, char const *name, be_val<ppcaddr_t> *outAddr)
{
   auto module = handle->ptr;
   auto exportPtr = module->findExport(name);

   if (!exportPtr) {
      gLog->debug("OSDynLoad_FindExport export {} not found", name);
      *outAddr = 0;
      return 0xBAD10001;
   }

   *outAddr = exportPtr;
   return 0;
}


/**
 * Release handle of loaded library.
 *
 * Decreases the ref count of the library pointed to by handle.
 * The library is unloaded if ref count hits 0.
 */
void
OSDynLoad_Release(LoadedModuleHandleData *handle)
{
   // TODO: Unload library when ref count hits 0
}

void
CoreInit::initialiseDynLoad()
{
   gMemAlloc = findExportAddress("MEM_DynLoad_DefaultAlloc");
   gMemFree = findExportAddress("MEM_DynLoad_DefaultFree");
}

void
CoreInit::registerDynLoadFunctions()
{
   RegisterKernelFunction(OSDynLoad_Acquire);
   RegisterKernelFunction(OSDynLoad_FindExport);
   RegisterKernelFunction(OSDynLoad_Release);
   RegisterKernelFunction(OSDynLoad_SetAllocator);
   RegisterKernelFunction(OSDynLoad_GetAllocator);
   RegisterKernelFunction(MEM_DynLoad_DefaultAlloc);
   RegisterKernelFunction(MEM_DynLoad_DefaultFree);
}

namespace coreinit
{

namespace internal
{

/**
 * Wrapper func to call function pointer set by OSDynLoad_SetAllocator
 */
int
dynLoadMemAlloc(int size, int alignment, void **outPtr)
{
   auto value = coreinit::internal::sysAlloc<be_val<ppcaddr_t>>();
   auto result = gMemAlloc(size, alignment, value);
   *outPtr = memory_translate(*value);
   coreinit::internal::sysFree(value);
   return result;
}


/**
 * Wrapper func to call function pointer set by OSDynLoad_SetAllocator
 */
void
dynLoadMemFree(void *addr)
{
   gMemFree(addr);
}

} // namespace internal

} // namespace coreinit
