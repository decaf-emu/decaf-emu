#include "coreinit.h"
#include "coreinit_core.h"
#include "coreinit_lockedcache.h"
#include "utils/teenyheap.h"

static const auto
LockedCacheBase = 0xF8000000u;

static const auto
LockedCacheAlign = 512;

static const auto
LockedCacheSize = 16u * 1024;

static TeenyHeap *
gLockedCache[CoreCount];

void *
LCAlloc(uint32_t size)
{
   auto cache = gLockedCache[OSGetCoreId()];
   return cache->alloc(size, LockedCacheAlign);
}

void
LCDealloc(void * addr)
{
   auto cache = gLockedCache[OSGetCoreId()];
   cache->free(addr);
}


void
CoreInit::initialiseLockedCache()
{
   auto base = reinterpret_cast<uint8_t *>(memory_translate(LockedCacheBase));

   for (auto i = 0u; i < CoreCount; ++i) {
      gLockedCache[i] = new TeenyHeap(base + (LockedCacheSize * i), LockedCacheSize);
   }
}

void
CoreInit::registerLockedCacheFunctions()
{
   RegisterKernelFunction(LCAlloc);
   RegisterKernelFunction(LCDealloc);
}
