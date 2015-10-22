#include <algorithm>
#include "coreinit.h"
#include "coreinit_dynload.h"
#include "coreinit_memheap.h"
#include "coreinit_expheap.h"
#include "utils/wfunc_call.h"
#include "memory_translate.h"
#include "system.h"
#include "utils/be_data.h"
#include "utils/be_val.h"
#include "utils/virtual_ptr.h"
#include "utils/wfunc_ptr.h"

static wfunc_ptr<int, int, int, be_val<uint32_t>*>
pOSDynLoad_MemAlloc;

static wfunc_ptr<void, void *>
pOSDynLoad_MemFree;

static int
MEM_DynLoad_DefaultAlloc(int size, int alignment, be_val<uint32_t> *outPtr)
{
   auto heap = MEMGetBaseHeapHandle(BaseHeapType::MEM2);
   auto memory = MEMAllocFromExpHeapEx(reinterpret_cast<ExpandedHeap*>(heap), size, alignment);
   *outPtr = memory_untranslate(memory);
   return 0;
}

static void
MEM_DynLoad_DefaultFree(uint8_t *addr)
{
   auto heap = MEMGetBaseHeapHandle(BaseHeapType::MEM2);
   MEMFreeToExpHeap(reinterpret_cast<ExpandedHeap*>(heap), addr);
}

int
OSDynLoad_SetAllocator(ppcaddr_t allocFn, ppcaddr_t freeFn)
{
   if (!allocFn || !freeFn) {
      return 0xBAD10017;
   }

   pOSDynLoad_MemAlloc = allocFn;
   pOSDynLoad_MemFree = freeFn;
   return 0;
}

int
OSDynLoad_GetAllocator(be_val<ppcaddr_t> *outAllocFn, be_val<ppcaddr_t> *outFreeFn)
{
   *outAllocFn = static_cast<ppcaddr_t>(pOSDynLoad_MemAlloc);
   *outFreeFn = static_cast<ppcaddr_t>(pOSDynLoad_MemFree);
   return 0;
}

// Wrapper func to call function pointer set by OSDynLoad_SetAllocator
int
OSDynLoad_MemAlloc(int size, int alignment, void **outPtr)
{
   be_data<uint32_t> value;
   auto result = pOSDynLoad_MemAlloc(size, alignment, &value);
   *outPtr = memory_translate(value);
   return result;
}

// Wrapper func to call function pointer set by OSDynLoad_SetAllocator
void
OSDynLoad_MemFree(void *addr)
{
   pOSDynLoad_MemFree(addr);
}

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

void
OSDynLoad_Release(LoadedModuleHandleData *handle)
{
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

void
CoreInit::initialiseDynLoad()
{
   pOSDynLoad_MemAlloc = findExportAddress("MEM_DynLoad_DefaultAlloc");
   pOSDynLoad_MemFree = findExportAddress("MEM_DynLoad_DefaultFree");
}
