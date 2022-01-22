#pragma once
#include "gx2_enum.h"

#include <libcpu/be2_struct.h>

namespace cafe::gx2
{

/**
 * \defgroup gx2_fence Fence
 * \ingroup gx2
 * @{
 */

void
GX2SetGPUFence(virt_ptr<uint32_t> memory,
               uint32_t mask,
               GX2CompareFunction op,
               uint32_t value);

/** @} */

} // namespace cafe::gx2
