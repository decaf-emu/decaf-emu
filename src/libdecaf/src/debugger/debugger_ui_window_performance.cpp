#include "debugger_ui_window_performance.h"
#include "debugger_ui_window_performance_gl.h"
#include "debugger_ui_plot.h"
#include "decaf_graphics.h"

#include <imgui.h>

struct GraphicsDebugInfo;

namespace debugger
{

namespace ui
{
static const float GRAPH_HEIGHT{ 70 };

constexpr std::size_t fpsSamplesSize { 100 };
constexpr std::size_t ftSamplesSize { 100 };

static float fpsValues[fpsSamplesSize] = { 0 };
static float ftValues[ftSamplesSize] = { 0 };

PerformanceWindow::PerformanceWindow(const std::string &name) :
   Window(name)
{
}

PerformanceWindow* PerformanceWindow::create(const std::string &name)
{
   auto driverType = decaf::getGraphicsDriver()->type();

   switch (driverType)
   {
   case gpu::GraphicsDriver::DRIVER_GL:
      return new PerformanceWindowGL(name);
   case gpu::GraphicsDriver::DRIVER_DX:
      // return new PerformanceWindowDX(name);
   case gpu::GraphicsDriver::DRIVER_VULKAN:
      // return new PerformanceWindowVulkan(name);
   default:
      return new PerformanceWindow(name);
   }
}

void PerformanceWindow::draw()
{
   drawGraphs();
   ImGui::End();
}

void PerformanceWindow::drawGraphs()
{
   auto ImgGuiNoResize = ImGuiWindowFlags_NoResize;

   ImGui::SetNextWindowSize(ImVec2{ 400.0f, 255.0f });
   ImGui::Begin("Performance", nullptr, ImgGuiNoResize);

   auto fps = decaf::getGraphicsDriver()->getAverageFPS();
   auto frameTime = decaf::getGraphicsDriver()->getAverageFrametime();

   auto shiftFloatArrBy1 = [](float arr[], size_t arrSize, float newVal) {
      std::memmove(arr, static_cast<void *>(arr + 1), (arrSize - 1) * sizeof(float));
      arr[arrSize - 1] = newVal;
   };

   // Frame rate
   shiftFloatArrBy1(fpsValues, fpsSamplesSize, fps);

   PlotLinesDecaf("", const_cast<const float*>(fpsValues), fpsSamplesSize, 0, NULL, 0.0f, 60.0f, ImVec2(0, GRAPH_HEIGHT), sizeof(float), 10);
   ImGui::SameLine();
   ImGui::Text("Frame Rate\n%.1f (fps)", fps);

   ImGui::Separator();

   // Frame time
   shiftFloatArrBy1(ftValues, ftSamplesSize, frameTime);

   PlotLinesDecaf("", const_cast<const float*>(ftValues), ftSamplesSize, 0, NULL, 0.0f, 100.0f, ImVec2(0, GRAPH_HEIGHT), sizeof(float), 10);
   ImGui::SameLine();
   ImGui::Text("Frame Time\n%.1f (ms)", frameTime);
}

} // namespace ui

} // namespace debugger
