#include "debugger_ui_internal.h"
#include "libcpu/cpu.h"
#include "libcpu/espresso/espresso_instructionid.h"
#include "libcpu/espresso/espresso_instructionset.h"
#include <algorithm>
#include <chrono>
#include <cinttypes>
#include <imgui.h>
#include <vector>

namespace debugger
{

namespace ui
{

namespace StatsView
{

static bool
sActivateFocus = false;

static std::vector<ppcaddr_t>
sProfileList;

bool
gIsVisible = true;

static bool
sNeedProfileListUpdate = true;


static void
updateProfileList()
{
   using TimePair = std::pair<ppcaddr_t, uint64_t>;
   std::vector<TimePair> tempList;

   const FastRegionMap<uint64_t> *blockTime;
   cpu::getJitProfileData(&blockTime, nullptr);
   uint32_t address;
   uint64_t time;
   if (blockTime->getFirstEntry(&address, &time)) {
      do {
         tempList.emplace_back(address, time);
      } while (blockTime->getNextEntry(&address, &time));
   }

   std::sort(tempList.begin(), tempList.end(),
             [](TimePair a, TimePair b) { return a.second > b.second; });

   sProfileList.resize(std::min(tempList.size(), static_cast<size_t>(50)));
   for (auto i = 0u; i < sProfileList.size(); i++) {
      sProfileList[i] = tempList[i].first;
   }
}

void
draw()
{
   if (!gIsVisible) {
      return;
   }

   if (sActivateFocus) {
      ImGui::SetNextWindowFocus();
      sActivateFocus = false;
   }

   ImGui::SetNextWindowSize(ImVec2(700, 400), ImGuiSetCond_FirstUseEver);

   if (!ImGui::Begin("Stats", &gIsVisible)) {
      ImGui::End();
      return;
   }

   ImGui::Columns(2, "totalSize", false);

   ImGui::Text("Total JIT Code Size");
   ImGui::NextColumn();
   uint64_t jitSize = cpu::getJitCodeSize();
   ImGui::Text("%.2f MB", jitSize / 1.0e6);
   ImGui::NextColumn();

   ImGui::Columns(1);

   if (ImGui::TreeNode("JIT Profiling"))
   {
      ImGui::NextColumn();

      ImGui::Columns(6, "statsList", false);
      ImGui::SetColumnOffset(0, ImGui::GetWindowWidth() * 0.00f);
      ImGui::SetColumnOffset(1, ImGui::GetWindowWidth() * 0.18f);
      ImGui::SetColumnOffset(2, ImGui::GetWindowWidth() * 0.40f);
      ImGui::SetColumnOffset(3, ImGui::GetWindowWidth() * 0.55f);
      ImGui::SetColumnOffset(4, ImGui::GetWindowWidth() * 0.70f);
      ImGui::SetColumnOffset(5, ImGui::GetWindowWidth() * 0.85f);

      if (sNeedProfileListUpdate) {
         updateProfileList();
         sNeedProfileListUpdate = false;
      }

      ImGui::Text("Address"); ImGui::NextColumn();
      ImGui::Text("Native Code"); ImGui::NextColumn();
      ImGui::Text("Time %%"); ImGui::NextColumn();
      ImGui::Text("Total Cycles"); ImGui::NextColumn();
      ImGui::Text("Call Count"); ImGui::NextColumn();
      ImGui::Text("Cycles/Call"); ImGui::NextColumn();
      ImGui::Separator();

      const FastRegionMap<uint64_t> *blockTime;
      const FastRegionMap<uint64_t> *blockCount;
      auto totalTime = cpu::getJitProfileData(&blockTime, &blockCount);

      for (auto &address : sProfileList) {
         auto time = blockTime->find(address);
         auto count = blockCount->find(address);
         ImGui::Text("%08X", address);
         ImGui::NextColumn();
         ImGui::Text("%p", cpu::findJitEntry(address));
         ImGui::NextColumn();
         ImGui::Text("%.2f%%", 100.0 * time / totalTime);
         ImGui::NextColumn();
         ImGui::Text("%" PRIu64, time);
         ImGui::NextColumn();
         ImGui::Text("%" PRIu64, count);
         ImGui::NextColumn();
         ImGui::Text("%" PRIu64, count ? (time + count/2) / count : 0);
         ImGui::NextColumn();
      }

      ImGui::TreePop();
   }
   else
   {
      // Update the list the next time the node is expanded.
      sNeedProfileListUpdate = true;
   }

   ImGui::Columns(1);
   ImGui::End();
}

} // namespace StatsView

} // namespace ui

} // namespace debugger
