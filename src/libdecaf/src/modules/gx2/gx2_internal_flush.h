#pragma once
#include <cstdint>

namespace gx2
{

namespace internal
{

void
notifyCpuFlush(void *ptr,
               uint32_t size);

void
notifyGpuFlush(void *ptr,
               uint32_t size);

} // namespace internal

} // namespace gx2
