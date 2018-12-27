#include "debugui_window_performance.h"
#include "debugui_window_performance_gl.h"
#include "debugui_window_performance_vulkan.h"
#include "debugui_plot.h"

#include <algorithm>
#include <imgui.h>
#include <libdecaf/decaf_graphics.h>
#include <inttypes.h>

struct GraphicsDebugInfo;

namespace debugui
{

static const ImVec4
TitleTextColor = HEXTOIMV4(0xFFFFFF, 1.0f);

static const ImVec4
ValueTextColor = HEXTOIMV4(0xD3D3D3, 1.0f);

PerformanceWindow::PerformanceWindow(const std::string &name) :
   Window(name)
{
}

PerformanceWindow *
PerformanceWindow::create(const std::string &name)
{
   auto driverType = decaf::getGraphicsDriver()->type();

   switch (driverType)
   {
#ifdef DECAF_GL
   case gpu::GraphicsDriverType::OpenGL:
      return new PerformanceWindowGL(name);
#endif
#ifdef DECAF_VULKAN
   case gpu::GraphicsDriverType::Vulkan:
      return new PerformanceWindowVulkan(name);
#endif
   }

   return new PerformanceWindow(name);
}

void
PerformanceWindow::drawTextAndValue(const char *text, uint64_t val)
{
   ImGui::PushStyleColor(ImGuiCol_Text, TitleTextColor);
   ImGui::Text("%s", text);
   ImGui::PopStyleColor();

   ImGui::PushStyleColor(ImGuiCol_Text, ValueTextColor);
   ImGui::SameLine();
   ImGui::Text("%" PRIu64, val);
   ImGui::PopStyleColor();
}

void
PerformanceWindow::draw()
{
   auto ImgGuiNoResize = ImGuiWindowFlags_NoResize;

   ImGui::SetNextWindowSize(ImVec2 { 400.0f, 0.0f });
   ImGui::Begin("Performance", nullptr, ImgGuiNoResize);

   drawGraphs();

   ImGui::Separator();
   ImGui::Text("Graphics Debugging Info:");
   ImGui::Separator();

   drawBackendInfo();

   ImGui::End();
}

void
PerformanceWindow::drawGraphs()
{
   auto fps = decaf::getGraphicsDriver()->getAverageFPS();
   auto frameTime = decaf::getGraphicsDriver()->getAverageFrametimeMS();

   // Frame rate
   std::copy(mFpsValues.begin() + 1, mFpsValues.end(), mFpsValues.begin());
   mFpsValues.back() = fps;

   PlotLinesDecaf("",
                  mFpsValues.data(),
                  static_cast<int>(mFpsValues.size()),
                  0,
                  NULL,
                  0.0f, 60.0f,
                  ImVec2 { 0, GraphHeight },
                  sizeof(float),
                  10);
   ImGui::SameLine();
   ImGui::Text("Frame Rate\n%.1f (fps)", fps);

   ImGui::Separator();

   // Frame time
   std::copy(mFtValues.begin() + 1, mFtValues.end(), mFtValues.begin());
   mFtValues.back() = frameTime;

   PlotLinesDecaf("",
                  mFtValues.data(),
                  static_cast<int>(mFtValues.size()),
                  0,
                  NULL,
                  0.0f, 100.0f,
                  ImVec2 { 0, GraphHeight },
                  sizeof(float),
                  10);
   ImGui::SameLine();
   ImGui::Text("Frame Time\n%.1f (ms)", frameTime);
}

void
PerformanceWindow::drawBackendInfo()
{
   ImGui::Text("Unsupported graphics backend in use.");
}

} // namespace debugui
