#ifdef DECAF_GL
#include "debugger_ui_window_performance_gl.h"
#include "decaf_graphics.h"

#include <common/decaf_assert.h>
#include <imgui.h>
#include <libgpu/gpu_opengldriver.h>

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
   auto graphicsDriver = decaf::getGraphicsDriver();
   decaf_check(graphicsDriver->type() == gpu::GraphicsDriverType::OpenGL);

   auto glGraphicsDriver = reinterpret_cast<gpu::OpenGLDriver *>(graphicsDriver);
   mInfo = glGraphicsDriver->getGraphicsDebuggerInfo();
}

void PerformanceWindowGL::draw()
{
   // Draw framerate/frametime graphs
   PerformanceWindow::drawGraphs();

   auto drawTextAndValue =
      [](const char *text, uint64_t val)
      {
         ImGui::PushStyleColor(ImGuiCol_Text, TitleTextColor);
         ImGui::Text(text);
         ImGui::PushStyleColor(ImGuiCol_Text, ValueTextColor);
         ImGui::SameLine();
         ImGui::Text("%ull", val);
         ImGui::PopStyleColor();
      };

   if (!mInfo) {
      return;
   }

   // Graphics info
   ImGui::Separator();
   ImGui::Text("\tGraphics Debugging Info:\n");

   ImGui::Columns(2);

   drawTextAndValue("Vertex Shaders:", mInfo->numVertexShaders);
   drawTextAndValue("Pixel  Shaders:", mInfo->numPixelShaders);
   drawTextAndValue("Fetch  Shaders:", mInfo->numFetchShaders);

   ImGui::NextColumn();

   drawTextAndValue("Shader Pipelines:", mInfo->numShaderPipelines);
   drawTextAndValue("Surfaces:", mInfo->numSurfaces);
   drawTextAndValue("Data Buffers:", mInfo->numDataBuffers);

   ImGui::End();
}

} // namespace ui

} // namespace debugger

#endif // ifdef DECAF_GL
