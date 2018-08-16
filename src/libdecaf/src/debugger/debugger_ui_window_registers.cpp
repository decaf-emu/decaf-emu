#include "debugger_ui_window_registers.h"
#include "debugger_threadutils.h"
#include "cafe/libraries/coreinit/coreinit_scheduler.h"

#include <fmt/format.h>
#include <imgui.h>

namespace debugger
{

namespace ui
{

static const ImVec4
ChangedColor = HEXTOIMV4(0xF44336, 1.0f);

RegistersWindow::RegistersWindow(const std::string &name) :
   Window(name),
   mLastResumeCount(0),
   mLastActiveThread(0)
{
   std::memset(&mCurrentRegisters, 0, sizeof(cpu::CoreRegs));
   std::memset(&mPreviousRegisters, 0, sizeof(cpu::CoreRegs));
}

void
RegistersWindow::update()
{
   auto activeThread = mStateTracker->getActiveThread();
   auto resumeCount = mStateTracker->getResumeCount();

   if (!activeThread) {
      return;
   }

   // Check if we need to update mPreviousRegisters
   if (mLastResumeCount != resumeCount || mLastActiveThread != activeThread) {
      mPreviousRegisters = mCurrentRegisters;
      mLastResumeCount = resumeCount;
      mLastActiveThread = activeThread;
   }

   if (auto pauseContext = getThreadCoreContext(mDebugger, activeThread)) {
      // If the thread is running on a core, read the core registers.
      std::memcpy(&mCurrentRegisters, pauseContext, sizeof(cpu::CoreRegs));
   } else {
      // If the thread is not running on a core, read from the thread's context.
      auto context = virt_addrof(activeThread->context);
      auto state = &mCurrentRegisters;
      std::memset(&mCurrentRegisters, 0xF1, sizeof(cpu::CoreRegs));

      for (auto i = 0u; i < 32; ++i) {
         state->gpr[i] = context->gpr[i];
      }

      for (auto i = 0u; i < 32; ++i) {
         state->fpr[i].value = context->fpr[i];
         state->fpr[i].paired1 = context->psf[i];
      }

      for (auto i = 0u; i < 8; ++i) {
         state->gqr[i].value = context->gqr[i];
      }

      state->cr.value = context->cr;
      state->lr = context->lr;
      state->ctr = context->ctr;
      state->xer.value = context->xer;
      state->fpscr.value = context->fpscr;
   }
}

void
RegistersWindow::draw()
{
   ImGui::SetNextWindowSize(ImVec2 { 300, 400 }, ImGuiSetCond_FirstUseEver);

   if (!ImGui::Begin(mName.c_str(), &mVisible)) {
      ImGui::End();
      return;
   }

   update();

   ImGui::Columns(4, "regsList", false);
   ImGui::SetColumnOffset(0, ImGui::GetWindowWidth() * 0.00f);
   ImGui::SetColumnOffset(1, ImGui::GetWindowWidth() * 0.15f);
   ImGui::SetColumnOffset(2, ImGui::GetWindowWidth() * 0.50f);
   ImGui::SetColumnOffset(3, ImGui::GetWindowWidth() * 0.65f);

   auto drawRegCol =
      [this](const std::string &name, const std::string &value, bool hasChanged) {
         ImGui::Text("%s", name.c_str());

         ImGui::NextColumn();
         if (mDebugger->paused()) {
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
                 fmt::format("{:08x}", mCurrentRegisters.gpr[i]),
                 mCurrentRegisters.gpr[i] != mPreviousRegisters.gpr[i]);

      drawRegCol(fmt::format("r{}", j),
                 fmt::format("{:08x}", mCurrentRegisters.gpr[j]),
                 mCurrentRegisters.gpr[j] != mPreviousRegisters.gpr[j]);
   }

   ImGui::Separator();

   drawRegCol("LR",
              fmt::format("{:08x}", mCurrentRegisters.lr),
              mCurrentRegisters.lr != mPreviousRegisters.lr);

   drawRegCol("CTR",
              fmt::format("{:08x}", mCurrentRegisters.ctr),
              mCurrentRegisters.ctr != mPreviousRegisters.ctr);

   ImGui::Separator();

   ImGui::NextColumn();
   ImGui::Text("O Z + -"); ImGui::NextColumn();
   ImGui::NextColumn();
   ImGui::Text("O Z + -"); ImGui::NextColumn();

   auto drawCrfCol =
      [this](uint32_t crfNum, uint32_t val, bool hasChanged) {
         ImGui::Text("crf%d", crfNum);
         ImGui::NextColumn();

         if (mDebugger->paused()) {
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
      auto iVal = (mCurrentRegisters.cr.value >> ((7 - i) * 4)) & 0xF;
      auto jVal = (mCurrentRegisters.cr.value >> ((7 - j) * 4)) & 0xF;
      auto iPrevVal = (mPreviousRegisters.cr.value >> ((7 - i) * 4)) & 0xF;
      auto jPrevVal = (mPreviousRegisters.cr.value >> ((7 - j) * 4)) & 0xF;

      drawCrfCol(i, iVal, iVal != iPrevVal);
      drawCrfCol(j, jVal, jVal != jPrevVal);
   }


   ImGui::Columns(1);
   ImGui::End();
}

} // namespace ui

} // namespace debugger
