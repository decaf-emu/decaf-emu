#include "debugui_window_registers.h"

#include <fmt/format.h>
#include <imgui.h>

namespace debugui
{

static const ImVec4
ChangedColor = HEXTOIMV4(0xF44336, 1.0f);

RegistersWindow::RegistersWindow(const std::string &name) :
   Window(name)
{
}

void
RegistersWindow::draw()
{
   ImGui::SetNextWindowSize(ImVec2 { 300, 400 }, ImGuiSetCond_FirstUseEver);

   if (!ImGui::Begin(mName.c_str(), &mVisible)) {
      ImGui::End();
      return;
   }

   auto thread = mStateTracker->getActiveThread();
   if (!thread.handle) {
      thread.gpr.fill(0u);
      thread.lr = 0;
      thread.ctr = 0;
      thread.cr = 0;
   }

   ImGui::Columns(4, "regsList", false);
   ImGui::SetColumnOffset(0, ImGui::GetWindowWidth() * 0.00f);
   ImGui::SetColumnOffset(1, ImGui::GetWindowWidth() * 0.15f);
   ImGui::SetColumnOffset(2, ImGui::GetWindowWidth() * 0.50f);
   ImGui::SetColumnOffset(3, ImGui::GetWindowWidth() * 0.65f);

   auto drawRegCol =
      [this](const std::string &name, const std::string &value, bool hasChanged) {
         ImGui::Text("%s", name.c_str());

         ImGui::NextColumn();
         if (decaf::debug::isPaused()) {
            if (!hasChanged) {
               ImGui::Text("%s", value.c_str());
            } else {
               ImGui::TextColored(ChangedColor, "%s", value.c_str());
            }
         }

         ImGui::NextColumn();
      };

   for (auto i = 0, j = 16; i < 16; i++, j++) {
      drawRegCol(fmt::format("r{}", i),
                 fmt::format("{:08x}", thread.gpr[i]),
                 thread.gpr[i] != mPreviousThreadState.gpr[i]);

      drawRegCol(fmt::format("r{}", j),
                 fmt::format("{:08x}", thread.gpr[j]),
                 thread.gpr[j] != mPreviousThreadState.gpr[j]);
   }

   ImGui::Separator();

   drawRegCol("LR",
              fmt::format("{:08x}", thread.lr),
              thread.lr != mPreviousThreadState.lr);

   drawRegCol("CTR",
              fmt::format("{:08x}", thread.ctr),
              thread.ctr != mPreviousThreadState.ctr);

   ImGui::Separator();

   ImGui::NextColumn();
   ImGui::Text("O Z + -"); ImGui::NextColumn();
   ImGui::NextColumn();
   ImGui::Text("O Z + -"); ImGui::NextColumn();

   auto drawCrfCol =
      [this](uint32_t crfNum, uint32_t val, bool hasChanged) {
         ImGui::Text("crf%d", crfNum);
         ImGui::NextColumn();

         if (decaf::debug::isPaused()) {
            if (!hasChanged) {
               ImGui::Text("%c %c %c %c",
                  (val & espresso::ConditionRegisterFlag::SummaryOverflow) ? 'X' : '_',
                           (val & espresso::ConditionRegisterFlag::Zero) ? 'X' : '_',
                           (val & espresso::ConditionRegisterFlag::Positive) ? 'X' : '_',
                           (val & espresso::ConditionRegisterFlag::Negative) ? 'X' : '_');
            } else {
               ImGui::TextColored(ChangedColor, "%c %c %c %c",
                  (val & espresso::ConditionRegisterFlag::SummaryOverflow) ? 'X' : '_',
                                  (val & espresso::ConditionRegisterFlag::Zero) ? 'X' : '_',
                                  (val & espresso::ConditionRegisterFlag::Positive) ? 'X' : '_',
                                  (val & espresso::ConditionRegisterFlag::Negative) ? 'X' : '_');
            }
         }

         ImGui::NextColumn();
      };

   for (auto i = 0, j = 4; i < 4; i++, j++) {
      auto iVal = (thread.cr >> ((7 - i) * 4)) & 0xF;
      auto jVal = (thread.cr >> ((7 - j) * 4)) & 0xF;
      auto iPrevVal = (mPreviousThreadState.cr >> ((7 - i) * 4)) & 0xF;
      auto jPrevVal = (mPreviousThreadState.cr >> ((7 - j) * 4)) & 0xF;

      drawCrfCol(i, iVal, iVal != iPrevVal);
      drawCrfCol(j, jVal, jVal != jPrevVal);
   }

   ImGui::Columns(1);
   ImGui::End();

   mPreviousThreadState = thread;
}

} // namespace debugui
