#include "debugger_ui_window_performance.h"
#include "debugger_ui_window_performance_gl.h"
#include "debugger_ui_plot.h"
#include "decaf_graphics.h"

#include <algorithm>
#include <imgui.h>

struct GraphicsDebugInfo;

namespace debugger
{

namespace ui
{

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
   }

   return new PerformanceWindow(name);
}

void
PerformanceWindow::draw()
{
   drawGraphs();
   ImGui::End();
}

void
PerformanceWindow::drawGraphs()
{
   auto ImgGuiNoResize = ImGuiWindowFlags_NoResize;

   ImGui::SetNextWindowSize(ImVec2{ 400.0f, 255.0f });
   ImGui::Begin("Performance", nullptr, ImgGuiNoResize);

   auto fps = decaf::getGraphicsDriver()->getAverageFPS();
   auto frameTime = decaf::getGraphicsDriver()->getAverageFrametimeMS();

   // Frame rate
   std::copy(mFpsValues.begin() + 1, mFpsValues.end(), mFpsValues.begin());
   mFpsValues.back() = fps;

   PlotLinesDecaf("", mFpsValues.data(), mFpsValues.size(), 0, NULL, 0.0f, 60.0f, ImVec2 { 0, GraphHeight }, sizeof(float), 10);
   ImGui::SameLine();
   ImGui::Text("Frame Rate\n%.1f (fps)", fps);

   ImGui::Separator();

   // Frame time
   std::copy(mFtValues.begin() + 1, mFtValues.end(), mFtValues.begin());
   mFtValues.back() = frameTime;

   PlotLinesDecaf("", mFtValues.data(), mFtValues.size(), 0, NULL, 0.0f, 100.0f, ImVec2 { 0, GraphHeight }, sizeof(float), 10);
   ImGui::SameLine();
   ImGui::Text("Frame Time\n%.1f (ms)", frameTime);
}

} // namespace ui

} // namespace debugger
