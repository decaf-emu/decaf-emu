#include "debugger_ui_window_segments.h"

#include "cafe/loader/cafe_loader_entry.h"
#include "cafe/loader/cafe_loader_loaded_rpl.h"

#include <fmt/format.h>
#include <imgui.h>

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

   cafe::loader::lockLoader();
   for (auto rpl = cafe::loader::getLoadedRplLinkedList(); rpl; rpl = rpl->nextLoadedRpl) {
      if (!rpl->sectionAddressBuffer ||
          !rpl->sectionAddressBuffer ||
          !rpl->moduleNameBuffer ||
          !rpl->moduleNameLen ||
          !rpl->sectionAddressBuffer[rpl->elfHeader.shstrndx]) {
         continue;
      }

      auto rplName = std::string_view {
            rpl->moduleNameBuffer.getRawPointer(),
            rpl->moduleNameLen
         };

      auto shStrTab =
         virt_cast<const char *>(rpl->sectionAddressBuffer[rpl->elfHeader.shstrndx])
         .getRawPointer();

      for (auto i = 0u; i < rpl->elfHeader.shnum; ++i) {
         auto sectionHeader =
            virt_cast<cafe::loader::rpl::SectionHeader *>(
               virt_cast<virt_addr>(rpl->sectionHeaderBuffer) +
               (i * rpl->elfHeader.shentsize));

         if (rpl->sectionAddressBuffer[i] &&
             sectionHeader->size != 0 &&
             (sectionHeader->flags & cafe::loader::rpl::SHF_ALLOC)) {
            addSegmentItem(mSegmentsCache, Segment {
                  fmt::format("{}:{}", rplName, shStrTab + sectionHeader->name),
                  static_cast<uint32_t>(rpl->sectionAddressBuffer[i]),
                  sectionHeader->size
               });
         }
      }
   }
   cafe::loader::unlockLoader();

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
