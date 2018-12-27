#ifdef DECAF_VULKAN
#include "debugui_window_performance_vulkan.h"

#include <common/decaf_assert.h>
#include <imgui.h>
#include <inttypes.h>
#include <libdecaf/decaf_graphics.h>
#include <libgpu/gpu_vulkandriver.h>

namespace debugui
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
   drawTextAndValue("Geom   Shaders:", mInfo->numGeometryShaders);
   drawTextAndValue("Pixel  Shaders:", mInfo->numPixelShaders);
   drawTextAndValue("Data Buffers:", mInfo->numDataBuffers);

   ImGui::NextColumn();

   drawTextAndValue("Render Passes:", mInfo->numRenderPasses);
   drawTextAndValue("Pipelines:", mInfo->numPipelines);
   drawTextAndValue("Samplers:", mInfo->numSamplers);
   drawTextAndValue("Surfaces:", mInfo->numSurfaces);
}

} // namespace debugui

#endif // ifdef DECAF_VULKAN
