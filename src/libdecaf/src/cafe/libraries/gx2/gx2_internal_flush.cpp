#include "decaf_graphics.h"
#include "gx2_internal_flush.h"
#include "gx2_internal_pm4cap.h"

namespace cafe::gx2::internal
{

void
notifyCpuFlush(phys_addr address,
               uint32_t size)
{
   captureCpuFlush(address, size);
   decaf::getGraphicsDriver()->notifyCpuFlush(address, size);
}

void
notifyGpuFlush(phys_addr address,
               uint32_t size)
{
   captureGpuFlush(address, size);
   decaf::getGraphicsDriver()->notifyGpuFlush(address, size);
}

} // namespace cafe::gx2::internal
