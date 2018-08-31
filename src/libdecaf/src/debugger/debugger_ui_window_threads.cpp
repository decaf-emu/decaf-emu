#include "debugger_ui_window_threads.h"
#include "debugger_threadutils.h"
#include "cafe/libraries/coreinit/coreinit_enum_string.h"
#include "cafe/libraries/coreinit/coreinit_scheduler.h"
#include "cafe/libraries/coreinit/coreinit_thread.h"

#include <cinttypes>
#include <fmt/format.h>
#include <imgui.h>

namespace debugger
{

namespace ui
{

static const ImVec4 CurrentColor = HEXTOIMV4(0x000000, 1.0f);
static const ImVec4 CurrentBgColor = HEXTOIMV4(0x00E676, 1.0f);

ThreadsWindow::ThreadsWindow(const std::string &name) :
   Window(name)
{
}

void
ThreadsWindow::update()
{
   mThreadsCache.clear();

   cafe::coreinit::internal::lockScheduler();
   auto core0Thread = cafe::coreinit::internal::getCoreRunningThread(0);
   auto core1Thread = cafe::coreinit::internal::getCoreRunningThread(1);
   auto core2Thread = cafe::coreinit::internal::getCoreRunningThread(2);
   auto firstThread = cafe::coreinit::internal::getFirstActiveThread();

   for (auto thread = firstThread; thread; thread = thread->activeLink.next) {
      auto info = ThreadInfo { };
      info.thread = thread;
      info.id = thread->id;
      info.name = thread->name ? thread->name.getRawPointer() : "";
      info.state = thread->state;
      info.priority = thread->priority;
      info.basePriority = thread->basePriority;
      info.affinity = thread->attr & cafe::coreinit::OSThreadAttributes::AffinityAny;

      if (thread == core0Thread) {
         info.coreId = 0;
      } else if (thread == core1Thread) {
         info.coreId = 1;
      } else if (thread == core2Thread) {
         info.coreId = 2;
      } else {
         info.coreId = -1;
      }

      info.coreTimeNs = thread->coreTimeConsumedNs;

      if (info.coreId != -1) {
         info.coreTimeNs += cafe::coreinit::internal::getCoreThreadRunningTime(info.coreId);
      }

      mThreadsCache.push_back(info);
   }

   cafe::coreinit::internal::unlockScheduler();
}

void
ThreadsWindow::draw()
{
   ImGui::SetNextWindowSize(ImVec2 { 600, 300 }, ImGuiSetCond_FirstUseEver);

   if (!ImGui::Begin(mName.c_str(), &mVisible)) {
      ImGui::End();
      return;
   }

   update();

   ImGui::Columns(8, "threadList", false);
   ImGui::SetColumnOffset(0, ImGui::GetWindowWidth() * 0.00f);
   ImGui::SetColumnOffset(1, ImGui::GetWindowWidth() * 0.05f);
   ImGui::SetColumnOffset(2, ImGui::GetWindowWidth() * 0.35f);
   ImGui::SetColumnOffset(3, ImGui::GetWindowWidth() * 0.50f);
   ImGui::SetColumnOffset(4, ImGui::GetWindowWidth() * 0.60f);
   ImGui::SetColumnOffset(5, ImGui::GetWindowWidth() * 0.70f);
   ImGui::SetColumnOffset(6, ImGui::GetWindowWidth() * 0.75f);
   ImGui::SetColumnOffset(7, ImGui::GetWindowWidth() * 0.84f);

   ImGui::Text("ID"); ImGui::NextColumn();
   ImGui::Text("Name"); ImGui::NextColumn();
   ImGui::Text("NIA"); ImGui::NextColumn();
   ImGui::Text("State"); ImGui::NextColumn();
   ImGui::Text("Prio"); ImGui::NextColumn();
   ImGui::Text("Aff"); ImGui::NextColumn();
   ImGui::Text("Core"); ImGui::NextColumn();
   ImGui::Text("Core Time"); ImGui::NextColumn();
   ImGui::Separator();

   for (auto &thread : mThreadsCache) {
      // ID
      if (thread.thread == mStateTracker->getActiveThread()) {
         // Highlight the currently active thread
         // TODO: Clean this up
         auto idStr = fmt::format("{}", thread.id);
         auto drawList = ImGui::GetWindowDrawList();
         auto lineHeight = ImGui::GetTextLineHeight();
         auto glyphWidth = ImGui::CalcTextSize("FF").x - ImGui::CalcTextSize("F").x;
         auto idWidth = glyphWidth * idStr.length();
         auto rootPos = ImGui::GetCursorScreenPos();
         auto idMin = ImVec2 { rootPos.x - 1, rootPos.y };
         auto idMax = ImVec2 { rootPos.x + idWidth + 2, rootPos.y + lineHeight + 1 };

         drawList->AddRectFilled(idMin, idMax, ImColor(CurrentBgColor), 2.0f);
         ImGui::TextColored(CurrentColor, "%s", idStr.c_str());
      } else {
         ImGui::Text("%d", thread.id);
      }
      ImGui::NextColumn();

      // Name
      if (mDebugger->paused()) {
         auto threadName = thread.name;

         if (thread.name.size() == 0) {
            threadName = fmt::format("(Unnamed Thread {})", thread.id);
         }

         if (ImGui::Selectable(threadName.c_str())) {
            mStateTracker->setActiveThread(thread.thread);
         }
      } else {
         ImGui::Text("%s", thread.name.c_str());
      }

      ImGui::NextColumn();

      // NIA
      if (mDebugger->paused()) {
         ImGui::Text("%08x", getThreadNia(mDebugger, thread.thread));
      } else {
         ImGui::Text("        ");
      }

      ImGui::NextColumn();

      // Thread State
      ImGui::Text("%s", cafe::coreinit::to_string(thread.state).c_str());
      ImGui::NextColumn();

      // Priority
      ImGui::Text("%d (%d)", thread.priority, thread.basePriority);
      ImGui::NextColumn();

      // Affinity
      std::string coreAff;

      if (thread.affinity & cafe::coreinit::OSThreadAttributes::AffinityCPU0) {
         coreAff += "0";
      }

      if (thread.affinity & cafe::coreinit::OSThreadAttributes::AffinityCPU1) {
         if (coreAff.size() != 0) {
            coreAff += "|1";
         } else {
            coreAff += "1";
         }
      }

      if (thread.affinity & cafe::coreinit::OSThreadAttributes::AffinityCPU2) {
         if (coreAff.size() != 0) {
            coreAff += "|2";
         } else {
            coreAff += "2";
         }
      }

      ImGui::Text("%s", coreAff.c_str());
      ImGui::NextColumn();

      // Core Id
      if (thread.coreId != -1) {
         ImGui::Text("%d", thread.coreId);
      }

      ImGui::NextColumn();

      // Core Time
      ImGui::Text("%" PRIu64, thread.coreTimeNs / 1000);
      ImGui::NextColumn();
   }

   ImGui::Columns(1);
   ImGui::End();
}

} // namespace ui

} // namespace debugger
