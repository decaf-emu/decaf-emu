#include "coreinit.h"
#include "coreinit_cache.h"
#include "common/align.h"
#include "decaf_graphics.h"

namespace coreinit
{


/**
 * Equivalent to dcbi instruction.
 */
void
DCInvalidateRange(void *addr, uint32_t size)
{
   // TODO: DCInvalidateRange
}


/**
 * Equivalent to dcbf, sync, eieio.
 */
void
DCFlushRange(void *addr, uint32_t size)
{
   // TODO: DCFlushRange

   // Also signal the memory store to the GPU.
   decaf::getGraphicsDriver()->handleDCFlush(mem::untranslate(addr), size);
}


/**
 * Equivalent to dcbst, sync, eieio.
 */
void
DCStoreRange(void *addr, uint32_t size)
{
   // TODO: DCStoreRange

   // Also signal the memory store to the GPU.
   decaf::getGraphicsDriver()->handleDCFlush(mem::untranslate(addr), size);
}


/**
 * Equivalent to dcbf.
 *
 * Does not perform sync, eieio like DCFlushRange.
 */
void
DCFlushRangeNoSync(void *addr, uint32_t size)
{
   // TODO: DCFlushRangeNoSync
}


/**
 * Equivalent to dcbst.
 *
 * Does not perform sync, eieio like DCStoreRange.
 */
void
DCStoreRangeNoSync(void *addr, uint32_t size)
{
   // TODO: DCStoreRangeNoSync
}


/**
 * Equivalent to dcbz instruction.
 */
void
DCZeroRange(void *addr, uint32_t size)
{
   // TODO: DCZeroRange check align direction is correct!
   size = align_down(size, 32);
   addr = align_up(addr, 32);
   memset(addr, 0, size);
}


/**
 * Equivalent to dcbt instruction.
 */
void
DCTouchRange(void *addr, uint32_t size)
{
   // TODO: DCTouchRange
}

BOOL
OSIsAddressRangeDCValid(void *addr,
                        uint32_t size)
{
   return TRUE;
}

void
OSCoherencyBarrier()
{
   // TODO: OSCoherencyBarrier
}


void
Module::registerCacheFunctions()
{
   RegisterKernelFunction(DCInvalidateRange);
   RegisterKernelFunction(DCFlushRange);
   RegisterKernelFunction(DCStoreRange);
   RegisterKernelFunction(DCFlushRangeNoSync);
   RegisterKernelFunction(DCStoreRangeNoSync);
   RegisterKernelFunction(DCZeroRange);
   RegisterKernelFunction(DCTouchRange);
   RegisterKernelFunction(OSIsAddressRangeDCValid);
   RegisterKernelFunction(OSCoherencyBarrier);
}

} // namespace coreinit
