#include "debugger_ui_internal.h"
#include "decaf_config.h"
#include <imgui.h>
#include <spdlog/spdlog.h>

namespace debugger
{

namespace ui
{

namespace RegView
{

static const ImVec4 ChangedColor = HEXTOIMV4(0xF44336, 1.0f);

cpu::CoreRegs
sCurrentRegs;

cpu::CoreRegs
sPreviousRegs;

uint64_t
sLastResumeCount = 0;

coreinit::OSThread *
sLastActiveThread = nullptr;

void
draw()
{
   if (!decaf::config::debugger::show_reg_view) {
      return;
   }

   ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiSetCond_FirstUseEver);

   if (!ImGui::Begin("Registers", &decaf::config::debugger::show_reg_view)) {
      ImGui::End();
      return;
   }

   auto activeThread = getActiveThread();
   auto resumeCount = getResumeCount();

   // The reason we store current/previous separately is because
   //  by the time resumeCount has been updated to indicate that we
   //  need to swap around the previous registers, the game is already
   //  resumed making it impossible to grab the registers.
   if (sLastResumeCount != resumeCount || sLastActiveThread != activeThread) {
      sPreviousRegs = sCurrentRegs;
      sLastResumeCount = resumeCount;
      sLastActiveThread = activeThread;
   }

   if (activeThread) {
      auto coreRegs = getThreadCoreRegs(activeThread);

      if (coreRegs) {
         sCurrentRegs = *coreRegs;
      } else {
         // Set everything to some error so its obvious if something is not restored.
         memset(&sCurrentRegs, 0xF1, sizeof(cpu::CoreRegs));

         // TODO: Kernel and Debugger should share OSContext reload code
         auto state = &sCurrentRegs;
         auto context = &activeThread->context;
         for (auto i = 0; i < 32; ++i) {
            state->gpr[i] = context->gpr[i];
         }

         for (auto i = 0; i < 32; ++i) {
            state->fpr[i].value = context->fpr[i];
            state->fpr[i].paired1 = context->psf[i];
         }

         for (auto i = 0; i < 8; ++i) {
            state->gqr[i].value = context->gqr[i];
         }

         state->cr.value = context->cr;
         state->lr = context->lr;
         state->ctr = context->ctr;
         state->xer.value = context->xer;
         //state->sr[0] = context->srr0;
         //state->sr[1] = context->srr1;
         state->fpscr.value = context->fpscr;
      }
   }

   ImGui::Columns(4, "regsList", false);
   ImGui::SetColumnOffset(0, ImGui::GetWindowWidth() * 0.00f);
   ImGui::SetColumnOffset(1, ImGui::GetWindowWidth() * 0.15f);
   ImGui::SetColumnOffset(2, ImGui::GetWindowWidth() * 0.50f);
   ImGui::SetColumnOffset(3, ImGui::GetWindowWidth() * 0.65f);

   auto drawRegCol =
      [](const std::string& name, const std::string& value, bool hasChanged) {
         ImGui::Text("%s", name.c_str());

         ImGui::NextColumn();
         if (isPaused()) {
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
         fmt::format("{:08x}", sCurrentRegs.gpr[i]),
         sCurrentRegs.gpr[i] != sPreviousRegs.gpr[i]);

      drawRegCol(fmt::format("r{}", j),
         fmt::format("{:08x}", sCurrentRegs.gpr[j]),
         sCurrentRegs.gpr[j] != sPreviousRegs.gpr[j]);
   }

   ImGui::Separator();

   drawRegCol("LR",
      fmt::format("{:08x}", sCurrentRegs.lr),
      sCurrentRegs.lr != sPreviousRegs.lr);

   drawRegCol("CTR",
      fmt::format("{:08x}", sCurrentRegs.ctr),
      sCurrentRegs.ctr != sPreviousRegs.ctr);

   ImGui::Separator();

   ImGui::NextColumn();
   ImGui::Text("O Z + -"); ImGui::NextColumn();
   ImGui::NextColumn();
   ImGui::Text("O Z + -"); ImGui::NextColumn();

   auto drawCrfCol =
      [](uint32_t crfNum, uint32_t val, bool hasChanged) {
         ImGui::Text("crf%d", crfNum);
         ImGui::NextColumn();

         if (isPaused()) {
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
      auto iVal = (sCurrentRegs.cr.value >> ((7 - i) * 4)) & 0xF;
      auto jVal = (sCurrentRegs.cr.value >> ((7 - j) * 4)) & 0xF;
      auto iPrevVal = (sPreviousRegs.cr.value >> ((7 - i) * 4)) & 0xF;
      auto jPrevVal = (sPreviousRegs.cr.value >> ((7 - j) * 4)) & 0xF;

      drawCrfCol(i, iVal, iVal != iPrevVal);
      drawCrfCol(j, jVal, jVal != jPrevVal);
   }


   ImGui::Columns(1);

   ImGui::End();
}

} // namespace RegView

} // namespace ui

} // namespace debugger
