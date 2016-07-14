#include "debugger_ui_internal.h"
#include "modules/coreinit/coreinit_enum_string.h"
#include "modules/coreinit/coreinit_scheduler.h"
#include "modules/coreinit/coreinit_thread.h"
#include <cinttypes>
#include <imgui.h>
#include <spdlog/spdlog.h>
#include <vector>

namespace debugger
{

namespace ui
{

namespace ThreadView
{

struct ThreadInfo
{
   coreinit::OSThread *thread;
   uint32_t id;
   std::string name;
   coreinit::OSThreadState state;
   int32_t coreId;
   uint64_t coreTimeNs;
   int32_t priority;
   int32_t basePriority;
   uint32_t affinity;
};

bool
gIsVisible = true;

static std::vector<ThreadInfo>
sThreadsCache;

void
draw()
{
   if (!gIsVisible) {
      return;
   }

   ImGui::SetNextWindowSize(ImVec2(600, 300), ImGuiSetCond_FirstUseEver);
   if (!ImGui::Begin("Threads", &gIsVisible)) {
      ImGui::End();
      return;
   }

   sThreadsCache.clear();

   coreinit::internal::lockScheduler();
   auto core0Thread = coreinit::internal::getCoreRunningThread(0);
   auto core1Thread = coreinit::internal::getCoreRunningThread(1);
   auto core2Thread = coreinit::internal::getCoreRunningThread(2);
   auto firstThread = coreinit::internal::getFirstActiveThread();

   for (auto thread = firstThread; thread; thread = thread->activeLink.next) {
      ThreadInfo tinfo;
      tinfo.thread = thread;
      tinfo.id = thread->id;
      tinfo.name = thread->name ? thread->name.get() : "";
      tinfo.state = thread->state;
      tinfo.priority = thread->priority;
      tinfo.basePriority = thread->basePriority;
      tinfo.affinity = thread->attr & coreinit::OSThreadAttributes::AffinityAny;

      if (thread == core0Thread) {
         tinfo.coreId = 0;
      } else if (thread == core1Thread) {
         tinfo.coreId = 1;
      } else if (thread == core2Thread) {
         tinfo.coreId = 2;
      } else {
         tinfo.coreId = -1;
      }

      tinfo.coreTimeNs = thread->coreTimeConsumedNs;
      if (tinfo.coreId != -1) {
         tinfo.coreTimeNs += coreinit::internal::getCoreThreadRunningTime(0);
      }

      sThreadsCache.push_back(tinfo);
   }

   coreinit::internal::unlockScheduler();

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

   for (auto &thread : sThreadsCache) {
      // ID
      ImGui::Text("%d", thread.id);
      ImGui::NextColumn();

      // Name
      if (isPaused()) {
         std::string threadName = thread.name;
         if (thread.name.size() == 0) {
            threadName = fmt::format("(Unnamed Thread {})", thread.id);
         }
         if (ImGui::Selectable(threadName.c_str())) {
            setActiveThread(thread.thread);
         }
      } else {
         ImGui::Text("%s", thread.name.c_str());
      }
      ImGui::NextColumn();

      // NIA
      if (isPaused()) {
         ImGui::Text("%08x", getThreadNia(thread.thread));
      } else {
         ImGui::Text("        ");
      }
      ImGui::NextColumn();

      // Thread State
      ImGui::Text("%s", coreinit::enumAsString(thread.state).c_str());
      ImGui::NextColumn();

      // Priority
      ImGui::Text("%d (%d)", thread.priority, thread.basePriority);
      ImGui::NextColumn();

      // Affinity
      std::string coreAff;
      if (thread.affinity & coreinit::OSThreadAttributes::AffinityCPU0) {
         coreAff += "0";
      }
      if (thread.affinity & coreinit::OSThreadAttributes::AffinityCPU1) {
         if (coreAff.size() != 0) {
            coreAff += "|1";
         } else {
            coreAff += "1";
         }
      }
      if (thread.affinity & coreinit::OSThreadAttributes::AffinityCPU2) {
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

} // namespace ThreadView

} // namespace ui

} // namespace debugger
