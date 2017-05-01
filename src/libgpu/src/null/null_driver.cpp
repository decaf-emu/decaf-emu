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

float
Driver::getAverageFPS()
{
   return 0.0f;
}

float
Driver::getAverageFrametime()
{
   return 0.0f;
}

gpu::GraphicsDriver::GraphicsDebugInfo
Driver::getGraphicsDebugInfo() {
   return gpu::GraphicsDriver::GraphicsDebugInfo{ 0 };
}

void
Driver::notifyCpuFlush(void *ptr,
                           uint32_t size)
{
}

void
Driver::notifyGpuFlush(void *ptr,
                           uint32_t size)
{
}

} // namespace null
