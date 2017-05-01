#include "debugger_ui_window_performance_gl.h"
#include "decaf_graphics.h"

#include <imgui.h>
#include "decaf_graphics_info.h"

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

   auto* debugInfo = decaf::getGraphicsDriver()->getGraphicsDebugInfo();

   if (!debugInfo)
      return;

   auto& glDebugInfo = *static_cast<decaf::GraphicsDebugInfoGL*>(debugInfo);

   // Graphics info
   ImGui::Separator();
   ImGui::Text("\tGraphics Debugging Info:\n");

   ImGui::Columns(2);

   drawTextAndValue("Vertex Shaders:", glDebugInfo.vertexShaders);
   drawTextAndValue("Pixel  Shaders:", glDebugInfo.pixelShaders);
   drawTextAndValue("Fetch  Shaders:", glDebugInfo.fetchShaders);

   ImGui::NextColumn();

   drawTextAndValue("Shader Pipelines:", glDebugInfo.shaderPipelines);
   drawTextAndValue("Surfaces:", glDebugInfo.surfaces);
   drawTextAndValue("Data Buffers:", glDebugInfo.dataBuffers);

   ImGui::End();
   delete &glDebugInfo;
}

} // namespace ui

} // namespace debugger
