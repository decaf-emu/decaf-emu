#include "coreinit.h"
#include "coreinit_cache.h"
#include "util.h"

void
DCInvalidateRange(void *addr, uint32_t size)
{
}

void
DCFlushRange(void *addr, uint32_t size)
{
}

void
DCStoreRange(void *addr, uint32_t size)
{
}

void
DCFlushRangeNoSync(void *addr, uint32_t size)
{
}

void
DCStoreRangeNoSync(void *addr, uint32_t size)
{
}

void
DCZeroRange(void *addr, uint32_t size)
{
   // TODO: Check align direction is correct!
   size = alignDown(size, 32);
   addr = alignUp(addr, 32);
   memset(addr, 0, size);
}

void
DCTouchRange(void *addr, uint32_t size)
{
}

void
CoreInit::registerCacheFunctions()
{
   RegisterKernelFunction(DCInvalidateRange);
   RegisterKernelFunction(DCFlushRange);
   RegisterKernelFunction(DCStoreRange);
   RegisterKernelFunction(DCFlushRangeNoSync);
   RegisterKernelFunction(DCStoreRangeNoSync);
   RegisterKernelFunction(DCZeroRange);
   RegisterKernelFunction(DCTouchRange);
}