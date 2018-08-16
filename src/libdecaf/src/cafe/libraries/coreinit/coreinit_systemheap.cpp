#include "coreinit.h"
#include "coreinit_cache.h"
#include "coreinit_cosreport.h"
#include "coreinit_memexpheap.h"
#include "coreinit_systemheap.h"
#include "coreinit_systeminfo.h"

namespace cafe::coreinit
{

struct StaticSystemHeapData
{
   be2_val<MEMHeapHandle> handle;
   be2_val<uint32_t> numAllocs;
   be2_val<uint32_t> numFrees;
};

static virt_ptr<StaticSystemHeapData>
sSystemHeapData = nullptr;


/**
 * Allocate memory from the system heap.
 *
 * \param size
 * The size of the memory block to allocate.
 *
 * \param align
 * The alignment of the memory block, see MEMAllocFromExpHeapEx for more info.
 *
 * \return
 * NULL on error, or pointer to newly allocated memory on success.
 */
virt_ptr<void>
OSAllocFromSystem(uint32_t size,
                  int32_t align)
{
   auto ptr = MEMAllocFromExpHeapEx(sSystemHeapData->handle, size, align);

   if (internal::isAppDebugLevelVerbose()) {
      internal::COSInfo(COSReportModule::Unknown2,
              "SYSTEM_HEAP:%d,ALLOC,=\"0%08x\",-%d\n",
              sSystemHeapData->numAllocs,
              virt_cast<virt_addr>(ptr).getAddress(),
              size);
      ++sSystemHeapData->numAllocs;
   }

   return ptr;
}


/**
 * Free memory to the system heap.
 *
 * \param ptr
 * The memory to free, this pointer must have previously be returned by
 * OSAllocFromSystem.
 */
void
OSFreeToSystem(virt_ptr<void> ptr)
{
   if (internal::isAppDebugLevelVerbose()) {
      internal::COSInfo(COSReportModule::Unknown2,
              "SYSTEM_HEAP:%d,FREE,=\"0%08x\",%d\n",
              sSystemHeapData->numAllocs,
              virt_cast<virt_addr>(ptr).getAddress(),
              0);
      ++sSystemHeapData->numAllocs;
   }

   MEMFreeToExpHeap(sSystemHeapData->handle, ptr);
}

namespace internal
{

void
dumpSystemHeap()
{
   MEMDumpHeap(sSystemHeapData->handle);
}

void
initialiseSystemHeap(virt_ptr<void> base,
                     uint32_t size)
{
   if (internal::isAppDebugLevelVerbose()) {
      COSInfo(COSReportModule::Unknown2,
              "RPL_SYSHEAP:Event,Change,Hex Addr,Bytes,Available\n");
      COSInfo(COSReportModule::Unknown2,
              "RPL_SYSHEAP:SYSHEAP START,CREATE,=\"0%08x\",%d\n",
              virt_cast<virt_addr>(base).getAddress(), size);
   }

   sSystemHeapData->handle = MEMCreateExpHeapEx(base,
                                                size,
                                                MEMHeapFlags::ThreadSafe);
   sSystemHeapData->numAllocs = 0u;
   sSystemHeapData->numFrees = 0u;
   OSMemoryBarrier();
}

} // namespace internal

void
Library::registerSystemHeapSymbols()
{
   RegisterFunctionExport(OSAllocFromSystem);
   RegisterFunctionExport(OSFreeToSystem);

   RegisterDataInternal(sSystemHeapData);
}

} // namespace cafe::coreinit
