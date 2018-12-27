#include "debugui_window_info.h"

#include <imgui.h>
#include <libdecaf/decaf_graphics.h>

namespace debugui
{

static const ImVec4
PausedTextColor = HEXTOIMV4(0xEF5350, 1.0f);

static const ImVec4
RunningTextColor = HEXTOIMV4(0x8BC34A, 1.0f);

InfoWindow::InfoWindow(const std::string &name) :
   Window(name)
{
   mFlags = Window::AlwaysVisible;
}

void InfoWindow::draw()
{
   auto &io = ImGui::GetIO();
   auto ImgGuiNoBorder = ImGuiWindowFlags_NoTitleBar
                       | ImGuiWindowFlags_NoResize
                       | ImGuiWindowFlags_NoMove
                       | ImGuiWindowFlags_NoScrollbar
                       | ImGuiWindowFlags_NoSavedSettings
                       | ImGuiWindowFlags_NoInputs;

   ImGui::SetNextWindowPos(ImVec2 { 8.0f, 25.0f });
   ImGui::SetNextWindowSize(ImVec2 { 180.0f, 45.0f });
   ImGui::Begin("Info", nullptr, ImgGuiNoBorder);

   auto fps = decaf::getGraphicsDriver()->getAverageFPS();
   auto ms = (fps == 0.0f) ? 0.0f : (1000.0f / fps);
   ImGui::Text("FPS: %.1f (%.3f ms)", fps, ms);

   ImGui::Text("Status: ");
   ImGui::SameLine();

   if (decaf::debug::isPaused()) {
      ImGui::TextColored(PausedTextColor, "Paused");
   } else {
      ImGui::TextColored(RunningTextColor, "Running...");
   }

   ImGui::End();
}

} // namespace debugui
