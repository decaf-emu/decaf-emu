#include "coreinit.h"
#include "coreinit_core.h"
#include "coreinit_lockedcache.h"
#include "coreinit_thread.h"
#include "kernel/kernel_memory.h"
#include "modules/gx2/gx2_internal_flush.h"

#include <common/teenyheap.h>
#include <array>

namespace coreinit
{

static const auto
sLockedCacheAlign = 512;

static const auto
sLockedCacheSize = 16u * 1024;

static std::array<TeenyHeap *, CoreCount>
sLockedCache;

static std::array<bool, CoreCount>
sDMAEnabled;

BOOL
LCHardwareIsAvailable()
{
   return TRUE;
}


/**
 * Allocate some memory from the current core's Locked Cache.
 */
void *
LCAlloc(uint32_t size)
{
   auto cache = sLockedCache[OSGetCoreId()];
   return cache->alloc(size, sLockedCacheAlign);
}


/**
 * Free some memory to the current core's Locked Cache.
 */
void
LCDealloc(void *addr)
{
   auto cache = sLockedCache[OSGetCoreId()];
   cache->free(addr);
}


/**
 * Get the maximum size of the current core's Locked Cache.
 */
uint32_t
LCGetMaxSize()
{
   return sLockedCacheSize;
}


/**
 * Get the largest allocatable size of the current core's Locked Cache.
 */
uint32_t
LCGetAllocatableSize()
{
   auto cache = sLockedCache[OSGetCoreId()];
   return static_cast<uint32_t>(cache->getLargestFreeSize());
}


/**
 * Get the total amount of unallocated memory in the current core's Locked Cache.
 */
uint32_t
LCGetUnallocated()
{
   auto cache = sLockedCache[OSGetCoreId()];
   return static_cast<uint32_t>(cache->getTotalFreeSize());
}


/**
 * Check if DMA is enabled for the current core.
 */
BOOL
LCIsDMAEnabled()
{
   return sDMAEnabled[OSGetCoreId()] ? TRUE : FALSE;
}


/**
 * Enable DMA for the current core.
 *
 * Only a thread with affinity set to run only on current core can enable DMA.
 */
BOOL
LCEnableDMA()
{
   auto thread = OSGetCurrentThread();
   auto core = OSGetCoreId();
   auto affinity = thread->attr & OSThreadAttributes::AffinityAny;

   // Ensure thread can only execute on current core
   if (core == 0 && affinity != OSThreadAttributes::AffinityCPU0) {
      return FALSE;
   }

   if (core == 1 && affinity != OSThreadAttributes::AffinityCPU1) {
      return FALSE;
   }

   if (core == 2 && affinity != OSThreadAttributes::AffinityCPU2) {
      return FALSE;
   }

   sDMAEnabled[OSGetCoreId()] = true;
   return TRUE;
}


/**
 * Disable DMA for current core.
 */
void
LCDisableDMA()
{
   sDMAEnabled[OSGetCoreId()] = false;
}


/**
 * Get the total number of pending DMA requests.
 */
uint32_t
LCGetDMAQueueLength()
{
   return 0;
}


/**
 * Add a DMA load request to the queue.
 *
 * We fake this by performing the load immediately.
 */
void
LCLoadDMABlocks(void *dst,
                const void *src,
                uint32_t size)
{
   // Signal the GPU to update the source range if necessary, as with
   //  DCInvalidateRange().
   gx2::internal::notifyGpuFlush(const_cast<void *>(src), size);

   if (size == 0) {
      size = 128;
   }

   std::memcpy(dst, src, size * 32);
}


/**
 * Add a DMA store request to the queue.
 *
 * We fake this by performing the store immediately.
 */
void
LCStoreDMABlocks(void *dst,
                 const void *src,
                 uint32_t size)
{
   if (size == 0) {
      size = 128;
   }

   std::memcpy(dst, src, size * 32);

   // Also signal the memory store to the GPU, as with DCFlushRange().
   gx2::internal::notifyCpuFlush(dst, size);
}


/**
 * Wait until the DMA queue is a certain length.
 *
 * As we fake DMA this can return immediately.
 */
void
LCWaitDMAQueue(uint32_t queueLength)
{
}


void
Module::initialiseLockedCache()
{
   auto bounds = kernel::getVirtualRange(kernel::VirtualRegion::LockedCache);
   auto base = bounds.start;
   sDMAEnabled.fill(false);

   for (auto i = 0u; i < CoreCount; ++i) {
      auto ptr = virt_cast<void *>(base).getRawPointer();
      sLockedCache[i] = new TeenyHeap { ptr, 16 * 1024 };
      base += 16 * 1024;
   }
}

void
Module::registerLockedCacheFunctions()
{
   RegisterKernelFunction(LCHardwareIsAvailable);
   RegisterKernelFunction(LCAlloc);
   RegisterKernelFunction(LCDealloc);
   RegisterKernelFunction(LCGetMaxSize);
   RegisterKernelFunction(LCGetAllocatableSize);
   RegisterKernelFunction(LCGetUnallocated);
   RegisterKernelFunction(LCIsDMAEnabled);
   RegisterKernelFunction(LCEnableDMA);
   RegisterKernelFunction(LCDisableDMA);
   RegisterKernelFunction(LCGetDMAQueueLength);
   RegisterKernelFunction(LCLoadDMABlocks);
   RegisterKernelFunction(LCStoreDMABlocks);
   RegisterKernelFunction(LCWaitDMAQueue);
}

} // namespace coreinit
