#include "coreinit.h"
#include "coreinit_cache.h"

#include <common/align.h>

namespace cafe::coreinit
{

/**
 * Equivalent to dcbi instruction.
 */
void
DCInvalidateRange(virt_ptr<void> ptr,
                  uint32_t size)
{
   // Also signal the GPU to update the memory range.
   // TODO: gx2::internal::notifyGpuFlush(addr, size);
}


/**
 * Equivalent to dcbf, sync, eieio.
 */
void
DCFlushRange(virt_ptr<void> ptr,
             uint32_t size)
{
   // Also signal the memory store to the GPU.
   // TODO: gx2::internal::notifyCpuFlush(addr, size);
}


/**
 * Equivalent to dcbst, sync, eieio.
 */
void
DCStoreRange(virt_ptr<void> ptr,
             uint32_t size)
{
   // Also signal the memory store to the GPU.
   // TODO: gx2::internal::notifyCpuFlush(addr, size);
}


/**
 * Equivalent to dcbf.
 */
void
DCFlushRangeNoSync(virt_ptr<void> ptr,
                   uint32_t size)
{
   // TODO: DCFlushRangeNoSync
}


/**
 * Equivalent to dcbst.
 */
void
DCStoreRangeNoSync(virt_ptr<void> ptr,
                   uint32_t size)
{
   // TODO: DCStoreRangeNoSync
}


/**
 * Equivalent to dcbz instruction.
 */
void
DCZeroRange(virt_ptr<void> ptr,
            uint32_t size)
{
   auto addr = virt_cast<virt_addr>(ptr);
   auto alignedAddr = align_down(addr, 32);
   auto alignedSize = align_up(size, 32);
   std::memset(virt_cast<void *>(alignedAddr).getRawPointer(), 0, alignedSize);
}


/**
 * Equivalent to dcbt instruction.
 */
void
DCTouchRange(virt_ptr<void> ptr,
             uint32_t size)
{
   // TODO: DCTouchRange
}


/**
 * Equivalent to a sync instruction.
 */
void
OSCoherencyBarrier()
{
}

/**
 * Equivalent to an eieio instruction.
 */
void
OSEnforceInorderIO()
{
}


/**
 * Checks if a range of memory is "DC Valid"?
 *
 * Fails when the range is between 0xE8000000 or 0xEC000000
 */
BOOL
OSIsAddressRangeDCValid(virt_ptr<void> ptr,
                        uint32_t size)
{
   auto beg = virt_cast<virt_addr>(ptr);
   auto end = beg + size - 1;

   if (beg > virt_addr { 0xEC000000 } && end > virt_addr { 0xEC000000 }) {
      return TRUE;
   }

   if (beg < virt_addr { 0xE8000000 } && end < virt_addr { 0xE8000000 }) {
      return TRUE;
   }

   return FALSE;
}


/**
 * Equivalent to a eieio, sync.
 */
void
OSMemoryBarrier()
{
   std::atomic_thread_fence(std::memory_order_seq_cst);
}


void
Library::registerCacheSymbols()
{
   RegisterFunctionExport(DCInvalidateRange);
   RegisterFunctionExport(DCFlushRange);
   RegisterFunctionExport(DCStoreRange);
   RegisterFunctionExport(DCFlushRangeNoSync);
   RegisterFunctionExport(DCStoreRangeNoSync);
   RegisterFunctionExport(DCZeroRange);
   RegisterFunctionExport(DCTouchRange);
   RegisterFunctionExport(OSCoherencyBarrier);
   RegisterFunctionExport(OSEnforceInorderIO);
   RegisterFunctionExport(OSIsAddressRangeDCValid);
   RegisterFunctionExport(OSMemoryBarrier);
}

} // namespace cafe::coreinit
