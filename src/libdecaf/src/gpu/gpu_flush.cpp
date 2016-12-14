#include "decaf_graphics.h"
#include "gpu_flush.h"
#include "pm4_capture.h"

namespace gpu
{

void
notifyCpuFlush(void *ptr,
               uint32_t size)
{
   pm4::captureCpuFlush(ptr, size);
   decaf::getGraphicsDriver()->notifyCpuFlush(ptr, size);
}

void
notifyGpuFlush(void *ptr,
               uint32_t size)
{
   pm4::captureGpuFlush(ptr, size);
   decaf::getGraphicsDriver()->notifyGpuFlush(ptr, size);
}

} // namespace gpu
