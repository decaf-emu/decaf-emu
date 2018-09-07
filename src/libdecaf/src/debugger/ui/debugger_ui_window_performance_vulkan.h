#pragma once
#ifdef DECAF_VULKAN
#include "debugger_ui_window.h"
#include "debugger_ui_window_performance.h"
#include <libgpu/gpu_vulkandriver.h>

namespace debugger
{

namespace ui
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

} // namespace ui

} // namespace debugger

#endif // ifdef DECAF_VULKAN