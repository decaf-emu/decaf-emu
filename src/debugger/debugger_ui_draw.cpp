#include "debugger_ui.h"
#include <imgui.h>
#include <vector>
#include <spdlog/spdlog.h>
#include "cpu/mem.h"
#include "modules/coreinit/coreinit_scheduler.h"
#include "modules/coreinit/coreinit_internal_loader.h"
#include "kernel/kernel_hlefunction.h"

namespace debugger
{

namespace ui
{

class MemoryMapView
{
   struct Segment {
      std::string name;
      uint32_t start;
      uint32_t end;
      std::vector<Segment> items;
   };

public:
   bool isVisible = true;
   bool activateFocus = false;

   void draw()
   {
      if (!isVisible) {
         return;
      }

      if (activateFocus) {
         ImGui::SetNextWindowFocus();
         activateFocus = false;
      }

      ImGui::SetNextWindowSize(ImVec2(550, 300), ImGuiSetCond_FirstUseEver);
      if (!ImGui::Begin("Memory Segments", &isVisible)) {
         ImGui::End();
         return;
      }

      mSegments.clear();

      mSegments.push_back(Segment{ "SystemData", mem::SystemBase, mem::SystemEnd });
      mSegments.push_back(Segment{ "Application", mem::ApplicationBase, mem::ApplicationEnd });
      mSegments.push_back(Segment{ "Apertures", mem::AperturesBase, mem::AperturesEnd });
      mSegments.push_back(Segment{ "Foreground", mem::ForegroundBase, mem::ForegroundEnd });
      mSegments.push_back(Segment{ "MEM1", mem::MEM1Base, mem::MEM1End });
      mSegments.push_back(Segment{ "LockedCache", mem::LockedCacheBase, mem::LockedCacheEnd });
      mSegments.push_back(Segment{ "SharedData", mem::SharedDataBase, mem::SharedDataEnd });

      coreinit::internal::lockScheduler();
      auto &modules = coreinit::internal::getLoadedModules();
      for (auto &mod : modules) {
         for (auto &sec : mod.second->sections) {
            addSegmentItem(mSegments, Segment{
               fmt::format("{}:{}", mod.second->name, sec.name),
               sec.start,
               sec.end
            });
         }
      }
      coreinit::internal::unlockScheduler();

      ImGui::Columns(3, "memLis", false);
      ImGui::SetColumnOffset(0, ImGui::GetWindowWidth() * 0.0f);
      ImGui::SetColumnOffset(1, ImGui::GetWindowWidth() * 0.7f);
      ImGui::SetColumnOffset(2, ImGui::GetWindowWidth() * 0.85f);

      ImGui::Text("Name"); ImGui::NextColumn();
      ImGui::Text("Start"); ImGui::NextColumn();
      ImGui::Text("End"); ImGui::NextColumn();
      ImGui::Separator();
      drawSegments(mSegments, "");

      ImGui::Columns(1);
      ImGui::End();
   }

protected:
   void drawSegments(const std::vector<Segment> &segments, std::string tabs) {
      for (auto &seg : segments) {
         ImGui::Selectable(fmt::format("{}{}", tabs, seg.name).c_str()); ImGui::NextColumn();
         if (ImGui::BeginPopupContextItem(fmt::format("{}{}-actions", tabs, seg.name).c_str(), 1)) {
            if (ImGui::MenuItem("Go to in Debugger")) {

            }
            if (ImGui::MenuItem("Go to in Memory View")) {

            }
            ImGui::EndPopup();
         }
         ImGui::Text(fmt::format("{:08x}", seg.start).c_str()); ImGui::NextColumn();
         ImGui::Text(fmt::format("{:08x}", seg.end).c_str()); ImGui::NextColumn();
         drawSegments(seg.items, tabs + "  ");
      }
   }

   void addSegmentItem(std::vector<Segment> &segments, const Segment &item)
   {
      for (auto &seg : segments) {
         if (item.start >= seg.start && item.start < seg.end) {
            addSegmentItem(seg.items, item);
            return;
         }
      }
      segments.push_back(item);
   }

   std::vector<Segment> mSegments;
};

class ThreadsView
{
   struct ThreadInfo {
      uint32_t id;
      std::string name;
      OSThreadState state;
      int coreId;
   };

public:
   bool isVisible = true;
   bool activateFocus = false;

   void draw()
   {
      if (!isVisible) {
         return;
      }

      if (activateFocus) {
         ImGui::SetNextWindowFocus();
         activateFocus = false;
      }

      ImGui::SetNextWindowSize(ImVec2(550, 300), ImGuiSetCond_FirstUseEver);
      if (!ImGui::Begin("Threads", &isVisible)) {
         ImGui::End();
         return;
      }

      mThreads.clear();
      
      coreinit::internal::lockScheduler();
      auto core0Thread = coreinit::internal::getCoreRunningThread(0);
      auto core1Thread = coreinit::internal::getCoreRunningThread(1);
      auto core2Thread = coreinit::internal::getCoreRunningThread(2);
      coreinit::OSThread *firstThread = coreinit::internal::getFirstActiveThread();
      for (auto thread = firstThread; thread; thread = thread->activeLink.next) {
         int coreId = -1;
         if (thread == core0Thread) coreId = 0;
         if (thread == core1Thread) coreId = 1;
         if (thread == core2Thread) coreId = 2;

         mThreads.push_back(ThreadInfo{
            thread->id,
            thread->name ? thread->name.get() : "",
            thread->state,
            coreId
         });
      }
      coreinit::internal::unlockScheduler();

      ImGui::Columns(4, "threadList", false);
      ImGui::SetColumnOffset(0, ImGui::GetWindowWidth() * 0.0f);
      ImGui::SetColumnOffset(1, ImGui::GetWindowWidth() * 0.1f);
      ImGui::SetColumnOffset(2, ImGui::GetWindowWidth() * 0.7f);
      ImGui::SetColumnOffset(3, ImGui::GetWindowWidth() * 0.9f);

      ImGui::Text("ID"); ImGui::NextColumn();
      ImGui::Text("Name"); ImGui::NextColumn();
      ImGui::Text("State"); ImGui::NextColumn();
      ImGui::Text("Core#"); ImGui::NextColumn();
      ImGui::Separator();

      for (auto &thread : mThreads) {
         ImGui::Text(fmt::format("{}", thread.id).c_str()); ImGui::NextColumn();
         ImGui::Selectable(thread.name.c_str()); ImGui::NextColumn();
         ImGui::Text(getThreadStateName(thread.state));  ImGui::NextColumn();
         ImGui::NextColumn();
      }
      ImGui::Columns(1);
      ImGui::End();
   }


protected:
   const char * getThreadStateName(OSThreadState state) {
      switch (state) {
      case OSThreadState::None:
         return "None";
      case OSThreadState::Ready:
         return "Ready";
      case OSThreadState::Running:
         return "Running";
      case OSThreadState::Waiting:
         return "Waiting";
      case OSThreadState::Moribund:
         return "Moribund";
      default:
         return "???";
      }
   }

   std::vector<ThreadInfo> mThreads;

};

static MemoryMapView
sMemoryMapView;

static ThreadsView
sThreadsView;

void draw()
{
   static bool debugViewsVisible = false;

   auto &io = ImGui::GetIO();

   if (io.KeyCtrl && ImGui::IsKeyPressed(static_cast<int>(decaf::input::KeyboardKey::D), false)) {
      debugViewsVisible = !debugViewsVisible;
   }

   if (debugViewsVisible) {
      ImGui::BeginMainMenuBar();
      if (ImGui::BeginMenu("Debug")) {
         ImGui::MenuItem("Pause", nullptr, false, false);
         ImGui::MenuItem("Resume", nullptr, false, false);
         ImGui::MenuItem("Step Over", nullptr, false, false);
         ImGui::MenuItem("Step Into", nullptr, false, false);
         ImGui::Separator();
         if (ImGui::MenuItem("Kernel Trace Enabled", nullptr, kernel::functions::enableTrace, true)) {
            kernel::functions::enableTrace = !kernel::functions::enableTrace;
         }
         ImGui::MenuItem("GX2 Texture Dump Enabled", nullptr, false, false);
         ImGui::MenuItem("GX2 Shader Dump Enabled", nullptr, false, false);
         ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("Windows")) {
         if (ImGui::MenuItem("Memory Map", "CTRL+S", sMemoryMapView.isVisible, true)) {
            sMemoryMapView.isVisible = !sMemoryMapView.isVisible;
         }
         if (ImGui::MenuItem("Threads", "CTRL+T", sThreadsView.isVisible, true)) {
            sThreadsView.isVisible = !sThreadsView.isVisible;
         }
         ImGui::EndMenu();
      }
      ImGui::EndMainMenuBar();

      if (io.KeyCtrl && ImGui::IsKeyPressed(static_cast<int>(decaf::input::KeyboardKey::S), false)) {
         sMemoryMapView.isVisible = !sMemoryMapView.isVisible;
      }
      if (io.KeyCtrl && ImGui::IsKeyPressed(static_cast<int>(decaf::input::KeyboardKey::T), false)) {
         sThreadsView.isVisible = !sThreadsView.isVisible;
      }

      sMemoryMapView.draw();
      sThreadsView.draw();
   }
}

} // namespace ui

} // namespace debugger