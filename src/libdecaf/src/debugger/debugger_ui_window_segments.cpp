#include "debugger_ui_window_segments.h"
#include "kernel/kernel_loader.h"

#include <imgui.h>
#include <spdlog/fmt/fmt.h>

namespace debugger
{

namespace ui
{

SegmentsWindow::SegmentsWindow(const std::string &name) :
   Window(name)
{
}

static void
addSegmentItem(std::vector<Segment> &segments,
               const Segment &item)
{
   for (auto &seg : segments) {
      if (item.start >= seg.start && item.start < seg.end) {
         addSegmentItem(seg.items, item);
         return;
      }
   }

   segments.push_back(item);
}

void
SegmentsWindow::drawSegments(const std::vector<Segment> &segments,
                             std::string tabs)
{
   for (auto &seg : segments) {
      ImGui::Selectable(fmt::format("{}{}", tabs, seg.name).c_str()); ImGui::NextColumn();

      if (ImGui::BeginPopupContextItem(fmt::format("{}{}-actions", tabs, seg.name).c_str(), 1)) {
         if (ImGui::MenuItem("Go to in Debugger")) {
            mStateTracker->gotoDisassemblyAddress(seg.start);
         }

         if (ImGui::MenuItem("Go to in Memory View")) {
            mStateTracker->gotoMemoryAddress(seg.start);
         }

         ImGui::EndPopup();
      }

      ImGui::Text("%08x", seg.start); ImGui::NextColumn();
      ImGui::Text("%08x", seg.end); ImGui::NextColumn();
      drawSegments(seg.items, tabs + "  ");
   }
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
   mSegmentsCache.push_back(Segment { "System", mem::SystemBase, mem::SystemEnd });
   mSegmentsCache.push_back(Segment { "MEM2", mem::MEM2Base, mem::MEM2End });
   mSegmentsCache.push_back(Segment { "OverlayArena", mem::OverlayArenaBase, mem::OverlayArenaEnd });
   mSegmentsCache.push_back(Segment { "Apertures", mem::AperturesBase, mem::AperturesEnd });
   mSegmentsCache.push_back(Segment { "Foreground", mem::ForegroundBase, mem::ForegroundEnd });
   mSegmentsCache.push_back(Segment { "MEM1", mem::MEM1Base, mem::MEM1End });
   mSegmentsCache.push_back(Segment { "LockedCache", mem::LockedCacheBase, mem::LockedCacheEnd });
   mSegmentsCache.push_back(Segment { "SharedData", mem::SharedDataBase, mem::SharedDataEnd });

   kernel::loader::lockLoader();
   const auto &modules = kernel::loader::getLoadedModules();

   for (auto &mod : modules) {
      for (auto &sec : mod.second->sections) {
         addSegmentItem(mSegmentsCache, Segment {
            fmt::format("{}:{}", mod.second->name, sec.name),
            sec.start,
            sec.end
         });
      }
   }

   kernel::loader::unlockLoader();

   ImGui::Columns(3, "memList", false);
   ImGui::SetColumnOffset(0, ImGui::GetWindowWidth() * 0.0f);
   ImGui::SetColumnOffset(1, ImGui::GetWindowWidth() * 0.7f);
   ImGui::SetColumnOffset(2, ImGui::GetWindowWidth() * 0.85f);

   ImGui::Text("Name"); ImGui::NextColumn();
   ImGui::Text("Start"); ImGui::NextColumn();
   ImGui::Text("End"); ImGui::NextColumn();
   ImGui::Separator();
   drawSegments(mSegmentsCache, "");

   ImGui::Columns(1);
   ImGui::End();
}

} // namespace ui

} // namespace debugger
