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
DCInvalidateRange(virt_addr address,
                  uint32_t size);

void
DCFlushRange(virt_addr address,
             uint32_t size);

void
DCStoreRange(virt_addr address,
             uint32_t size);

void
DCFlushRangeNoSync(virt_addr address,
                   uint32_t size);

void
DCStoreRangeNoSync(virt_addr address,
                   uint32_t size);

void
DCZeroRange(virt_addr address,
            uint32_t size);

void
DCTouchRange(virt_addr address,
             uint32_t size);

void
OSCoherencyBarrier();

void
OSEnforceInorderIO();

BOOL
OSIsAddressRangeDCValid(virt_addr address,
                        uint32_t size);

void
OSMemoryBarrier();

/** @} */

} // namespace cafe::coreinit
