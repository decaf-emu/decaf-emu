#pragma once
#include <cstdint>

namespace gpu
{

void
notifyCpuFlush(void *addr,
               uint32_t size);

void
notifyGpuFlush(void *addr,
               uint32_t size);

} // namespace gpu
