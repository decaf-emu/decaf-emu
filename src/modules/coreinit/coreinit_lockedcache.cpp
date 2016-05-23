#include <array>
#include "coreinit.h"
#include "coreinit_core.h"
#include "coreinit_lockedcache.h"
#include "coreinit_thread.h"
#include "mem/mem.h"
#include "utils/teenyheap.h"

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

void *
LCAlloc(uint32_t size)
{
   auto cache = sLockedCache[OSGetCoreId()];
   return cache->alloc(size, sLockedCacheAlign);
}

void
LCDealloc(void * addr)
{
   auto cache = sLockedCache[OSGetCoreId()];
   cache->free(addr);
}

uint32_t
LCGetMaxSize()
{
   return sLockedCacheSize;
}

uint32_t
LCGetAllocatableSize()
{
   auto cache = sLockedCache[OSGetCoreId()];
   return static_cast<uint32_t>(cache->getLargestFreeSize());
}

uint32_t
LCGetUnallocated()
{
   auto cache = sLockedCache[OSGetCoreId()];
   return static_cast<uint32_t>(cache->getTotalFreeSize());
}

BOOL
LCIsDMAEnabled()
{
   return sDMAEnabled[OSGetCoreId()] ? TRUE : FALSE;
}

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

void
LCDisableDMA()
{
   sDMAEnabled[OSGetCoreId()] = false;
}

uint32_t
LCGetDMAQueueLength()
{
   return 0;
}

void
LCLoadDMABlocks(void *dst, const void *src, uint32_t size)
{
   if (!LCIsDMAEnabled()) {
      return;
   }

   if (size == 0) {
      size = 128;
   }

   std::memcpy(dst, src, size * 32);
}

void
LCStoreDMABlocks(void *dst, const void *src, uint32_t size)
{
   if (!LCIsDMAEnabled()) {
      return;
   }

   if (size == 0) {
      size = 128;
   }

   std::memcpy(dst, src, size * 32);
}

void
LCWaitDMAQueue(uint32_t queueLength)
{
}

void
Module::initialiseLockedCache()
{
   auto base = reinterpret_cast<uint8_t *>(memory_translate(mem::LockedCacheBase));
   sDMAEnabled.fill(false);

   for (auto i = 0u; i < CoreCount; ++i) {
      sLockedCache[i] = new TeenyHeap(base + (sLockedCacheSize * i), sLockedCacheSize);
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
