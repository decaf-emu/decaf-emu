#pragma once
#include <libcpu/be2_struct.h>

namespace cafe::coreinit
{

/**
 * \defgroup coreinit_lockedcache Locked Cache
 * \ingroup coreinit
 * @{
 */

BOOL
LCHardwareIsAvailable();

virt_ptr<void>
LCAlloc(uint32_t size);

void
LCDealloc(virt_ptr<void> addr);

uint32_t
LCGetMaxSize();

uint32_t
LCGetAllocatableSize();

uint32_t
LCGetUnallocated();

BOOL
LCIsDMAEnabled();

BOOL
LCEnableDMA();

void
LCDisableDMA();

uint32_t
LCGetDMAQueueLength();

void
LCLoadDMABlocks(virt_ptr<void> dst,
                virt_ptr<const void> src,
                uint32_t size);

void
LCStoreDMABlocks(virt_ptr<void> dst,
                 virt_ptr<const void> src,
                 uint32_t size);

void
LCWaitDMAQueue(uint32_t queueLength);

namespace internal
{

void
initialiseLockedCache(uint32_t coreId);

} // namespace internal

/** @} */

} // namespace cafe::coreinit
