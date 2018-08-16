#include "decaf_graphics.h"
#include "gx2_internal_flush.h"
#include "gx2_internal_pm4cap.h"

namespace cafe::gx2::internal
{

void
notifyCpuFlush(void *ptr,
               uint32_t size)
{
   captureCpuFlush(ptr, size);
   decaf::getGraphicsDriver()->notifyCpuFlush(ptr, size);
}

void
notifyGpuFlush(void *ptr,
               uint32_t size)
{
   captureGpuFlush(ptr, size);
   decaf::getGraphicsDriver()->notifyGpuFlush(ptr, size);
}

} // namespace cafe::gx2::internal
