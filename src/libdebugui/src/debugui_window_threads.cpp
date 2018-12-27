#include "debugui_window_threads.h"

#include <cinttypes>
#include <fmt/format.h>
#include <imgui.h>

namespace debugui
{

static const ImVec4 CurrentColor = HEXTOIMV4(0x000000, 1.0f);
static const ImVec4 CurrentBgColor = HEXTOIMV4(0x00E676, 1.0f);

ThreadsWindow::ThreadsWindow(const std::string &name) :
   Window(name)
{
}

void
ThreadsWindow::draw()
{
   ImGui::SetNextWindowSize(ImVec2 { 600, 300 }, ImGuiSetCond_FirstUseEver);

   if (!ImGui::Begin(mName.c_str(), &mVisible)) {
      ImGui::End();
      return;
   }

   mThreadsCache.clear();
   decaf::debug::sampleCafeThreads(mThreadsCache);

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
      if (thread.handle == mStateTracker->getActiveThread().handle) {
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
      if (decaf::debug::isPaused()) {
         auto threadName = thread.name;

         if (thread.name.size() == 0) {
            threadName = fmt::format("(Unnamed Thread {})", thread.id);
         }

         if (ImGui::Selectable(threadName.c_str())) {
            mStateTracker->setActiveThread(thread);
         }
      } else {
         ImGui::Text("%s", thread.name.c_str());
      }

      ImGui::NextColumn();

      // NIA
      ImGui::Text("%08x", thread.nia);
      ImGui::NextColumn();

      // Thread State
      switch (thread.state) {
      case decaf::debug::CafeThread::Inactive:
         ImGui::Text("Inactive");
         break;
      case decaf::debug::CafeThread::Ready:
         ImGui::Text("Ready");
         break;
      case decaf::debug::CafeThread::Running:
         ImGui::Text("Running");
         break;
      case decaf::debug::CafeThread::Waiting:
         ImGui::Text("Waiting");
         break;
      case decaf::debug::CafeThread::Moribund:
         ImGui::Text("Moribund");
         break;
      default:
         ImGui::Text("");
      }
      ImGui::NextColumn();

      // Priority
      ImGui::Text("%d (%d)", thread.priority, thread.basePriority);
      ImGui::NextColumn();

      // Affinity
      std::string coreAff;

      if (thread.affinity & decaf::debug::CafeThread::ThreadAffinity::Core0) {
         coreAff += "0";
      }

      if (thread.affinity & decaf::debug::CafeThread::ThreadAffinity::Core1) {
         if (coreAff.size() != 0) {
            coreAff += "|1";
         } else {
            coreAff += "1";
         }
      }

      if (thread.affinity & decaf::debug::CafeThread::ThreadAffinity::Core2) {
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
      ImGui::Text("%" PRIu64, std::chrono::duration_cast<std::chrono::microseconds>(thread.executionTime).count());
      ImGui::NextColumn();
   }

   ImGui::Columns(1);
   ImGui::End();
}

} // namespace debugui
