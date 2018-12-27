#pragma once
#ifdef DECAF_VULKAN
#include "debugui_window.h"
#include "debugui_window_performance.h"
#include <libgpu/gpu_vulkandriver.h>

namespace debugui
{

class PerformanceWindowVulkan : public PerformanceWindow
{
public:
   PerformanceWindowVulkan(const std::string &name);
   virtual ~PerformanceWindowVulkan() = default;

   void drawBackendInfo() override;

private:
   gpu::VulkanDriver::DebuggerInfo *mInfo;
};

} // namespace debugui

#endif // ifdef DECAF_VULKAN