#pragma once
#ifdef DECAF_VULKAN
#include "gpu_graphicsdriver.h"
#include "gpu_vulkandriver.h"

namespace vulkan
{

class Driver : public gpu::VulkanDriver
{
public:
   virtual ~Driver() = default;
   virtual gpu::GraphicsDriverType type() override;

   virtual void run() override;
   virtual void stop() override;
   virtual void runUntilFlip() override;

   virtual float getAverageFPS() override;
   virtual float getAverageFrametimeMS() override;

   virtual gpu::VulkanDriver::DebuggerInfo *
   getDebuggerInfo() override;

   virtual void notifyCpuFlush(phys_addr address, uint32_t size) override;
   virtual void notifyGpuFlush(phys_addr address, uint32_t size) override;

private:
   bool mRunning = false;
   gpu::VulkanDriver::DebuggerInfo mDebuggerInfo;
};

} // namespace vulkan

#endif // DECAF_VULKAN
