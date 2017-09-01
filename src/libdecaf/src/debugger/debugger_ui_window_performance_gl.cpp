#include "debugger_ui_window_performance_gl.h"
#include "decaf_graphics.h"
#include "decaf_graphics_info.h"
#include "libgpu/gpu_opengldriver.h"

#include <imgui.h>

namespace debugger
{

namespace ui
{

static const ImVec4
   TitleTextColor = HEXTOIMV4(0xFFFFFF, 1.0f);
static const ImVec4
   ValueTextColor = HEXTOIMV4(0xD3D3D3, 1.0f);

PerformanceWindowGL::PerformanceWindowGL(const std::string &name) :
   PerformanceWindow(name)
{
   auto debugInfo = reinterpret_cast<gpu::OpenGLDriver*>(decaf::getGraphicsDriver());
   mDebugInfo.reset(debugInfo->getGraphicsDebugInfoPtr());
}

void PerformanceWindowGL::draw()
{
   // Draw framerate/frametime graphs
   PerformanceWindow::drawGraphs();

   auto drawTextAndValue = [](const char *text, uint64_t val) {
      ImGui::PushStyleColor(ImGuiCol_Text, TitleTextColor);
      ImGui::Text(text);
      ImGui::PushStyleColor(ImGuiCol_Text, ValueTextColor);
      ImGui::SameLine();
      ImGui::Text("%d", val);
      ImGui::PopStyleColor();
   };

   if (!mDebugInfo)
      return;

   // Graphics info
   ImGui::Separator();
   ImGui::Text("\tGraphics Debugging Info:\n");

   ImGui::Columns(2);

   drawTextAndValue("Vertex Shaders:", mDebugInfo->vertexShaders);
   drawTextAndValue("Pixel  Shaders:", mDebugInfo->pixelShaders);
   drawTextAndValue("Fetch  Shaders:", mDebugInfo->fetchShaders);

   ImGui::NextColumn();

   drawTextAndValue("Shader Pipelines:", mDebugInfo->shaderPipelines);
   drawTextAndValue("Surfaces:", mDebugInfo->surfaces);
   drawTextAndValue("Data Buffers:", mDebugInfo->dataBuffers);

   ImGui::End();
}

} // namespace ui

} // namespace debugger
