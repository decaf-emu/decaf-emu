#include "debugger_ui_internal.h"
#include "libcpu/jit_stats.h"
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

static std::vector<cpu::jit::CodeBlock *>
sProfileList;

bool
gIsVisible = true;

static bool
sNeedProfileListUpdate = true;


static void
updateProfileList()
{
   using TimePair = std::pair<cpu::jit::CodeBlock *, uint64_t>;
   auto tempList = std::vector<TimePair> { };
   auto stats = cpu::jit::JitStats { };

   if (!cpu::jit::sampleStats(stats)) {
      sProfileList.clear();
      return;
   }

   for (auto &block : stats.compiledBlocks) {
      tempList.emplace_back(&block, block.profileData.time.load());
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

   auto stats = cpu::jit::JitStats {};
   auto sampled = cpu::jit::sampleStats(stats);

   ImGui::Columns(2, "totalSize", false);

   ImGui::Text("Total JIT Code Size");
   ImGui::NextColumn();
   ImGui::Text("%.2f MB", stats.usedCodeCacheSize / 1.0e6);
   ImGui::NextColumn();

   ImGui::Text("Total JIT Data Size");
   ImGui::NextColumn();
   ImGui::Text("%.2f MB", stats.usedDataCacheSize / 1.0e6);
   ImGui::NextColumn();

   ImGui::Columns(1);

   if (ImGui::TreeNode("JIT Profiling") && sampled) {
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

      auto totalTime = stats.totalTimeInCodeBlocks;

      for (auto &block : sProfileList) {
         auto time = block->profileData.time.load();
         auto count = block->profileData.count.load();
         ImGui::Text("%08X", block->address);
         ImGui::NextColumn();
         ImGui::Text("%p", block->code);
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
   } else {
      // Update the list the next time the node is expanded.
      sNeedProfileListUpdate = true;
   }

   ImGui::Columns(1);
   ImGui::End();
}

} // namespace StatsView

} // namespace ui

} // namespace debugger
