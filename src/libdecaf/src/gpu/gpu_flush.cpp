#include "gpu_flush.h"
#include "decaf_graphics.h"

namespace gpu
{

void
notifyCpuFlush(void *ptr,
               uint32_t size)
{
   decaf::getGraphicsDriver()->notifyCpuFlush(ptr, size);
}

void
notifyGpuFlush(void *ptr,
               uint32_t size)
{
   decaf::getGraphicsDriver()->notifyGpuFlush(ptr, size);
}

} // namespace gpu
