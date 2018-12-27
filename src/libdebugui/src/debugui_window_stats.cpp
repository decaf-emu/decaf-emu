#include "debugui_window_stats.h"

#include <algorithm>
#include <cinttypes>
#include <imgui.h>
#include <libcpu/jit_stats.h>

namespace debugui
{

StatsWindow::StatsWindow(const std::string &name) :
   Window(name)
{
}

void
StatsWindow::update()
{
   using TimePair = std::pair<cpu::jit::CodeBlock *, uint64_t>;
   auto tempList = std::vector<TimePair> {};
   auto stats = cpu::jit::JitStats {};

   mLastProfileListUpdate = std::chrono::system_clock::now();

   if (!cpu::jit::sampleStats(stats)) {
      mProfileList.clear();
      return;
   }

   for (auto &block : stats.compiledBlocks) {
      tempList.emplace_back(&block, block.profileData.time.load());
   }

   std::sort(tempList.begin(), tempList.end(),
             [](TimePair a, TimePair b) { return a.second > b.second; });

   mProfileList.resize(std::min(tempList.size(), static_cast<size_t>(50)));

   for (auto i = 0u; i < mProfileList.size(); i++) {
      mProfileList[i] = tempList[i].first;
   }
}

void
StatsWindow::draw()
{
   ImGui::SetNextWindowSize(ImVec2 { 700, 400 }, ImGuiSetCond_FirstUseEver);

   if (!ImGui::Begin(mName.c_str(), &mVisible)) {
      ImGui::End();
      return;
   }

   // Update JIT block list every 5 seconds
   auto dt = std::chrono::system_clock::now() - mLastProfileListUpdate;

   if (std::chrono::duration_cast<std::chrono::seconds>(dt).count() > 5) {
      mNeedProfileListUpdate = true;
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

   if (sampled) {
      if (ImGui::TreeNode("JIT Profiling")) {
         ImGui::NextColumn();

         ImGui::Columns(6, "statsList", false);
         ImGui::SetColumnOffset(0, ImGui::GetWindowWidth() * 0.00f);
         ImGui::SetColumnOffset(1, ImGui::GetWindowWidth() * 0.18f);
         ImGui::SetColumnOffset(2, ImGui::GetWindowWidth() * 0.40f);
         ImGui::SetColumnOffset(3, ImGui::GetWindowWidth() * 0.55f);
         ImGui::SetColumnOffset(4, ImGui::GetWindowWidth() * 0.70f);
         ImGui::SetColumnOffset(5, ImGui::GetWindowWidth() * 0.85f);

         if (mNeedProfileListUpdate) {
            update();
            mNeedProfileListUpdate = false;
         }

         ImGui::Text("Address"); ImGui::NextColumn();
         ImGui::Text("Native Code"); ImGui::NextColumn();
         ImGui::Text("Time %%"); ImGui::NextColumn();
         ImGui::Text("Total Cycles"); ImGui::NextColumn();
         ImGui::Text("Call Count"); ImGui::NextColumn();
         ImGui::Text("Cycles/Call"); ImGui::NextColumn();
         ImGui::Separator();

         auto totalTime = stats.totalTimeInCodeBlocks;

         for (auto &block : mProfileList) {
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
            ImGui::Text("%" PRIu64, count ? (time + count / 2) / count : 0);
            ImGui::NextColumn();
         }

         ImGui::TreePop();
      } else {
         // Update the list the next time the node is expanded.
         mNeedProfileListUpdate = true;
      }
   }

   ImGui::Columns(1);
   ImGui::End();
}

} // namespace debugui
