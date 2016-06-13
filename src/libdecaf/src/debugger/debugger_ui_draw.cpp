#include "debugger_ui.h"
#include <imgui.h>
#include <vector>
#include <spdlog/spdlog.h>
#include "libcpu/mem.h"
#include "modules/coreinit/coreinit_scheduler.h"
#include "modules/coreinit/coreinit_internal_loader.h"
#include "kernel/kernel_hlefunction.h"
#include "decaf.h"

namespace debugger
{

namespace ui
{

void openAddrInMemoryView(uint32_t addr);

class InfoView
{
public:
   void draw()
   {
      auto &io = ImGui::GetIO();
      auto ImgGuiNoBorder = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
         ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs;

      ImGui::SetNextWindowPos(ImVec2(8.0f, 25.0f));
      ImGui::SetNextWindowSize(ImVec2(180.0f, 27.0f));
      ImGui::Begin("Info", nullptr, ImgGuiNoBorder);
      float fps = decaf::getAverageFps();
      ImGui::Text("FPS: %.1f (%.3f ms)", fps, 1000.0f / fps);
      ImGui::End();
   }


protected:

};

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
      const auto &modules = coreinit::internal::getLoadedModules();
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
               openAddrInMemoryView(seg.start);
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

class MemoryView
{
public:
   MemoryView()
   {
      numColumns = 16;
      gotoTargetAddr = -1;
      editAddr = -1;
      editTakeFocus = false;
      strcpy(dataInput, "");
      strcpy(addrInput, "");
      allowEditing = true;
      regionStart = 0x00000000;
      regionSize = 0x00000000;
      userModule = 0;
      regionName = "?";
   }

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

      std::string windowKey = "Memory - " + regionName + "###Memory";
      if (!ImGui::Begin(windowKey.c_str(), &isVisible)) {
         ImGui::End();
         return;
      }

      auto base_display_addr = regionStart;
      auto mem_size = regionSize;

      ImGui::BeginChild("##scrolling", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()));

      ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

      float glyph_width = ImGui::CalcTextSize("F").x;
      float cell_width = glyph_width * 3; // "FF " we include trailing space in the width to easily catch clicks everywhere

      float line_height = ImGui::GetTextLineHeight();
      int line_total_count = (int)((mem_size + numColumns - 1) / numColumns);
      ImGuiListClipper clipper(line_total_count, line_height);
      int visible_start_addr = clipper.DisplayStart * numColumns;
      int visible_end_addr = clipper.DisplayEnd * numColumns;

      if (gotoTargetAddr != -1) {
         updateRegion(static_cast<uint32_t>(gotoTargetAddr));
         editAddr = gotoTargetAddr - regionStart;
         editTakeFocus = true;
         ImGui::SetScrollFromPosY(ImGui::GetCursorStartPos().y + (editAddr / numColumns) * ImGui::GetTextLineHeight());
         gotoTargetAddr = -1;
      }

      bool data_next = false;

      if (!allowEditing || editAddr >= mem_size)
         editAddr = -1;

      int64_t data_editing_addr_backup = editAddr;
      if (editAddr != -1)
      {
         if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_UpArrow)) && editAddr >= numColumns) {
            editAddr -= numColumns;
            editTakeFocus = true;
         } else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_DownArrow)) && editAddr < mem_size - numColumns) {
            editAddr += numColumns;
            editTakeFocus = true;
         } else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_LeftArrow)) && editAddr > 0) {
            editAddr -= 1;
            editTakeFocus = true;
         } else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_RightArrow)) && editAddr < mem_size - 1) {
            editAddr += 1;
            editTakeFocus = true;
         }
      }
      if ((editAddr / numColumns) != (data_editing_addr_backup / numColumns))
      {
         // Track cursor movements
         float scroll_offset = ((editAddr / numColumns) - (data_editing_addr_backup / numColumns)) * line_height;
         bool scroll_desired = (scroll_offset < 0.0f && editAddr < visible_start_addr + numColumns * 2) || (scroll_offset > 0.0f && editAddr > visible_end_addr - numColumns * 2);
         if (scroll_desired)
            ImGui::SetScrollY(ImGui::GetScrollY() + scroll_offset);
      }

      bool draw_separator = true;
      for (int line_i = clipper.DisplayStart; line_i < clipper.DisplayEnd; line_i++) // display only visible items
      {
         uint32_t addr = line_i * numColumns;
         ImGui::Text("%08X: ", base_display_addr + addr);
         ImGui::SameLine();

         // Draw Hexadecimal
         float line_start_x = ImGui::GetCursorPosX();
         for (int n = 0; n < numColumns && addr < mem_size; n++, addr++)
         {
            ImGui::SameLine(line_start_x + cell_width * n);

            if (editAddr == addr)
            {
               // Display text input on current byte
               ImGui::PushID(addr);
               struct FuncHolder
               {
                  // FIXME: We should have a way to retrieve the text edit cursor position more easily in the API, this is rather tedious.
                  static int Callback(ImGuiTextEditCallbackData* data)
                  {
                     int* p_cursor_pos = (int*)data->UserData;
                     if (!data->HasSelection())
                        *p_cursor_pos = data->CursorPos;
                     return 0;
                  }
               };
               int cursor_pos = -1;
               bool data_write = false;
               if (editTakeFocus)
               {
                  if (mem::valid(regionStart + addr)) {
                     ImGui::SetKeyboardFocusHere();
                     sprintf(addrInput, "%08X", base_display_addr + addr);
                     sprintf(dataInput, "%02X", mem::read<unsigned char>(regionStart + addr));
                  }
               }
               ImGui::PushItemWidth(ImGui::CalcTextSize("FF").x);
               ImGuiInputTextFlags flags = ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_NoHorizontalScroll | ImGuiInputTextFlags_AlwaysInsertMode | ImGuiInputTextFlags_CallbackAlways;
               if (ImGui::InputText("##data", dataInput, 32, flags, FuncHolder::Callback, &cursor_pos))
                  data_write = data_next = true;
               else if (!editTakeFocus && !ImGui::IsItemActive())
                  editAddr = -1;
               editTakeFocus = false;
               ImGui::PopItemWidth();
               if (cursor_pos >= 2)
                  data_write = data_next = true;
               if (data_write)
               {
                  int data;
                  if (sscanf(dataInput, "%X", &data) == 1)
                     mem::write(regionStart + addr, static_cast<unsigned char>(data));
               }
               ImGui::PopID();
            } else
            {
               if (mem::valid(regionStart + addr)) {
                  ImGui::Text("%02X ", mem::read<unsigned char>(regionStart + addr));
                  if (allowEditing && ImGui::IsItemHovered() && ImGui::IsMouseClicked(0))
                  {
                     editTakeFocus = true;
                     editAddr = addr;
                  }
               } else {
                  ImGui::Text("   ");
               }
            }
         }

         ImGui::SameLine(line_start_x + cell_width * numColumns + glyph_width * 2);

         if (draw_separator)
         {
            ImVec2 screen_pos = ImGui::GetCursorScreenPos();
            ImGui::GetWindowDrawList()->AddLine(ImVec2(screen_pos.x - glyph_width, screen_pos.y - 9999), ImVec2(screen_pos.x - glyph_width, screen_pos.y + 9999), ImColor(ImGui::GetStyle().Colors[ImGuiCol_Border]));
            draw_separator = false;
         }

         // Draw ASCII values
         addr = line_i * numColumns;
         for (int n = 0; n < numColumns && addr < mem_size; n++, addr++)
         {
            if (n > 0) ImGui::SameLine();
            if (mem::valid(regionStart + addr)) {
               unsigned char c = mem::read<unsigned char>(regionStart + addr);
               ImGui::Text("%c", (c >= 32 && c < 128) ? c : '.');
            } else {
               ImGui::Text(" ");
            }
         }
      }
      clipper.End();
      ImGui::PopStyleVar(2);

      ImGui::EndChild();

      if (data_next && editAddr < mem_size)
      {
         editAddr = editAddr + 1;
         editTakeFocus = true;
      }

      ImGui::Separator();

      ImGui::AlignFirstTextHeightToWidgets();
      ImGui::PushItemWidth(50);
      ImGui::PushAllowKeyboardFocus(false);
      int rows_backup = numColumns;
      if (ImGui::DragInt("##cols", &numColumns, 0.2f, 4, 32, "%.0f cols"))
      {
         ImVec2 new_window_size = ImGui::GetWindowSize();
         new_window_size.x += (numColumns - rows_backup) * (cell_width + glyph_width);
         ImGui::SetWindowSize(new_window_size);
      }
      ImGui::PopAllowKeyboardFocus();
      ImGui::PopItemWidth();
      ImGui::SameLine();
      ImGui::Text("Range %08X..%08X", (int)base_display_addr, (int)base_display_addr + mem_size - 1);
      ImGui::SameLine();
      ImGui::PushItemWidth(70);
      if (ImGui::InputText("##addr", addrInput, 32, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue))
      {
         uint32_t goto_addr;
         if (sscanf(addrInput, "%X", &goto_addr) == 1) {
            gotoAddress(goto_addr);
         }
      }

      ImGui::PopItemWidth();
      ImGui::End();
   }

   void gotoAddress(uint32_t addr)
   {
      gotoTargetAddr = addr;
   }

private:
   void calcDefaultRegion(uint32_t addr, uint32_t *start, uint32_t *size) {
      static const int64_t DefaultRegionPad = 0x10000;
      int64_t calcStart = (addr & 0xFFFF0000) - DefaultRegionPad;
      int64_t calcSize = 2 * DefaultRegionPad;
      if (calcStart < 0) {
         calcStart = 0;
      }
      if (calcStart + calcSize >= 0x100000000) {
         calcSize = 0x100000000 - calcStart;
      }
      *start = static_cast<uint32_t>(calcStart);
      *size = static_cast<uint32_t>(calcSize);
   }

   void updateRegion(uint32_t addr) {
      // First we look for a matching section,
      //  then we default to a surrounding default region.

      coreinit::internal::lockScheduler();
      const auto &modules = coreinit::internal::getLoadedModules();
      for (auto &mod : modules) {
         for (auto &sec : mod.second->sections) {
            if (addr >= sec.start && addr < sec.end) {
               regionName = mod.second->name + ":" + sec.name;
               regionStart = sec.start;
               regionSize = sec.end - sec.start;
               coreinit::internal::unlockScheduler();
               return;
            }
         }
      }
      coreinit::internal::unlockScheduler();

      regionName = "?";
      calcDefaultRegion(addr, &regionStart, &regionSize);
   }

   coreinit::internal::LoadedModule *userModule;
   int64_t   gotoTargetAddr;
   bool      allowEditing;
   int       numColumns;
   int64_t   editAddr;
   bool      editTakeFocus;
   char      dataInput[32];
   char      addrInput[32];
   std::string regionName;
   uint32_t  regionStart;
   uint32_t  regionSize;

};

static InfoView
sInfoView;

static MemoryMapView
sMemoryMapView;

static ThreadsView
sThreadsView;

static MemoryView
sMemoryView;

void openAddrInMemoryView(uint32_t addr)
{
   sMemoryView.gotoAddress(addr);
   sMemoryView.activateFocus = true;
   sMemoryView.isVisible = true;
}

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
         if (ImGui::MenuItem("Memory", "CTRL+M", sMemoryView.isVisible, true)) {
            sMemoryView.isVisible = !sMemoryView.isVisible;
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
      if (io.KeyCtrl && ImGui::IsKeyPressed(static_cast<int>(decaf::input::KeyboardKey::M), false)) {
         sMemoryView.isVisible = !sMemoryView.isVisible;
      }

      sInfoView.draw();
      sMemoryMapView.draw();
      sThreadsView.draw();
      sMemoryView.draw();
   }
}

} // namespace ui

} // namespace debugger