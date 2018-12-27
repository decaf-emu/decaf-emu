#include "debugui_window_segments.h"

#include <fmt/format.h>
#include <imgui.h>
#include <libdecaf/decaf_debug_api.h>

namespace debugui
{

SegmentsWindow::SegmentsWindow(const std::string &name) :
   Window(name)
{
}

void
SegmentsWindow::draw()
{
   ImGui::SetNextWindowSize(ImVec2 { 550.0f, 300.0f }, ImGuiSetCond_FirstUseEver);

   if (!ImGui::Begin("Memory Segments", &mVisible)) {
      ImGui::End();
      return;
   }

   mSegmentsCache.clear();
   decaf::debug::sampleCafeMemorySegments(mSegmentsCache);

   ImGui::Columns(3, "memList", false);
   ImGui::SetColumnOffset(0, ImGui::GetWindowWidth() * 0.0f);
   ImGui::SetColumnOffset(1, ImGui::GetWindowWidth() * 0.7f);
   ImGui::SetColumnOffset(2, ImGui::GetWindowWidth() * 0.85f);

   ImGui::Text("Name"); ImGui::NextColumn();
   ImGui::Text("Start"); ImGui::NextColumn();
   ImGui::Text("End"); ImGui::NextColumn();
   ImGui::Separator();

   for (auto &segment : mSegmentsCache) {
      ImGui::Selectable(fmt::format("{}", segment.name).c_str()); ImGui::NextColumn();

      if (ImGui::BeginPopupContextItem(fmt::format("{}-actions", segment.name).c_str(), 1)) {
         if (ImGui::MenuItem("Go to in Debugger")) {
            mStateTracker->gotoDisassemblyAddress(segment.address);
         }

         if (ImGui::MenuItem("Go to in Memory View")) {
            mStateTracker->gotoMemoryAddress(segment.address);
         }

         ImGui::EndPopup();
      }

      ImGui::Text("%08x", segment.address); ImGui::NextColumn();
      ImGui::Text("%08x", segment.address + segment.size); ImGui::NextColumn();
   }

   ImGui::Columns(1);
   ImGui::End();
}

} // namespace debugui
