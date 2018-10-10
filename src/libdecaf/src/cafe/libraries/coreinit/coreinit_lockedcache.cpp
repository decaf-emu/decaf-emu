#include "coreinit.h"
#include "coreinit_core.h"
#include "coreinit_lockedcache.h"
#include "coreinit_memory.h"
#include "coreinit_mutex.h"
#include "coreinit_systeminfo.h"
#include "coreinit_thread.h"

#include <common/align.h>
#include <common/bitutils.h>

namespace cafe::coreinit
{

constexpr auto LCBlockSize = 512u;
constexpr auto LCMaxSize = 16u * 1024;

struct LockedCacheState
{
   //! Lock for this structure.
   be2_struct<OSMutex> mutex;

   //! Base address of locked cache memory.
   be2_val<virt_addr> baseAddress;

   //! Free size in bytes remaining in Locked Cache.
   be2_val<uint32_t> freeSize;

   //! The bitmask of the currently allocated blocks in the locked cache.
   be2_val<uint32_t> allocBitMask;

   //! Storage of the bitmasks of current allocations, only set for the first
   //! bit index of an allocation.
   be2_array<uint32_t, 32> allocatedMasks;

   //! The size of an allocation in bytes, only set for the first bit index of
   //! an allocation.
   be2_array<uint32_t, 32> allocatedSize;

   //! Reference count for LCEnableDMA / LCDisableDMA.
   be2_val<uint32_t> dmaRefCount;
};
CHECK_OFFSET(LockedCacheState, 0x00, mutex);
CHECK_OFFSET(LockedCacheState, 0x2C, baseAddress);
CHECK_OFFSET(LockedCacheState, 0x30, freeSize);
CHECK_OFFSET(LockedCacheState, 0x34, allocBitMask);
CHECK_OFFSET(LockedCacheState, 0x38, allocatedMasks);
CHECK_OFFSET(LockedCacheState, 0xB8, allocatedSize);
CHECK_OFFSET(LockedCacheState, 0x138, dmaRefCount);
CHECK_SIZE(LockedCacheState, 0x13C);

struct StaticLockedCacheData
{
   be2_array<LockedCacheState, OSGetCoreCount()> coreState;
};

virt_ptr<StaticLockedCacheData>
sLockedCacheData = nullptr;

virt_ptr<LockedCacheState>
getCoreLockedCacheState()
{
   return virt_addrof(sLockedCacheData->coreState[OSGetCoreId()]);
}


/**
 * Check if Locked Cache hardware is available on this process.
 */
BOOL
LCHardwareIsAvailable()
{
   if (OSGetCoreId() != 2) {
      return OSGetForegroundBucket(nullptr, nullptr);
   }

   auto upid = OSGetUPID();
   if (upid == kernel::UniqueProcessId::Game ||
       upid == kernel::UniqueProcessId::HomeMenu) {
      return TRUE;
   }

   return FALSE;
}


/**
 * Allocate some memory from the current core's Locked Cache.
 */
virt_ptr<void>
LCAlloc(uint32_t size)
{
   if (size > LCMaxSize) {
      return  nullptr;
   }

   auto lcState = getCoreLockedCacheState();
   auto result = virt_ptr<void> { nullptr };
   OSLockMutex(virt_addrof(lcState->mutex));

   if (lcState->freeSize >= size) {
      auto numBlocks = align_up(size, LCBlockSize) / LCBlockSize;
      auto bitMask = make_bitmask(numBlocks);
      auto index = 0u;

      // Find a free spot in the allocBitMask which can fit bitMask
      while (lcState->allocBitMask & (bitMask << index)) {
         if (index >= 32 - numBlocks) {
            break;
         }

         index++;
      }

      if ((index < 32 - numBlocks) ||
          (index == 0 && numBlocks == 32)) {
         // Do the allocation!
         auto mask = bitMask << index;
         auto size = numBlocks * LCBlockSize;

         lcState->allocBitMask |= mask;
         lcState->freeSize -= size;

         lcState->allocatedMasks[index] = mask;
         lcState->allocatedSize[index] = size;

         result = virt_cast<void *>(lcState->baseAddress + index * LCBlockSize);
      }
   }

   OSUnlockMutex(virt_addrof(lcState->mutex));
   return result;
}


/**
 * Free some memory to the current core's Locked Cache.
 */
void
LCDealloc(virt_ptr<void> ptr)
{
   auto lcState = getCoreLockedCacheState();
   auto addr = virt_cast<virt_addr>(ptr);

   if (addr < lcState->baseAddress ||
       addr >= lcState->baseAddress + LCMaxSize) {
      return;
   }

   OSLockMutex(virt_addrof(lcState->mutex));

   auto index = static_cast<uint32_t>((addr - lcState->baseAddress) / LCBlockSize);
   auto mask = lcState->allocatedMasks[index];
   auto size = lcState->allocatedSize[index];

   lcState->allocBitMask &= ~mask;
   lcState->freeSize += size;

   lcState->allocatedMasks[index] = 0u;
   lcState->allocatedSize[index] = 0u;

   OSUnlockMutex(virt_addrof(lcState->mutex));
}


/**
 * Get the maximum size of the current core's Locked Cache.
 */
uint32_t
LCGetMaxSize()
{
   return LCMaxSize;
}


/**
 * Get the largest allocatable size of the current core's Locked Cache.
 */
uint32_t
LCGetAllocatableSize()
{
   auto lcState = getCoreLockedCacheState();
   OSLockMutex(virt_addrof(lcState->mutex));

   // Find the largest span of 0 in the allocBitMask
   auto largestBitSpan = 0u;
   auto currentSpanSize = 0u;

   for (auto i = 0u; i < 32; ++i) {
      if (lcState->allocBitMask & (1 << i)) {
         largestBitSpan = std::max(currentSpanSize, largestBitSpan);
         currentSpanSize = 0u;
         continue;
      }

      currentSpanSize++;
   }

   OSUnlockMutex(virt_addrof(lcState->mutex));
   return largestBitSpan * LCBlockSize;
}


/**
 * Get the total amount of unallocated memory in the current core's Locked Cache.
 */
uint32_t
LCGetUnallocated()
{
   auto lcState = getCoreLockedCacheState();
   return lcState->freeSize;
}


/**
 * Check if DMA is enabled for the current core.
 */
BOOL
LCIsDMAEnabled()
{
   auto lcState = getCoreLockedCacheState();
   return lcState->dmaRefCount > 0 ? TRUE : FALSE;
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

   auto lcState = getCoreLockedCacheState();
   lcState->dmaRefCount++;
   return TRUE;
}


/**
 * Disable DMA for current core.
 */
void
LCDisableDMA()
{
   auto lcState = getCoreLockedCacheState();
   lcState->dmaRefCount--;

   if (lcState->dmaRefCount == 0) {
      LCWaitDMAQueue(0);
   }
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
LCLoadDMABlocks(virt_ptr<void> dst,
                virt_ptr<const void> src,
                uint32_t size)
{
   // TODO: Notify GPU
   // Signal the GPU to update the source range if necessary, as with
   //  DCInvalidateRange().
   // gx2::internal::notifyGpuFlush(const_cast<void *>(src), size);

   if (size == 0) {
      size = 128;
   }

   std::memcpy(dst.get(), src.get(), size * 32);
}


/**
 * Add a DMA store request to the queue.
 *
 * We fake this by performing the store immediately.
 */
void
LCStoreDMABlocks(virt_ptr<void> dst,
                 virt_ptr<const void> src,
                 uint32_t size)
{
   if (size == 0) {
      size = 128;
   }

   std::memcpy(dst.get(), src.get(), size * 32);

   // TODO: Notify GPU
   // Also signal the memory store to the GPU, as with DCFlushRange().
   // gx2::internal::notifyCpuFlush(dst, size);
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


namespace internal
{

void
initialiseLockedCache(uint32_t coreId)
{
   auto &state = sLockedCacheData->coreState[coreId];
   OSInitMutex(virt_addrof(state.mutex));

   state.baseAddress = getLockedCacheBaseAddress(coreId);
   state.freeSize = 0x4000u;
   state.allocBitMask = 0u;
   state.allocatedMasks.fill(0u);
   state.allocatedSize.fill(0u);
   state.dmaRefCount = 0u;
}

} // namespace internal

void
Library::registerLockedCacheSymbols()
{
   RegisterFunctionExport(LCHardwareIsAvailable);
   RegisterFunctionExport(LCAlloc);
   RegisterFunctionExport(LCDealloc);
   RegisterFunctionExport(LCGetMaxSize);
   RegisterFunctionExport(LCGetAllocatableSize);
   RegisterFunctionExport(LCGetUnallocated);
   RegisterFunctionExport(LCIsDMAEnabled);
   RegisterFunctionExport(LCEnableDMA);
   RegisterFunctionExport(LCDisableDMA);
   RegisterFunctionExport(LCGetDMAQueueLength);
   RegisterFunctionExport(LCLoadDMABlocks);
   RegisterFunctionExport(LCStoreDMABlocks);
   RegisterFunctionExport(LCWaitDMAQueue);

   RegisterDataInternal(sLockedCacheData);
}

} // namespace cafe::coreinit
