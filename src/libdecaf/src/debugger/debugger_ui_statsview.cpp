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

static const size_t
InstrCount = static_cast<size_t>(espresso::InstructionID::InstructionCount);

bool
gIsVisible = true;

static bool
sActivateFocus = false;

static std::vector<std::pair<size_t, uint64_t>>
sJitFallbackStats;

static std::chrono::time_point<std::chrono::system_clock>
sFirstSeen = std::chrono::time_point<std::chrono::system_clock>::max();

static uint64_t
sFirstSeenValues[InstrCount] = { 0 };

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

   ImGui::SetNextWindowSize(ImVec2(600, 300), ImGuiSetCond_FirstUseEver);

   if (!ImGui::Begin("Stats", &gIsVisible)) {
      ImGui::End();
      return;
   }

   ImGui::Columns(3, "statsList", false);
   ImGui::SetColumnOffset(0, ImGui::GetWindowWidth() * 0.00f);
   ImGui::SetColumnOffset(1, ImGui::GetWindowWidth() * 0.60f);
   ImGui::SetColumnOffset(2, ImGui::GetWindowWidth() * 0.80f);

   ImGui::Text("Name"); ImGui::NextColumn();
   ImGui::Text("Value"); ImGui::NextColumn();
   ImGui::Text("IPS"); ImGui::NextColumn();
   ImGui::Separator();

   if (ImGui::TreeNode("JIT Fallback"))
   {
      ImGui::NextColumn();
      ImGui::NextColumn();
      ImGui::NextColumn();

      uint64_t *fallbackStats = cpu::getJitFallbackStats();

      if (sFirstSeen == std::chrono::time_point<std::chrono::system_clock>::max()) {
         sFirstSeen = std::chrono::system_clock::now();

         for (size_t i = 0; i < InstrCount; ++i) {
            sFirstSeenValues[i] = fallbackStats[i];
         }
      }

      sJitFallbackStats.clear();

      for (size_t i = 0; i < InstrCount; ++i) {
         sJitFallbackStats.emplace_back(i, fallbackStats[i]);
      }

      std::sort(sJitFallbackStats.begin(), sJitFallbackStats.end(),
         [](const std::pair<size_t, uint64_t> &a, const std::pair<size_t, uint64_t> &b) {
            return b.second < a.second;
         });

      using seconds_duration = std::chrono::duration<float, std::chrono::seconds::period>;
      auto timeDelta = std::chrono::system_clock::now() - sFirstSeen;
      auto secondsDelta = std::chrono::duration_cast<seconds_duration>(timeDelta).count();

      for (auto i = 0u; i < sJitFallbackStats.size(); ++i) {
         auto instrIdx = sJitFallbackStats[i].first;
         auto instrId = static_cast<espresso::InstructionID>(instrIdx);
         auto info = espresso::findInstructionInfo(instrId);

         if (!info) {
            continue;
         }

         auto instrOps = sJitFallbackStats[i].second;
         auto ips = static_cast<float>(instrOps - sFirstSeenValues[instrIdx]) / static_cast<float>(secondsDelta);

         ImGui::Text("%s", info->name.c_str());
         ImGui::NextColumn();
         ImGui::Text("%" PRIu64, instrOps);
         ImGui::NextColumn();
         ImGui::Text("%.0f", ips);
         ImGui::NextColumn();
      }

      ImGui::TreePop();
   }

   ImGui::Columns(1);
   ImGui::End();
}

} // namespace StatsView

} // namespace ui

} // namespace debugger
