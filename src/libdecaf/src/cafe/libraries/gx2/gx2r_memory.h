#pragma once
#include "gx2_enum.h"
#include <libcpu/be2_struct.h>

namespace cafe::gx2
{

/**
 * \defgroup gx2r_memory GX2R Memory
 * \ingroup gx2
 * @{
 */

void
GX2RInvalidateMemory(GX2RResourceFlags flags,
                     virt_ptr<void> buffer,
                     uint32_t size);

/** @} */

} // namespace cafe::gx2
