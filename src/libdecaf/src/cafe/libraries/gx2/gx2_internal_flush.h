#pragma once
#include <libcpu/be2_struct.h>

namespace cafe::gx2::internal
{

void
notifyCpuFlush(phys_addr address,
               uint32_t size);

void
notifyGpuFlush(phys_addr address,
               uint32_t size);

} // namespace cafe::gx2::internal
