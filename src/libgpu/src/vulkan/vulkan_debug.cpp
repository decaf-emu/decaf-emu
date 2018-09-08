#ifdef DECAF_VULKAN
#include "vulkan_driver.h"

namespace vulkan
{

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
Driver::updateDebuggerInfo()
{
   // TODO: Actually update this with useful information
}

} // namespace vulkan

#endif // DECAF_VULKAN
