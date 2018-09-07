#ifdef DECAF_VULKAN
#include "vulkan_driver.h"
#include "gpu_event.h"
#include "gpu_graphicsdriver.h"
#include "gpu_ringbuffer.h"

namespace vulkan
{

gpu::GraphicsDriverType
Driver::type()
{
   return gpu::GraphicsDriverType::Vulkan;
}

void
Driver::run()
{
}

void
Driver::stop()
{
}

void
Driver::runUntilFlip()
{
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

gpu::VulkanDriver::DebuggerInfo *
Driver::getDebuggerInfo()
{
   return &mDebuggerInfo;
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

} // namespace vulkan

#endif // DECAF_VULKAN
