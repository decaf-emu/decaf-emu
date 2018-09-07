#ifdef DECAF_VULKAN
#include "debugger_ui_window_performance_vulkan.h"
#include "decaf_graphics.h"

#include <common/decaf_assert.h>
#include <imgui.h>
#include <inttypes.h>
#include <libgpu/gpu_vulkandriver.h>

namespace debugger
{

namespace ui
{

PerformanceWindowVulkan::PerformanceWindowVulkan(const std::string &name) :
   PerformanceWindow(name)
{
   auto graphicsDriver = decaf::getGraphicsDriver();
   decaf_check(graphicsDriver->type() == gpu::GraphicsDriverType::Vulkan);

   auto glGraphicsDriver = reinterpret_cast<gpu::VulkanDriver *>(graphicsDriver);
   mInfo = glGraphicsDriver->getDebuggerInfo();
}

void PerformanceWindowVulkan::drawBackendInfo()
{
   if (!mInfo) {
      return;
   }

   ImGui::Columns(2);

   drawTextAndValue("Vertex Shaders:", mInfo->numVertexShaders);
   drawTextAndValue("Pixel  Shaders:", mInfo->numPixelShaders);
   drawTextAndValue("Fetch  Shaders:", mInfo->numFetchShaders);

   ImGui::NextColumn();

   drawTextAndValue("Shader Pipelines:", mInfo->numShaderPipelines);
   drawTextAndValue("Surfaces:", mInfo->numSurfaces);
   drawTextAndValue("Data Buffers:", mInfo->numDataBuffers);
}

} // namespace ui

} // namespace debugger

#endif // ifdef DECAF_VULKAN
