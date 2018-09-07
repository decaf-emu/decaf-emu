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
      auto buffer = gpu::ringbuffer::waitForItem();

      if (buffer.numWords) {
         continue;
      }

      gpu::onRetire(buffer.context);
   }
}

void
Driver::stop()
{
   mRunning = false;
   gpu::ringbuffer::awaken();
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
