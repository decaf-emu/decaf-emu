#include "coreinit.h"
#include "coreinit_cache.h"
#include "gpu/gpu_flush.h"
#include "common/align.h"

namespace coreinit
{


/**
 * Equivalent to dcbi instruction.
 */
void
DCInvalidateRange(void *addr, uint32_t size)
{
   // TODO: DCInvalidateRange

   // Also signal the GPU to update the memory range.
   gpu::notifyGpuFlush(addr, size);
}


/**
 * Equivalent to dcbf, sync, eieio.
 */
void
DCFlushRange(void *addr, uint32_t size)
{
   // TODO: DCFlushRange

   // Also signal the memory store to the GPU.
   gpu::notifyCpuFlush(addr, size);
}


/**
 * Equivalent to dcbst, sync, eieio.
 */
void
DCStoreRange(void *addr, uint32_t size)
{
   // TODO: DCStoreRange

   // Also signal the memory store to the GPU.
   gpu::notifyCpuFlush(addr, size);
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
   addr = align_down(addr, 32);
   size = align_up(size, 32);
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
