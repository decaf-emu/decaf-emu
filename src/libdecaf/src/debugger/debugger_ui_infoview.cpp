#include "debugger_ui_internal.h"
#include "decaf_graphics.h"
#include <imgui.h>

namespace debugger
{

namespace ui
{

namespace InfoView
{

static const ImVec4 PausedTextColor = HEXTOIMV4(0xEF5350, 1.0f);
static const ImVec4 RunningTextColor = HEXTOIMV4(0x8BC34A, 1.0f);

void
draw()
{
   auto &io = ImGui::GetIO();
   auto ImgGuiNoBorder =
      ImGuiWindowFlags_NoTitleBar
      | ImGuiWindowFlags_NoResize
      | ImGuiWindowFlags_NoMove
      | ImGuiWindowFlags_NoScrollbar
      | ImGuiWindowFlags_NoSavedSettings
      | ImGuiWindowFlags_NoInputs;

   ImGui::SetNextWindowPos(ImVec2(8.0f, 25.0f));
   ImGui::SetNextWindowSize(ImVec2(180.0f, 45.0f));
   ImGui::Begin("Info", nullptr, ImgGuiNoBorder);

   float fps = decaf::getGraphicsDriver()->getAverageFPS();
   ImGui::Text("FPS: %.1f (%.3f ms)", fps, 1000.0f / fps);

   ImGui::Text("Status: ");
   ImGui::SameLine();
   if (debugger::ui::isPaused()) {
      ImGui::TextColored(PausedTextColor, "Paused");
   } else {
      ImGui::TextColored(RunningTextColor, "Running...");
   }

   ImGui::End();
}

} // namespace InfoView

} // namespace ui

} // namespace debugger
