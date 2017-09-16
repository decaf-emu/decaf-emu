#include "vulkan_driver.h"
#include "gpu_event.h"
#include "gpu_ringbuffer.h"

namespace vulkan
{

void
Driver::run()
{
}

void
Driver::stop()
{
}

gpu::GraphicsDriverType
Driver::type()
{
   return gpu::GraphicsDriverType::Vulkan;
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
Driver::notifyCpuFlush(void *ptr,
                       uint32_t size)
{
}

void
Driver::notifyGpuFlush(void *ptr,
                       uint32_t size)
{
}

} // namespace vulkan
