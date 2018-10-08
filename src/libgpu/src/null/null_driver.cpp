#include "null_driver.h"
#include "gpu_event.h"
#include "gpu_ringbuffer.h"

namespace null
{

void
Driver::run()
{
   mRunning = true;

   while (mRunning) {
      if (gpu::ringbuffer::wait()) {
         auto items = gpu::ringbuffer::read();
         // TODO: We need to actually process pm4 to do EVENT_WRITE_EOP for retired timestamps
      }
   }
}

void
Driver::stop()
{
   mRunning = false;
   gpu::ringbuffer::wake();
}

void
Driver::runUntilFlip()
{
   // The null driver does not support "force sync"
   decaf_check(0);
}

gpu::GraphicsDriverType
Driver::type()
{
   return gpu::GraphicsDriverType::Null;
}

float
Driver::getAverageFPS()
{
   return 0.0f;
}

float
Driver::getAverageFrametimeMS()
{
   return 0.0f;
}

void
Driver::notifyCpuFlush(phys_addr address,
                       uint32_t size)
{
}

void
Driver::notifyGpuFlush(phys_addr address,
                       uint32_t size)
{
}

} // namespace null
