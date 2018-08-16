#pragma once
#include <libcpu/be2_struct.h>

namespace cafe::coreinit
{

/**
 * \defgroup coreinit_cache Cache
 * \ingroup coreinit
 * @{
 */

void
DCInvalidateRange(virt_ptr<void> ptr,
                  uint32_t size);

void
DCFlushRange(virt_ptr<void> ptr,
             uint32_t size);

void
DCStoreRange(virt_ptr<void> ptr,
             uint32_t size);

void
DCFlushRangeNoSync(virt_ptr<void> ptr,
                   uint32_t size);

void
DCStoreRangeNoSync(virt_ptr<void> ptr,
                   uint32_t size);

void
DCZeroRange(virt_ptr<void> ptr,
            uint32_t size);

void
DCTouchRange(virt_ptr<void> ptr,
             uint32_t size);

void
OSCoherencyBarrier();

void
OSEnforceInorderIO();

BOOL
OSIsAddressRangeDCValid(virt_ptr<void> ptr,
                        uint32_t size);

void
OSMemoryBarrier();

/** @} */

} // namespace cafe::coreinit
