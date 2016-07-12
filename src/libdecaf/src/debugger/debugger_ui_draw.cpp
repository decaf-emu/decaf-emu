#include "common/decaf_assert.h"
#include "common/strutils.h"
#include "debugger.h"
#include "debugger_analysis.h"
#include "debugger_branchcalc.h"
#include "debugger_ui.h"
#include "decaf.h"
#include "kernel/kernel_hlefunction.h"
#include "libcpu/espresso/espresso_disassembler.h"
#include "libcpu/mem.h"
#include "modules/coreinit/coreinit_scheduler.h"
#include "modules/coreinit/coreinit_internal_loader.h"
#include "modules/coreinit/coreinit_enum_string.h"
#include <cinttypes>
#include <functional>
#include <imgui.h>
#include <map>
#include <spdlog/spdlog.h>
#include <sstream>
#include <vector>

#define HEXTOF(h) static_cast<float>(h&0xFF)/255.0f
#define HEXTOIMV4(h, a) ImVec4(HEXTOF(h>>16), HEXTOF(h>>8), HEXTOF(h>>0), a)

namespace debugger
{

namespace ui
{

static const ImVec4 InfoPausedTextColor = HEXTOIMV4(0xEF5350, 1.0f);
static const ImVec4 InfoRunningTextColor = HEXTOIMV4(0x8BC34A, 1.0f);
static const ImVec4 DisasmDataColor = HEXTOIMV4(0xB0C4CE, 1.0f);
static const ImVec4 DisasmFuncColor = HEXTOIMV4(0xAB47BC, 1.0f);
static const ImVec4 DisasmFuncLinkColor = HEXTOIMV4(0xCFD8DC, 1.0f);
static const ImVec4 DisasmFuncFollowColor = HEXTOIMV4(0x66BB6A, 1.0f);
static const ImVec4 DisasmFuncSkipColor = HEXTOIMV4(0xF44336, 1.0f);
static const ImVec4 DisasmJmpColor = HEXTOIMV4(0xFF5722, 1.0f);
static const ImVec4 DisasmSelBgColor = HEXTOIMV4(0x263238, 1.0f);
static const ImVec4 DisasmNiaColor = HEXTOIMV4(0x000000, 1.0f);
static const ImVec4 DisasmNiaBgColor = HEXTOIMV4(0x00E676, 1.0f);
static const ImVec4 DisasmBpColor = HEXTOIMV4(0x000000, 1.0f);
static const ImVec4 DisasmBpBgColor = HEXTOIMV4(0xF44336, 1.0f);
static const ImVec4 RegsChangedColor = HEXTOIMV4(0xF44336, 1.0f);
static const ImVec4 ThreadsCurrentColor = HEXTOIMV4(0x000000, 1.0f);
static const ImVec4 ThreadsCurrentBgColor = HEXTOIMV4(0x00E676, 1.0f);

// We store this locally so that we do not end up with isRunning
//  switching whilst we are in the midst of drawing the UI.
static bool
sIsPaused = false;

static uint64_t
sResumeCount = 0;

static uint32_t
sActiveCore = 1;

static coreinit::OSThread *
sActiveThread = nullptr;

static std::map<uint32_t, bool>
sBreakpoints;

void openAddrInMemoryView(uint32_t addr);
void openAddrInDisassemblyView(uint32_t addr);

bool hasBreakpoint(uint32_t address)
{
   auto bpIter = sBreakpoints.find(address);
   return bpIter != sBreakpoints.end() && bpIter->second;
}

void toggleBreakpoint(uint32_t address)
{
   if (sBreakpoints[address]) {
      cpu::removeBreakpoint(address, cpu::USER_BPFLAG);
      sBreakpoints[address] = false;
   } else {
      cpu::addBreakpoint(address, cpu::USER_BPFLAG);
      sBreakpoints[address] = true;
   }
}

uint32_t getThreadNia(coreinit::OSThread *thread)
{
   decaf_check(sIsPaused);

   coreinit::OSThread *coreThread[3] = {
      coreinit::internal::getCoreRunningThread(0),
      coreinit::internal::getCoreRunningThread(1),
      coreinit::internal::getCoreRunningThread(2)
   };

   if (thread == coreThread[0]) {
      return debugger::getPausedCoreState(0)->nia;
   } else if (thread == coreThread[1]) {
      return debugger::getPausedCoreState(1)->nia;
   } else if (thread == coreThread[2]) {
      return debugger::getPausedCoreState(2)->nia;
   } else {
      return mem::read<uint32_t>(thread->context.gpr[1] + 8);
   }
}

class AddressScroller
{
public:
   AddressScroller()
      : mNumColumns(1), mScrollPos(0x00000000), mScrollToAddress(0xFFFFFFFF)
   {
   }

   void Begin(int64_t numColumns, ImVec2 size)
   {
      ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
      ImGui::BeginChild("##scrolling", size, false, flags);
      ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

      // There is a differentiation between visible and rendered lines as if
      //  a line is half-visible, we don't want to count it as being a 'visible'
      //  line, on the other hand, we should render the half-line if we can.
      float lineHeight = ImGui::GetTextLineHeight();
      float clipHeight = ImGui::GetWindowHeight();
      int64_t numVisibleLines = static_cast<int64_t>(std::floor(clipHeight / lineHeight));
      int64_t numDrawLines = static_cast<int64_t>(std::ceil(clipHeight / lineHeight));

      if (ImGui::IsWindowHovered()) {
         auto &io = ImGui::GetIO();
         int64_t scrollDelta = -static_cast<int64_t>(io.MouseWheel);
         SetScrollPos(mScrollPos + scrollDelta * numColumns);
      }

      mNumColumns = numColumns;
      mNumDrawLines = numDrawLines;
      mNumVisibleLines = numVisibleLines;

      // We defer ScrollTo requests to make sure we have all the meta-data
      //  we need in order to accurately calculate the scroll position.
      if (mScrollToAddress != -1) {
         if (std::abs(mScrollToAddress - mScrollPos) >= 0x400) {
            // If we are making a significant jump, don't bother doing a friendly
            //  scroll, lets just put the address at the top of the view.

            SetScrollPos(mScrollToAddress - mNumColumns);
         } else {
            // Try to just scroll the view so that the user can keep track
            //  of the icon easier after scrolling...

            const int64_t minVisBound = mNumColumns;
            const int64_t maxVisBound = (mNumVisibleLines * mNumColumns) - mNumColumns - mNumColumns;

            if (mScrollToAddress < mScrollPos + minVisBound) {
               SetScrollPos(mScrollToAddress - minVisBound);
            }
            if (mScrollToAddress >= mScrollPos + maxVisBound) {
               SetScrollPos(mScrollToAddress - maxVisBound);
            }
         }

         mScrollToAddress = -1;
      }
   }

   void End()
   {
      ImGui::PopStyleVar(2);
      ImGui::EndChild();
   }

   uint32_t Reset()
   {
      mIterPos = mScrollPos;
      return static_cast<uint32_t>(mIterPos);
   }

   uint32_t Advance()
   {
      mIterPos += mNumColumns;
      return static_cast<uint32_t>(mIterPos);
   }

   bool HasMore()
   {
      return mIterPos < mScrollPos + mNumDrawLines * mNumColumns && mIterPos < 0x100000000;
   }

   bool IsValidOffset(uint32_t offset) {
      return mIterPos + static_cast<int64_t>(offset) < 0x100000000;
   }

   void ScrollTo(uint32_t address)
   {
      mScrollToAddress = address;
   }

private:
   void SetScrollPos(int64_t position)
   {
      int64_t numTotalLines = (0x100000000 + mNumColumns - 1) / mNumColumns;

      // Make sure we stay within the bounds of our memory
      int64_t maxScrollPos = (numTotalLines - mNumVisibleLines) * mNumColumns;
      position = std::max<int64_t>(0, position);
      position = std::min<int64_t>(position, maxScrollPos);

      // Remap the scroll position to the closest line start
      position -= position % mNumColumns;

      mScrollPos = position;
   }

   int64_t mNumColumns;
   int64_t mScrollPos;
   int64_t mIterPos;
   int64_t mNumDrawLines;
   int64_t mNumVisibleLines;
   int64_t mScrollToAddress;

};

class InfoView
{
public:
   void draw()
   {
      auto &io = ImGui::GetIO();
      auto ImgGuiNoBorder =
         ImGuiWindowFlags_NoTitleBar
         | ImGuiWindowFlags_NoResize
         | ImGuiWindowFlags_NoMove
         | ImGuiWindowFlags_NoScrollbar
         | ImGuiWindowFlags_NoSavedSettings
         | ImGuiWindowFlags_NoInputs;

      ImGui::SetNextWindowPos(ImVec2(8.0f, 25.0f));
      ImGui::SetNextWindowSize(ImVec2(180.0f, 45.0f));
      ImGui::Begin("Info", nullptr, ImgGuiNoBorder);

      float fps = decaf::getGraphicsDriver()->getAverageFPS();
      ImGui::Text("FPS: %.1f (%.3f ms)", fps, 1000.0f / fps);

      ImGui::Text("Status: ");
      ImGui::SameLine();
      if (sIsPaused) {
         ImGui::TextColored(InfoPausedTextColor, "Paused");
      } else {
         ImGui::TextColored(InfoRunningTextColor, "Running...");
      }

      ImGui::End();
   }

protected:
};

class MemoryMapView
{
   struct Segment
   {
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
      mSegments.push_back(Segment { "SystemData", mem::SystemBase, mem::SystemEnd });
      mSegments.push_back(Segment { "Application", mem::ApplicationBase, mem::ApplicationEnd });
      mSegments.push_back(Segment { "Apertures", mem::AperturesBase, mem::AperturesEnd });
      mSegments.push_back(Segment { "Foreground", mem::ForegroundBase, mem::ForegroundEnd });
      mSegments.push_back(Segment { "MEM1", mem::MEM1Base, mem::MEM1End });
      mSegments.push_back(Segment { "LockedCache", mem::LockedCacheBase, mem::LockedCacheEnd });
      mSegments.push_back(Segment { "SharedData", mem::SharedDataBase, mem::SharedDataEnd });

      coreinit::internal::lockLoader();
      const auto &modules = coreinit::internal::getLoadedModules();

      for (auto &mod : modules) {
         for (auto &sec : mod.second->sections) {
            addSegmentItem(mSegments, Segment {
               fmt::format("{}:{}", mod.second->name, sec.name),
               sec.start,
               sec.end
            });
         }
      }
      coreinit::internal::unlockLoader();

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
   void drawSegments(const std::vector<Segment> &segments, std::string tabs)
   {
      for (auto &seg : segments) {
         ImGui::Selectable(fmt::format("{}{}", tabs, seg.name).c_str()); ImGui::NextColumn();

         if (ImGui::BeginPopupContextItem(fmt::format("{}{}-actions", tabs, seg.name).c_str(), 1)) {
            if (ImGui::MenuItem("Go to in Debugger")) {
               openAddrInDisassemblyView(seg.start);
            }

            if (ImGui::MenuItem("Go to in Memory View")) {
               openAddrInMemoryView(seg.start);
            }

            ImGui::EndPopup();
         }

         ImGui::Text("%08x", seg.start); ImGui::NextColumn();
         ImGui::Text("%08x", seg.end); ImGui::NextColumn();
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

      ImGui::SetNextWindowSize(ImVec2(600, 300), ImGuiSetCond_FirstUseEver);
      if (!ImGui::Begin("Threads", &isVisible)) {
         ImGui::End();
         return;
      }

      mThreads.clear();

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

         mThreads.push_back(tinfo);
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

      for (auto &thread : mThreads) {
         // ID
         if (thread.thread == sActiveThread) {
            // Draw a green background under the current thread's ID
            auto drawList = ImGui::GetWindowDrawList();
            auto wndWidth = ImGui::GetWindowContentRegionWidth();
            auto lineHeight = ImGui::GetTextLineHeight();
            float glyphWidth = ImGui::CalcTextSize("FF").x - ImGui::CalcTextSize("F").x;
            auto rootPos = ImGui::GetCursorScreenPos();
            auto lineMin = ImVec2(rootPos.x, rootPos.y);
            auto lineMax = ImVec2(rootPos.x + wndWidth, rootPos.y + lineHeight);
            auto idMin = ImVec2(lineMin.x - 1, lineMin.y);
            auto idMax = ImVec2(lineMin.x + (glyphWidth * 2) + 2, lineMax.y);
            drawList->AddRectFilled(idMin, idMax, ImColor(ThreadsCurrentBgColor), 2.0f);
            // Use a custom text color to make it easier to read
            ImGui::TextColored(ThreadsCurrentColor, "%d", thread.id);
         } else {
            ImGui::Text("%d", thread.id);
         }
         ImGui::NextColumn();

         // Name
         if (sIsPaused) {
            const char *threadName;
            std::string nameBuf;
            if (thread.name.size() > 0) {
               threadName = thread.name.c_str();
            } else {
               nameBuf = fmt::format("(Unnamed Thread {})", thread.id);
               threadName = nameBuf.c_str();
            }
            if (ImGui::Selectable(threadName)) {
               setActiveThread(thread.thread);
            }
         } else {
            ImGui::Text("%s", thread.name.c_str());
         }
         ImGui::NextColumn();

         // NIA
         if (sIsPaused) {
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

protected:
   std::vector<ThreadInfo> mThreads;
};

class MemoryView
{
public:
   MemoryView()
      : mGotoTargetAddr(-1), mLastEditAddress(-1),
         mEditAddress(-1), mEditingEnabled(true)
   {
      mAddressInput[0] = 0;
      mDataInput[0] = 0;
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

      std::string windowKey = "Memory";
      if (!ImGui::Begin(windowKey.c_str(), &isVisible)) {
         ImGui::End();
         return;
      }

      // We use this 'hack' to get the true with without line-advance offsets.
      float glyphWidth = ImGui::CalcTextSize("FF").x - ImGui::CalcTextSize("F").x;
      float cellWidth = glyphWidth * 3;

      // We precalcalculate these so we can reverse engineer our column
      //  count and line render locations.
      float addrAdvance = glyphWidth * 10.0f;
      float cellAdvance = glyphWidth * 2.5f;
      float gapAdvance = glyphWidth * 1.0f;
      float charAdvance = glyphWidth * 1.0f;
      auto wndDynRegionSize = ImGui::GetWindowContentRegionWidth() - addrAdvance - gapAdvance;
      int64_t numColumns = static_cast<int64_t>(wndDynRegionSize / (cellAdvance + charAdvance));

      // Impose a limit on the number of columns in case the window
      //  does not yet have a size.
      numColumns = std::max<int64_t>(1, std::min<int64_t>(numColumns, 64));

      // Clear the last edit address whenever our current address is cleared
      if (mEditAddress == -1) {
         mLastEditAddress = -1;
      }

      // Check if we need to move around or scroll
      if (mEditAddress != -1)
      {
         auto originalAddress = mEditAddress;

         // Check if the user wants to move
         if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_UpArrow))) {
            mEditAddress -= numColumns;
         } else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_DownArrow))) {
            mEditAddress += numColumns;
         } else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_LeftArrow))) {
            mEditAddress--;
         } else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_RightArrow))) {
            mEditAddress++;
         }

         // Clamp the edit address
         mEditAddress = std::max<int64_t>(0, mEditAddress);
         mEditAddress = std::min<int64_t>(mEditAddress, 0xFFFFFFFF);

         // Before we start processing an edit, lets make sure it's valid memory to be editing...
         if (!mem::valid(static_cast<uint32_t>(mEditAddress))) {
            mEditAddress = -1;
         }

         // Make sure that the address always stays visible!  We do this before
         //  checking for valid memory so you can still goto an invalid address.
         if (mEditAddress != originalAddress && mEditAddress != -1) {
            mScroller.ScrollTo(static_cast<uint32_t>(mEditAddress));
         }
      }

      int64_t editAddress = mEditAddress;

      mScroller.Begin(numColumns, ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()));
      for (auto addr = mScroller.Reset(); mScroller.HasMore(); addr = mScroller.Advance()) {
         auto linePos = ImGui::GetCursorPos();

         // Render the address for this line
         ImGui::Text("%08X:", addr);
         linePos.x += addrAdvance;

         // Draw all of the hex cells
         for (uint32_t i = 0; i < numColumns; ++i) {
           if (static_cast<int64_t>(addr + i) == editAddress) {
              // If this is the address that we are currently editing, lets
              //  render an input box rather than just text...
              ImGui::SetCursorPos(linePos);
              ImGui::PushID(addr + i);

              // If the active edit address has changed, lets make sure to force
              //  the focus to the new input box.
              bool newlyFocused = false;
              if (mLastEditAddress != editAddress) {
                 ImGui::SetKeyboardFocusHere();

                 uint32_t targetAddress = static_cast<uint32_t>(editAddress);
                 snprintf(mAddressInput, 32, "%08X", targetAddress);
                 snprintf(mDataInput, 32, "%02X", mem::read<unsigned char>(targetAddress));

                 mLastEditAddress = editAddress;
                 newlyFocused = true;
              }

              // Draw the actual input box for the hex input
              int cursorPos = -1;
              ImGui::PushItemWidth(glyphWidth * 2);
              ImGuiInputTextFlags flags =
                 ImGuiInputTextFlags_CharsHexadecimal |
                 ImGuiInputTextFlags_EnterReturnsTrue |
                 ImGuiInputTextFlags_AutoSelectAll |
                 ImGuiInputTextFlags_NoHorizontalScroll |
                 ImGuiInputTextFlags_AlwaysInsertMode |
                 ImGuiInputTextFlags_CallbackAlways;
              if (ImGui::InputText("##data", mDataInput, 32, flags, [](auto data) {
                 auto cursorPosPtr = static_cast<int*>(data->UserData);
                 if (!data->HasSelection())
                    *cursorPosPtr = data->CursorPos;
                 return 0;
              }, &cursorPos)) {
                 mEditAddress += 1;
              } else if (!newlyFocused && !ImGui::IsItemActive()) {
                 mEditAddress = -1;
              }
              if (cursorPos >= 2) {
                 mEditAddress += 1;
              }
              ImGui::PopItemWidth();

              ImGui::PopID();

              if (mEditAddress != mLastEditAddress) {
                 // If the edit address has changed, we need to update
                 //  the memory itself.
                 std::istringstream is(mDataInput);
                 int data;
                 if (is >> std::hex >> data) {
                    mem::write(addr + i, static_cast<unsigned char>(data));
                 }
              }

            } else {
              ImGui::SetCursorPos(linePos);
               if (mScroller.IsValidOffset(i) && mem::valid(addr + i)) {
                  ImGui::Text("%02X ", mem::read<unsigned char>(addr + i));
                  if (mEditingEnabled && ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
                     mEditAddress = addr + i;
                  }
               } else {
                  ImGui::Text("??");
               }
            }

            linePos.x += cellAdvance;
         }

         // Draw a line?
         linePos.x += gapAdvance;

         // Draw all of the ASCII characters
         for (uint32_t i = 0; i < numColumns; ++i) {
            if (mScroller.IsValidOffset(i) && mem::valid(addr + i)) {
               ImGui::SetCursorPos(linePos);
               unsigned char c = mem::read<unsigned char>(addr + i);
               ImGui::Text("%c", (c >= 32 && c < 128) ? c : '.');
            }

            linePos.x += charAdvance;
         }

      }
      mScroller.End();

      ImGui::Separator();

      // Render the bottom bar for the window
      ImGui::AlignFirstTextHeightToWidgets();
      ImGui::Text("Go To Address: ");
      ImGui::SameLine();
      ImGui::PushItemWidth(70);
      if (ImGui::InputText("##addr", mAddressInput, 32, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue)) {
         std::istringstream is(mAddressInput);
         uint32_t goto_addr;
         if ((is >> std::hex >> goto_addr)) {
            gotoAddress(goto_addr);
         }
      }
      ImGui::PopItemWidth();
      ImGui::SameLine();
      ImGui::Text("Showing %d Columns", static_cast<int>(numColumns));

      // End the memory view window
      ImGui::End();
   }

   void gotoAddress(uint32_t addr)
   {
      mScroller.ScrollTo(addr);
      mEditAddress = addr;
   }

private:
   // We use 64-bit here so we can deal with the 32-bit PPC memory
   //  ranges easier without reverting to inclusive ranges.
   AddressScroller mScroller;
   bool mEditingEnabled;
   int64_t mEditAddress;
   int64_t mLastEditAddress;
   int64_t mGotoTargetAddr;
   char mAddressInput[32];
   char mDataInput[32];

};

class DisassemblyView
{
public:
   DisassemblyView()
      : mSelectedAddr(-1)
   {
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

      std::string windowKey = "Disassembly";
      if (!ImGui::Begin(windowKey.c_str(), &isVisible)) {
         ImGui::End();
         return;
      }

      // Lets grab the active core registers if they are available to us...
      cpu::CoreRegs *activeCoreRegs = nullptr;
      if (sActiveThread && sActiveCore != -1) {
         activeCoreRegs = getPausedCoreState(sActiveCore);
      }

      // Check if we need to move around or scroll or mark stuff
      if (mSelectedAddr != -1)
      {
         auto originalAddress = mSelectedAddr;

         // Check if the user tapped F, if so, mark this as a function!
         if (ImGui::IsKeyPressed(static_cast<int>(decaf::input::KeyboardKey::F))) {
            analysis::toggleAsFunction(static_cast<uint32_t>(mSelectedAddr));
         }

         // Check if the user tapped Enter, if so, jump to the branch target!
         if (ImGui::IsKeyPressed(static_cast<int>(decaf::input::KeyboardKey::Enter))) {
            uint32_t selectedAddr = static_cast<uint32_t>(mSelectedAddr);
            auto instr = mem::read<espresso::Instruction>(selectedAddr);
            auto data = espresso::decodeInstruction(instr);

            if (isBranchInstr(data)) {
               auto meta = getBranchMeta(selectedAddr, instr, data, activeCoreRegs);
               if (!meta.isVariable || activeCoreRegs) {
                  mSelectedAddr = meta.target;
               }
            }
         }

         // Check if the user wants to move
         if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_UpArrow))) {
            mSelectedAddr -= 4;
         } else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_DownArrow))) {
            mSelectedAddr += 4;
         }

         // Clamp the edit address
         mSelectedAddr = std::max<int64_t>(0, mSelectedAddr);
         mSelectedAddr = std::min<int64_t>(mSelectedAddr, 0xFFFFFFFF);

         // Before we start processing an edit, lets make sure it's valid memory to be editing...
         if (!mem::valid(static_cast<uint32_t>(mSelectedAddr))) {
            mSelectedAddr = -1;
         }

         // Make sure that the address always stays visible!  We do this before
         //  checking for valid memory so you can still goto an invalid address.
         if (mSelectedAddr != originalAddress && mSelectedAddr != -1) {
            mScroller.ScrollTo(static_cast<uint32_t>(mSelectedAddr));
         }
      }

      // We use this 'hack' to get the true width without line-advance offsets.
      auto lineHeight = ImGui::GetTextLineHeight();
      float glyphWidth = ImGui::CalcTextSize("FF").x - ImGui::CalcTextSize("F").x;

      // We precalcalculate
      float addrAdvance = glyphWidth * 10.0f;
      float dataAdvance = glyphWidth * 9.0f;
      float funcLineAdvance = glyphWidth * 1.0f;
      float jmpArrowAdvance = glyphWidth * 1.0f;
      float jmpLineAdvance = glyphWidth * 1.0f;
      float instrAdvance = glyphWidth * 28.0f;

      // We grab some stuff to do some custom rendering...
      auto drawList = ImGui::GetWindowDrawList();
      auto wndWidth = ImGui::GetWindowContentRegionWidth();

      // Lets precalculate some stuff we need for the currently visible instructions
      enum class BranchGlyph : uint32_t {
         None,
         StartDown,
         StartUp,
         EndDown,
         EndUp,
         Middle,
         EndBoth,
      };
      struct VisInstrInfo {
         BranchGlyph branchGlyph;
         ImVec4 branchColor;
      };
      std::map<uint32_t, VisInstrInfo> visInstrInfo;

      mScroller.Begin(4, ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()));

      // Find the upper and lower bounds of the visible area
      uint32_t visFirstAddr = mScroller.Reset();
      uint32_t visLastAddr = visFirstAddr;
      for (auto addr = visFirstAddr; mScroller.HasMore(); addr = mScroller.Advance()) {
         visLastAddr = addr;
      }

      auto ForEachVisInstr = [&](uint32_t first, uint32_t last, std::function<void(uint32_t,VisInstrInfo&)> fn) {
         if (first > last) {
            std::swap(first, last);
         }

         auto visLoopFirst = std::max(first, visFirstAddr);
         auto visLoopLast = std::min(last, visLastAddr);
         if (visLoopFirst > visLastAddr || visLoopLast < visFirstAddr) {
            return;
         }
         for (uint32_t addr = visLoopFirst; ; addr += 4) {
            fn(addr, visInstrInfo[addr]);
            if (addr == visLoopLast) {
               break;
            }
         }
      };

      if (mSelectedAddr != -1) {
         uint32_t selectedAddr = static_cast<uint32_t>(mSelectedAddr);
         auto instr = mem::read<espresso::Instruction>(selectedAddr);
         auto data = espresso::decodeInstruction(instr);
         auto info = analysis::get(selectedAddr);

         bool isVisBranchSource = false;
         if (data && isBranchInstr(data)) {
            auto meta = getBranchMeta(selectedAddr, instr, data, activeCoreRegs);
            if (!meta.isCall && (!meta.isVariable || activeCoreRegs)) {
               ForEachVisInstr(selectedAddr, meta.target, [&](uint32_t addr, VisInstrInfo &vii) {
                  if (addr == selectedAddr) {
                     if (meta.target < selectedAddr) {
                        vii.branchGlyph = BranchGlyph::StartUp;
                     } else {
                        vii.branchGlyph = BranchGlyph::StartDown;
                     }
                  } else if (addr == meta.target) {
                     if (meta.target < selectedAddr) {
                        vii.branchGlyph = BranchGlyph::EndUp;
                     } else {
                        vii.branchGlyph = BranchGlyph::EndDown;
                     }
                  } else {
                     vii.branchGlyph = BranchGlyph::Middle;
                  }
                  if (activeCoreRegs && meta.conditionSatisfied) {
                     vii.branchColor = DisasmFuncFollowColor;;
                  } else if (activeCoreRegs && !meta.conditionSatisfied) {
                     vii.branchColor = DisasmFuncSkipColor;
                  } else {
                     vii.branchColor = DisasmFuncLinkColor;
                  }
               });
               isVisBranchSource = true;
            }
         }
         if (!isVisBranchSource) {
            if (info.instr) {
               auto &visInfo = visInstrInfo[selectedAddr];

               uint32_t selectedMinSource = selectedAddr;
               uint32_t selectedMaxSource = selectedAddr;

               for (auto sourceAddr : info.instr->sourceBranches) {
                  if (sourceAddr > selectedMaxSource) {
                     selectedMaxSource = sourceAddr;
                  }
                  if (sourceAddr < selectedMinSource) {
                     selectedMinSource = sourceAddr;
                  }

                  auto &sourceVisInfo = visInstrInfo[sourceAddr];
                  if (sourceAddr > selectedAddr) {
                     sourceVisInfo.branchGlyph = BranchGlyph::StartUp;
                  } else {
                     sourceVisInfo.branchGlyph = BranchGlyph::StartDown;
                  }
                  sourceVisInfo.branchColor = DisasmFuncLinkColor;
               }

               if (selectedMinSource < selectedAddr && selectedMaxSource > selectedAddr) {
                  visInfo.branchGlyph = BranchGlyph::EndBoth;
               } else if (selectedMinSource < selectedAddr) {
                  visInfo.branchGlyph = BranchGlyph::EndDown;
               } else if (selectedMaxSource > selectedAddr) {
                  visInfo.branchGlyph = BranchGlyph::EndUp;
               }

               if (selectedMinSource != selectedMaxSource) {
                  ForEachVisInstr(selectedMinSource, selectedMaxSource, [&](uint32_t addr, VisInstrInfo &vii) {
                     if (vii.branchGlyph == BranchGlyph::None) {
                        vii.branchGlyph = BranchGlyph::Middle;
                        vii.branchColor = DisasmFuncLinkColor;
                     }
                  });
               }

               visInfo.branchColor = DisasmFuncLinkColor;
            }
         }
      }

      for (auto addr = mScroller.Reset(); mScroller.HasMore(); addr = mScroller.Advance()) {
         auto rootPos = ImGui::GetCursorScreenPos();
         auto linePos = ImGui::GetCursorPos();

         auto lineMin = ImVec2(rootPos.x, rootPos.y);
         auto lineMax = ImVec2(rootPos.x + wndWidth, rootPos.y + lineHeight);

         // Handle a new address being selected
         if (ImGui::IsMouseHoveringRect(lineMin, lineMax)) {
            if (ImGui::IsMouseClicked(0) || ImGui::IsMouseDown(0)) {
               mSelectedAddr = addr;
            }
            // Maybe render a 'highlight' here?
         }

         // Draw the rectangle for the current line selection
         if (static_cast<int64_t>(addr) == mSelectedAddr) {
            auto lineMinB = ImVec2(lineMin.x - 1, lineMin.y);
            auto lineMaxB = ImVec2(lineMax.x + 1, lineMax.y);
            drawList->AddRectFilled(lineMinB, lineMaxB, ImColor(DisasmSelBgColor), 2.0f);
         }

         // Check if this is the current instruction
         // This should be simpler...  gActiveCoreIdx instead maybe?
         if (sIsPaused && sActiveThread && addr == getThreadNia(sActiveThread)) {
            // Render a green BG behind the address
            auto lineMinC = ImVec2(lineMin.x - 1, lineMin.y);
            auto lineMaxC = ImVec2(lineMin.x + (glyphWidth * 8) + 2, lineMax.y);
            drawList->AddRectFilled(lineMinC, lineMaxC, ImColor(DisasmNiaBgColor), 2.0f);

            // Render the address for this line
            //  (custom colored text so it is easy to see)
            ImGui::TextColored(DisasmNiaColor, "%08X:", addr);
         } else if (hasBreakpoint(addr)) {
            // Render a red BG behind the address
            auto lineMinC = ImVec2(lineMin.x - 1, lineMin.y);
            auto lineMaxC = ImVec2(lineMin.x + (glyphWidth * 8) + 2, lineMax.y);
            drawList->AddRectFilled(lineMinC, lineMaxC, ImColor(DisasmBpBgColor), 2.0f);

            // Render the address for this line
            //  (custom colored text so it is easy to see)
            ImGui::TextColored(DisasmBpColor, "%08X:", addr);
         } else {
            // Render the address for this line in normal color
            ImGui::Text("%08X:", addr);
         }
         if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered()) {
            toggleBreakpoint(addr);
         }
         linePos.x += addrAdvance;

         // Stop drawing if this is invalid memory
         if (!mem::valid(addr)) {
            continue;
         }

         auto instr = mem::read<espresso::Instruction>(addr);
         auto data = espresso::decodeInstruction(instr);
         auto info = analysis::get(addr);

         auto visInfoIter = visInstrInfo.find(addr);
         auto *visInfo = visInfoIter != visInstrInfo.end() ? &visInfoIter->second : nullptr;

         // Render the instructions bytes
         ImGui::SetCursorPos(linePos);
         ImGui::TextColored(DisasmDataColor, "%02x%02x%02x%02x",
            mem::read<unsigned char>(addr + 0),
            mem::read<unsigned char>(addr + 1),
            mem::read<unsigned char>(addr + 2),
            mem::read<unsigned char>(addr + 3));
         linePos.x += dataAdvance;

         if (info.func) {
            ImGui::SetCursorPos(linePos);
            if (addr == info.func->start) {
               ImGui::TextColored(DisasmFuncColor, u8"\u250f");
            } else if (info.func->end == 0xFFFFFFFF) {
               if (addr == info.func->start + 4) {
                  ImGui::TextColored(DisasmFuncColor, u8"\u2575");
                  ImGui::SetCursorPos(linePos);
                  ImGui::TextColored(DisasmFuncColor, u8"\u25BE");
               }
            } else if (addr == info.func->end - 4) {
               ImGui::TextColored(DisasmFuncColor, u8"\u2517");
            } else {
               ImGui::TextColored(DisasmFuncColor, u8"\u2503");
            }
         }
         linePos.x += funcLineAdvance;

         // This renders an arrow representing the direction of any branch statement
         if (data && isBranchInstr(data)) {
            auto meta = getBranchMeta(addr, instr, data, nullptr);
            if (!meta.isVariable && !meta.isCall) {
               ImGui::SetCursorPos(linePos);
               if (meta.target > addr) {
                  ImGui::TextColored(DisasmJmpColor, u8"\u25BE");
               } else {
                  ImGui::TextColored(DisasmJmpColor, u8"\u25B4");
               }
            }
         }
         linePos.x += jmpArrowAdvance;

         // This renders the brackets which display jump source/destinations
         //  for the currently selected instruction
         if (visInfo) {
            auto selectedGlyph = visInfo->branchGlyph;
            auto selectedColor = visInfo->branchColor;
            if (selectedGlyph != BranchGlyph::None) {
               // This drawing is a bit of a hack to be honest, but it looks nicer...
               ImGui::SetCursorPos(ImVec2(linePos.x - glyphWidth*0.25f, linePos.y));
               if (selectedGlyph == BranchGlyph::StartDown) {
                  ImGui::TextColored(selectedColor, u8"\u256D");
               } else if (selectedGlyph == BranchGlyph::StartUp) {
                  ImGui::TextColored(selectedColor, u8"\u2570");
               } else if (selectedGlyph == BranchGlyph::Middle) {
                  ImGui::TextColored(selectedColor, u8"\u2502");
               } else if (selectedGlyph == BranchGlyph::EndDown) {
                  ImGui::TextColored(selectedColor, u8"\u2570");
                  ImGui::SetCursorPos(ImVec2(linePos.x + glyphWidth*0.25f, linePos.y));
                  ImGui::TextColored(selectedColor, u8"\u25B8");
               } else if (selectedGlyph == BranchGlyph::EndUp) {
                  ImGui::TextColored(selectedColor, u8"\u256D");
                  ImGui::SetCursorPos(ImVec2(linePos.x + glyphWidth*0.25f, linePos.y));
                  ImGui::TextColored(selectedColor, u8"\u25B8");
               } else if (selectedGlyph == BranchGlyph::EndBoth) {
                  ImGui::TextColored(selectedColor, u8"\u251C");
               }
            }
         }
         linePos.x += jmpLineAdvance;

         ImGui::SetCursorPos(linePos);
         espresso::Disassembly dis;
         if (espresso::disassemble(instr, dis, addr)) {
            // TODO: Better integration with the disassembler,
            //  as well as providing per-arg highlighting?
            auto cmdVsArgs = dis.text.find(' ');
            if (cmdVsArgs != std::string::npos) {
               auto cmd = dis.text.substr(0, cmdVsArgs);
               auto args = dis.text.substr(cmdVsArgs + 1);
               ImGui::Text("%-6s %s", cmd.c_str(), args.c_str());
            } else {
               ImGui::Text("%-6s", dis.text.c_str());
            }
         } else {
            ImGui::Text("??");
         }
         linePos.x += instrAdvance;

         std::string lineInfo;
         if (info.func && addr == info.func->start && info.func->name.size() > 0) {
            lineInfo = info.func->name;
         }
         if (data && isBranchInstr(data)) {
            auto meta = getBranchMeta(addr, instr, data, nullptr);
            if (!meta.isVariable) {
               auto func = analysis::getFunction(meta.target);
               if (func) {
                  if (lineInfo.size() > 0) {
                     lineInfo += " - ";
                  }
                  lineInfo += "<" + func->name + ">";
               }
            }
         }
         if (info.instr && info.instr->comments.size() > 0) {
            if (lineInfo.size() > 0) {
               lineInfo += " - ";
            }
            lineInfo += info.instr->comments;
         }
         if (lineInfo.size() > 0) {
            ImGui::SetCursorPos(linePos);
            ImGui::Text("; %s", lineInfo.c_str());
         }
      }
      mScroller.End();

      ImGui::Separator();

      // Render the bottom bar for the window
      ImGui::AlignFirstTextHeightToWidgets();
      ImGui::Text("Go To Address: ");
      ImGui::SameLine();
      ImGui::PushItemWidth(70);
      if (ImGui::InputText("##addr", mAddressInput, 32, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue)) {
         std::istringstream is(mAddressInput);
         uint32_t goto_addr;
         if ((is >> std::hex >> goto_addr)) {
            gotoAddress(goto_addr);
         }
      }
      ImGui::PopItemWidth();

      ImGui::End();
   }

   void gotoAddress(uint32_t addr)
   {
      mScroller.ScrollTo(addr);
      mSelectedAddr = addr;
   }

   bool isAddressSet()
   {
      return mSelectedAddr != -1;
   }

private:
   AddressScroller mScroller;
   int64_t mSelectedAddr;
   char mAddressInput[32];

};

class RegistersView
{

public:
   RegistersView()
      : mLastActiveThread(nullptr), mLastResumeCount(0)
   {
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

      ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiSetCond_FirstUseEver);
      if (!ImGui::Begin("Registers", &isVisible)) {
         ImGui::End();
         return;
      }

      // The reason we store current/previous separately is because
      //  by the time resumeCount has been updated to indicate that we
      //  need to swap around the previous registers, the game is already
      //  resumed making it impossible to grab the registers.
      if (mLastResumeCount != sResumeCount || mLastActiveThread != sActiveThread) {
         mPreviousRegs = mCurrentRegs;
         mLastResumeCount = sResumeCount;
         mLastActiveThread = sActiveThread;
      }

      if (sActiveThread) {
         if (sActiveCore != -1) {
            mCurrentRegs = *getPausedCoreState(sActiveCore);
         } else {
            // Set everything to some error so its obvious if something is not restored.
            memset(&mCurrentRegs, 0xF1, sizeof(cpu::CoreRegs));

            // TODO: Kernel and Debugger should share OSContext reload code
            auto state = &mCurrentRegs;
            auto context = &sActiveThread->context;
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

      auto DrawRegCol = [](const std::string& name, const std::string& value, bool hasChanged) {
         ImGui::Text("%s", name.c_str());
         ImGui::NextColumn();
         if (sIsPaused) {
            if (!hasChanged) {
               ImGui::Text("%s", value.c_str());
            } else {
               ImGui::TextColored(RegsChangedColor, "%s", value.c_str());
            }
         }
         ImGui::NextColumn();
      };

      for (auto i = 0, j = 16; i < 16; i++, j++) {
         DrawRegCol(fmt::format("r{}", i),
            fmt::format("{:08x}", mCurrentRegs.gpr[i]),
            mCurrentRegs.gpr[i] != mPreviousRegs.gpr[i]);

         DrawRegCol(fmt::format("r{}", j),
            fmt::format("{:08x}", mCurrentRegs.gpr[j]),
            mCurrentRegs.gpr[j] != mPreviousRegs.gpr[j]);
      }

      ImGui::Separator();

      DrawRegCol("LR",
         fmt::format("{:08x}", mCurrentRegs.lr),
         mCurrentRegs.lr != mPreviousRegs.lr);
      DrawRegCol("CTR",
         fmt::format("{:08x}", mCurrentRegs.ctr),
         mCurrentRegs.ctr != mPreviousRegs.ctr);

      ImGui::Separator();

      ImGui::NextColumn();
      ImGui::Text("O Z + -"); ImGui::NextColumn();
      ImGui::NextColumn();
      ImGui::Text("O Z + -"); ImGui::NextColumn();

      auto DrawCrfCol = [](uint32_t crfNum, uint32_t val, bool hasChanged) {
         ImGui::Text("crf%d", crfNum);
         ImGui::NextColumn();
         if (sIsPaused) {
            if (!hasChanged) {
               ImGui::Text("%c %c %c %c",
                  (val & espresso::ConditionRegisterFlag::SummaryOverflow) ? 'X' : '_',
                  (val & espresso::ConditionRegisterFlag::Zero) ? 'X' : '_',
                  (val & espresso::ConditionRegisterFlag::Positive) ? 'X' : '_',
                  (val & espresso::ConditionRegisterFlag::Negative) ? 'X' : '_');
            } else {
               ImGui::TextColored(RegsChangedColor, "%c %c %c %c",
                  (val & espresso::ConditionRegisterFlag::SummaryOverflow) ? 'X' : '_',
                  (val & espresso::ConditionRegisterFlag::Zero) ? 'X' : '_',
                  (val & espresso::ConditionRegisterFlag::Positive) ? 'X' : '_',
                  (val & espresso::ConditionRegisterFlag::Negative) ? 'X' : '_');
            }
         }
         ImGui::NextColumn();
      };

      for (auto i = 0, j = 4; i < 4; i++, j++) {
         auto iVal = (mCurrentRegs.cr.value >> ((7-i) * 4)) & 0xF;
         auto jVal = (mCurrentRegs.cr.value >> ((7-j) * 4)) & 0xF;
         auto iPrevVal = (mPreviousRegs.cr.value >> ((7-i) * 4)) & 0xF;
         auto jPrevVal = (mPreviousRegs.cr.value >> ((7-j) * 4)) & 0xF;

         DrawCrfCol(i, iVal, iVal != iPrevVal);
         DrawCrfCol(j, jVal, jVal != jPrevVal);
      }


      ImGui::Columns(1);

      ImGui::End();
   }

protected:
   cpu::CoreRegs mCurrentRegs;
   cpu::CoreRegs mPreviousRegs;
   uint64_t mLastResumeCount;
   coreinit::OSThread *mLastActiveThread;

};

class StatsView
{
   static const size_t InstrCount = static_cast<size_t>(espresso::InstructionID::InstructionCount);

public:
   StatsView()
      : mFirstSeen(std::chrono::time_point<std::chrono::system_clock>::max())
   {
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

      ImGui::SetNextWindowSize(ImVec2(600, 300), ImGuiSetCond_FirstUseEver);
      if (!ImGui::Begin("Stats", &isVisible)) {
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
         if (mFirstSeen == std::chrono::time_point<std::chrono::system_clock>::max()) {
            mFirstSeen = std::chrono::system_clock::now();
            for (size_t i = 0; i < InstrCount; ++i) {
               mFirstSeenValues[i] = fallbackStats[i];
            }
         }
         mJitFallbackStats.clear();
         for (size_t i = 0; i < InstrCount; ++i) {
            mJitFallbackStats.emplace_back(i, fallbackStats[i]);
         }
         std::sort(mJitFallbackStats.begin(), mJitFallbackStats.end(),
            [](const std::pair<size_t, uint64_t> &a, const std::pair<size_t, uint64_t> &b) {
            return b.second < a.second;
         });
         auto timeDelta = std::chrono::system_clock::now() - mFirstSeen;
         using seconds_duration = std::chrono::duration<float, std::chrono::seconds::period>;
         auto secondsDelta = std::chrono::duration_cast<seconds_duration>(timeDelta).count();
         for (size_t i = 0; i < mJitFallbackStats.size(); ++i) {
            auto instrIdx = mJitFallbackStats[i].first;
            auto instrId = static_cast<espresso::InstructionID>(instrIdx);
            auto info = espresso::findInstructionInfo(instrId);
            if (!info) {
               continue;
            }

            auto instrOps = mJitFallbackStats[i].second;
            auto ips = static_cast<float>(instrOps - mFirstSeenValues[instrIdx]) / static_cast<float>(secondsDelta);

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

protected:
   std::vector<std::pair<size_t, uint64_t>> mJitFallbackStats;
   std::chrono::time_point<std::chrono::system_clock> mFirstSeen;
   uint64_t mFirstSeenValues[InstrCount];

};

static InfoView
sInfoView;

static MemoryMapView
sMemoryMapView;

static ThreadsView
sThreadsView;

static MemoryView
sMemoryView;

static DisassemblyView
sDisassemblyView;

static RegistersView
sRegistersView;

static StatsView
sStatsView;

void openAddrInMemoryView(uint32_t addr)
{
   sMemoryView.gotoAddress(addr);
   sMemoryView.activateFocus = true;
   sMemoryView.isVisible = true;
}

void openAddrInDisassemblyView(uint32_t addr)
{
   sDisassemblyView.gotoAddress(addr);
   sDisassemblyView.activateFocus = true;
   sDisassemblyView.isVisible = true;
}

void setActiveThread(coreinit::OSThread *thread)
{
   sActiveThread = thread;

   if (sActiveThread == coreinit::internal::getCoreRunningThread(0)) {
      sActiveCore = 0;
   } else if (sActiveThread == coreinit::internal::getCoreRunningThread(1)) {
      sActiveCore = 1;
   } else if (sActiveThread == coreinit::internal::getCoreRunningThread(2)) {
      sActiveCore = 2;
   } else {
      sActiveCore = -1;
   }

   if (sIsPaused && sActiveThread) {
      openAddrInDisassemblyView(getThreadNia(sActiveThread));
   }
}

void handleGamePaused()
{
   if (!sActiveThread && sActiveCore != -1) {
      // Lets first try to find the thread running on our core.
      sActiveThread = coreinit::internal::getCoreRunningThread(sActiveCore);
   }

   if (!sActiveThread) {
      // Now lets just try to find any running thread.
      sActiveThread = coreinit::internal::getCoreRunningThread(0);

      if (!sActiveThread) {
         sActiveThread = coreinit::internal::getCoreRunningThread(1);

         if (!sActiveThread) {
            sActiveThread = coreinit::internal::getCoreRunningThread(2);
         }
      }
   }

   if (!sActiveThread) {
      // Gezus... Pick the first one...
      sActiveThread = coreinit::internal::getFirstActiveThread();
   }

   setActiveThread(sActiveThread);
}

void handleGameResumed()
{
   sResumeCount++;
   sActiveThread = nullptr;
}

void draw()
{
   static auto firstActivation = true;
   static auto debugViewsVisible = false;

   if (!debugger::enabled()) {
      return;
   }

   auto &io = ImGui::GetIO();

   if (debugger::paused() && !sIsPaused) {
      // Just Paused
      sIsPaused = true;
      handleGamePaused();

      // Force the debugger to pop up
      debugViewsVisible = true;
   }

   if (io.KeyCtrl && ImGui::IsKeyPressed(static_cast<int>(decaf::input::KeyboardKey::D), false)) {
      debugViewsVisible = !debugViewsVisible;
   }

   // This is a stupid hack to avoid code duplation everywhere her...
   auto wantsPause = false;
   auto wantsResume = false;
   auto wantsStepOver = false;
   auto wantsStepInto = false;

   if (debugViewsVisible) {
      auto userModule = coreinit::internal::getUserModule();

      if (firstActivation && userModule) {
         // Place the views somewhere sane to start
         sMemoryView.gotoAddress(userModule->entryPoint);
         if (!sDisassemblyView.isAddressSet()) {
            sDisassemblyView.gotoAddress(userModule->entryPoint);
         }

         // Automatically analyse the primary user module
         for (auto &sec : userModule->sections) {
            if (sec.name.compare(".text") == 0) {
               analysis::analyse(sec.start, sec.end);
               break;
            }
         }

         firstActivation = false;
      }

      ImGui::BeginMainMenuBar();

      if (ImGui::BeginMenu("Debug")) {
         if (ImGui::MenuItem("Pause", nullptr, false, !sIsPaused)) {
            wantsPause = true;
         }

         if (ImGui::MenuItem("Resume", "F5", false, sIsPaused)) {
            wantsResume = true;
         }

         if (ImGui::MenuItem("Step Over", "F10", false, sIsPaused && sActiveCore != -1)) {
            wantsStepOver = true;
         }

         if (ImGui::MenuItem("Step Into", "F11", false, sIsPaused && sActiveCore != -1)) {
            wantsStepInto = true;
         }

         ImGui::Separator();

         if (ImGui::MenuItem("Kernel Trace Enabled", nullptr, decaf::config::log::kernel_trace, true)) {
            decaf::config::log::kernel_trace = !decaf::config::log::kernel_trace;
         }

         if (ImGui::MenuItem("GX2 Texture Dump Enabled", nullptr, decaf::config::gx2::dump_textures, true)) {
            decaf::config::gx2::dump_textures = !decaf::config::gx2::dump_textures;
         }

         if (ImGui::MenuItem("GX2 Shader Dump Enabled", nullptr, decaf::config::gx2::dump_shaders, true)) {
            decaf::config::gx2::dump_shaders = !decaf::config::gx2::dump_shaders;
         }

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

         if (ImGui::MenuItem("Disassembly", "CTRL+I", sDisassemblyView.isVisible, true)) {
            sDisassemblyView.isVisible = !sDisassemblyView.isVisible;
         }

         if (ImGui::MenuItem("Registers", "CTRL+R", sRegistersView.isVisible, true)) {
            sRegistersView.isVisible = !sRegistersView.isVisible;
         }

         if (ImGui::MenuItem("Stats", "CTRL+Q", sStatsView.isVisible, true)) {
            sStatsView.isVisible = !sStatsView.isVisible;
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

      if (io.KeyCtrl && ImGui::IsKeyPressed(static_cast<int>(decaf::input::KeyboardKey::I), false)) {
         sDisassemblyView.isVisible = !sDisassemblyView.isVisible;
      }

      if (io.KeyCtrl && ImGui::IsKeyPressed(static_cast<int>(decaf::input::KeyboardKey::R), false)) {
         sRegistersView.isVisible = !sRegistersView.isVisible;
      }

      if (io.KeyCtrl && ImGui::IsKeyPressed(static_cast<int>(decaf::input::KeyboardKey::Q), false)) {
         sStatsView.isVisible = !sStatsView.isVisible;
      }

      if (sIsPaused && ImGui::IsKeyPressed(static_cast<int>(decaf::input::KeyboardKey::F5), false)) {
         wantsResume = true;
      }

      if (sIsPaused && ImGui::IsKeyPressed(static_cast<int>(decaf::input::KeyboardKey::F10), true)) {
         wantsStepOver = true;
      }

      if (sIsPaused && ImGui::IsKeyPressed(static_cast<int>(decaf::input::KeyboardKey::F11), true)) {
         wantsStepInto = true;
      }

      if (wantsPause && !sIsPaused) {
         debugger::pauseAll();
      }

      if (wantsResume && sIsPaused) {
         debugger::resumeAll();
         sIsPaused = false;
         handleGameResumed();
      }

      if (wantsStepOver && sIsPaused && sActiveCore != -1) {
         debugger::stepCoreOver(sActiveCore);
         sIsPaused = false;
         handleGameResumed();
      }

      if (wantsStepInto && sIsPaused && sActiveCore != -1) {
         debugger::stepCoreInto(sActiveCore);
         sIsPaused = false;
         handleGameResumed();
      }

      sInfoView.draw();
      sMemoryMapView.draw();
      sThreadsView.draw();
      sMemoryView.draw();
      sDisassemblyView.draw();
      sRegistersView.draw();
      sStatsView.draw();
   }
}

} // namespace ui

} // namespace debugger
