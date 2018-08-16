#pragma once
#include <cstdint>

namespace cafe::gx2::internal
{

void
notifyCpuFlush(void *ptr,
               uint32_t size);

void
notifyGpuFlush(void *ptr,
               uint32_t size);

} // namespace cafe::gx2::internal
