#include <algorithm>
#include "coreinit.h"
#include "coreinit_dynload.h"
#include "coreinit_memheap.h"
#include "coreinit_expheap.h"
#include "interpreter.h"
#include "system.h"

static wfunc_ptr<int, int, int, be_val<uint32_t>*>
pOSDynLoad_MemAlloc;

static wfunc_ptr<void, void *>
pOSDynLoad_MemFree;

int MEM_DynLoad_DefaultAlloc(int size, int alignment, be_val<uint32_t> *outPtr)
{
   auto heap = MEMGetBaseHeapHandle(BaseHeapType::MEM2);
   auto memory = MEMAllocFromExpHeapEx(reinterpret_cast<ExpandedHeap*>(heap), size, alignment);
   *outPtr = gMemory.untranslate(memory);
   return 0;
}

void MEM_DynLoad_DefaultFree(void *addr)
{
   auto heap = MEMGetBaseHeapHandle(BaseHeapType::MEM2);
   MEMFreeToExpHeap(reinterpret_cast<ExpandedHeap*>(heap), addr);
}

int
OSDynLoad_SetAllocator(uint32_t allocFn, uint32_t freeFn)
{
   if (!allocFn || !freeFn) {
      return 0xBAD10017;
   }

   pOSDynLoad_MemAlloc = allocFn;
   pOSDynLoad_MemFree = freeFn;
   return 0;
}

int
OSDynLoad_GetAllocator(be_val<uint32_t> *outAllocFn, be_val<uint32_t> *outFreeFn)
{
   *outAllocFn = static_cast<uint32_t>(pOSDynLoad_MemAlloc);
   *outFreeFn = static_cast<uint32_t>(pOSDynLoad_MemFree);
   return 0;
}

// Wrapper func to call function pointer set by OSDynLoad_SetAllocator
int
OSDynLoad_MemAlloc(int size, int alignment, uint32_t *outPtr)
{
   be_val<uint32_t> value;
   auto result = pOSDynLoad_MemAlloc(size, alignment, &value);
   *outPtr = value;
   return result;
}

// Wrapper func to call function pointer set by OSDynLoad_SetAllocator
void
OSDynLoad_MemFree(void *addr)
{
   pOSDynLoad_MemFree(addr);
}

int
OSDynLoad_Acquire(char const *name, be_val<uint32_t> *outHandle)
{
   auto module = gSystem.findModule(name);

   if (module) {
      *outHandle = static_cast<uint32_t>(module->getHandle());
      return 0;
   } else {
      *outHandle = 0;
      return 0xBAD10001;
   }
}

int
OSDynLoad_FindExport(LoadedModule *module, int isData, char const *name, be_val<uint32_t> *outAddr)
{
   uint32_t addr = 0;

   if (module->type == LoadedModule::Kernel) {
      auto kmod = reinterpret_cast<KernelModule*>(module->ptr);
      auto exp = kmod->findExport(name);

      if (!exp) {
         gLog->debug("OSDynLoad_FindExport export {} not found", name);
      } else if (exp->type == KernelExport::Data) {
         if (isData == 0) {
            gLog->debug("OSDynLoad_FindExport isData == 0 for data export {}", name);
         } else {
            auto kdata = reinterpret_cast<KernelData *>(exp);
            addr = static_cast<uint32_t>(*kdata->vptr);
         }
      } else if (exp->type == KernelExport::Function) {
         if (isData == 1) {
            gLog->debug("OSDynLoad_FindExport isData == 1 for function export {}", name);
         } else {
            auto kfunc = reinterpret_cast<KernelFunction *>(exp);
            addr = kfunc->vaddr;
         }
      }
   }

   if (addr) {
      *outAddr = addr;
      return 0;
   } else {
      *outAddr = 0;
      return 0xBAD10001;
   }
}

void
OSDynLoad_Release(DynLoadModuleHandle handle)
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
