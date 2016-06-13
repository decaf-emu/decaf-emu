#pragma once
#include "common/types.h"

namespace coreinit
{

/**
 * \defgroup coreinit_lockedcache Locked Cache
 * \ingroup coreinit
 * @{
 */

BOOL
LCHardwareIsAvailable();

void *
LCAlloc(uint32_t size);

void
LCDealloc(void * addr);

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
LCLoadDMABlocks(void *dst, const void *src, uint32_t size);

void
LCStoreDMABlocks(void *dst, const void *src, uint32_t size);

void
LCWaitDMAQueue(uint32_t queueLength);

/** @} */

} // namespace coreinit
