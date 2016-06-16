#include "common/emuassert.h"
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
#include <imgui.h>
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
static const ImVec4 DisasmDataColor = HEXTOIMV4(0x90A4AE, 1.0f);
static const ImVec4 DisasmFuncLinkColor = HEXTOIMV4(0x7E57C2, 1.0f);
static const ImVec4 DisasmJmpColor = HEXTOIMV4(0xFF5722, 1.0f);
static const ImVec4 DisasmNiaColor = HEXTOIMV4(0x00E676, 1.0f);

// We store this locally so that we do not end up with isRunning
//  switching whilst we are in the midst of drawing the UI.
static bool
sIsPaused = false;

static uint32_t
sActiveCore = 1;

static coreinit::OSThread *
sActiveThread = nullptr;

void openAddrInMemoryView(uint32_t addr);
void openAddrInDisassemblyView(uint32_t addr);
void setActiveThread(coreinit::OSThread *thread);

uint32_t getThreadNia(coreinit::OSThread *thread)
{
   emuassert(sIsPaused);

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
      return mem::read<uint32_t>(thread->context.gpr[1]);
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

      mSegments.push_back(Segment{ "SystemData", mem::SystemBase, mem::SystemEnd });
      mSegments.push_back(Segment{ "Application", mem::ApplicationBase, mem::ApplicationEnd });
      mSegments.push_back(Segment{ "Apertures", mem::AperturesBase, mem::AperturesEnd });
      mSegments.push_back(Segment{ "Foreground", mem::ForegroundBase, mem::ForegroundEnd });
      mSegments.push_back(Segment{ "MEM1", mem::MEM1Base, mem::MEM1End });
      mSegments.push_back(Segment{ "LockedCache", mem::LockedCacheBase, mem::LockedCacheEnd });
      mSegments.push_back(Segment{ "SharedData", mem::SharedDataBase, mem::SharedDataEnd });

      coreinit::internal::lockLoader();
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
   struct ThreadInfo
   {
      coreinit::OSThread *thread;
      uint32_t id;
      std::string name;
      coreinit::OSThreadState state;
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
      auto firstThread = coreinit::internal::getFirstActiveThread();

      for (auto thread = firstThread; thread; thread = thread->activeLink.next) {
         ThreadInfo tinfo;
         tinfo.thread = thread;
         tinfo.id = thread->id;
         tinfo.name = thread->name ? thread->name.get() : "";
         tinfo.state = thread->state;

         if (thread == core0Thread) {
            tinfo.coreId = 0;
         } else if (thread == core1Thread) {
            tinfo.coreId = 1;
         } else if (thread == core2Thread) {
            tinfo.coreId = 2;
         } else {
            tinfo.coreId = -1;
         }

         mThreads.push_back(tinfo);
      }

      coreinit::internal::unlockScheduler();

      ImGui::Columns(5, "threadList", false);
      ImGui::SetColumnOffset(0, ImGui::GetWindowWidth() * 0.0f);
      ImGui::SetColumnOffset(1, ImGui::GetWindowWidth() * 0.1f);
      ImGui::SetColumnOffset(2, ImGui::GetWindowWidth() * 0.6f);
      ImGui::SetColumnOffset(3, ImGui::GetWindowWidth() * 0.75f);
      ImGui::SetColumnOffset(4, ImGui::GetWindowWidth() * 0.9f);

      ImGui::Text("ID"); ImGui::NextColumn();
      ImGui::Text("Name"); ImGui::NextColumn();
      ImGui::Text("NIA"); ImGui::NextColumn();
      ImGui::Text("State"); ImGui::NextColumn();
      ImGui::Text("Core#"); ImGui::NextColumn();
      ImGui::Separator();

      for (auto &thread : mThreads) {
         ImGui::Text(fmt::format("{}", thread.id).c_str()); ImGui::NextColumn();
         if (sIsPaused) {
            if (ImGui::Selectable(thread.name.c_str())) {
               setActiveThread(thread.thread);
            }
         } else {
            ImGui::Text(thread.name.c_str());
         }
         ImGui::NextColumn();

         if (sIsPaused) {
            ImGui::Text(fmt::format("{:08x}", getThreadNia(thread.thread)).c_str());
         } else {
            ImGui::Text("        ");
         }

         ImGui::NextColumn();
         ImGui::Text(coreinit::enumAsString(thread.state).c_str());  ImGui::NextColumn();
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
      ImGui::Text("Showing %d Columns", numColumns);

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

      // We use this 'hack' to get the true with without line-advance offsets.
      float glyphWidth = ImGui::CalcTextSize("FF").x - ImGui::CalcTextSize("F").x;

      // We precalcalculate
      float addrAdvance = glyphWidth * 10.0f;
      float dataAdvance = glyphWidth * 10.0f;
      float funcLineAdvance = glyphWidth * 1.0f;
      float jmpLineAdvance = glyphWidth * 1.0f;
      float instrAdvance = glyphWidth * 6.0f;

      mScroller.Begin(4, ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()));
      for (auto addr = mScroller.Reset(); mScroller.HasMore(); addr = mScroller.Advance()) {
         auto linePos = ImGui::GetCursorPos();

         // Render the address for this line
         ImGui::Text("%08X:", addr);
         linePos.x += addrAdvance;

         // Stop drawing if this is invalid memory
         if (!mem::valid(addr)) {
            continue;
         }

         // Render the instructions bytes
         ImGui::SetCursorPos(linePos);
         ImGui::TextColored(DisasmDataColor, "%02x%02x%02x%02x",
            mem::read<unsigned char>(addr + 0),
            mem::read<unsigned char>(addr + 1),
            mem::read<unsigned char>(addr + 2),
            mem::read<unsigned char>(addr + 3));
         linePos.x += dataAdvance;

         // TODO: Render the function bounds determined during
         //  analysis here with DisasmFuncLinkColor
         //   Function Start Glyph: u8"\x250f"
         //   Function Continue Glyph: u8"\x2503"
         linePos.x += funcLineAdvance;

         // This should be simpler...  gActiveCoreIdx instead maybe?
         ImGui::SetCursorPos(linePos);
         if (sActiveThread && addr == getThreadNia(sActiveThread)) {
            // We should show the direction of the jump in NIA color when we are on that jump
            ImGui::TextColored(DisasmNiaColor, u8"\x25B6");
         } else {
           // TODO: Check if this is a jump and put an arrow,
           //  DisasmJmpColor can be used for this.
           //   Up Glyph: u8"\x25B4"
           //   Down Glyph: u8"\x25BE"
         }
         linePos.x += jmpLineAdvance;

         ImGui::SetCursorPos(linePos);
         auto instr = mem::read<espresso::Instruction>(addr);
         espresso::Disassembly dis;
         if (espresso::disassemble(instr, dis, addr)) {
            // TODO: Better integration with the disassembler,
            //  as well as providing per-arg highlighting?
            auto cmdVsArgs = dis.text.find(' ');
            if (cmdVsArgs != std::string::npos) {
               auto cmd = dis.text.substr(0, cmdVsArgs);
               auto args = dis.text.substr(cmdVsArgs + 1);
               ImGui::Text("%- 6s %s", cmd.c_str(), args.c_str());
            } else {
               ImGui::Text("%- 6s", dis.text.c_str());
            }
         } else {
            ImGui::Text("??");
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
   }

private:
   AddressScroller mScroller;
   char mAddressInput[32];

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
   emuassert(sIsPaused);

   coreinit::OSThread *coreThread[3] = {
      coreinit::internal::getCoreRunningThread(0),
      coreinit::internal::getCoreRunningThread(1),
      coreinit::internal::getCoreRunningThread(2)
   };

   sActiveThread = thread;

   if (sActiveThread == coreThread[0]) {
      sActiveCore = 0;
   } else if (sActiveThread == coreThread[1]) {
      sActiveCore = 1;
   } else if (sActiveThread == coreThread[2]) {
      sActiveCore = 2;
   } else {
      sActiveCore = -1;
   }

   if (sActiveThread) {
      openAddrInDisassemblyView(getThreadNia(sActiveThread));
   }
}

void handleGamePaused()
{
   coreinit::OSThread *firstActiveThread =
      coreinit::internal::getFirstActiveThread();
   coreinit::OSThread *coreThread[3] = {
      coreinit::internal::getCoreRunningThread(0),
      coreinit::internal::getCoreRunningThread(1),
      coreinit::internal::getCoreRunningThread(2)
   };

   if (!sActiveThread) {
      // Lets first try to find the thread running on our core.
      if (coreThread[sActiveCore]) {
         sActiveThread = coreThread[sActiveCore];
      }
   }
   if (!sActiveThread) {
      // Now lets just try to find any running thread.
      sActiveThread = coreThread[0];
      if (!sActiveThread) {
         sActiveThread = coreThread[1];
         if (!sActiveThread) {
            sActiveThread = coreThread[2];
         }
      }
   }
   if (!sActiveThread) {
      // Gezus... Pick the first one...
      sActiveThread = firstActiveThread;
   }

   setActiveThread(sActiveThread);
}

void handleGameResumed()
{
   sActiveThread = nullptr;
}

void draw()
{
   static bool firstActivation = true;
   static bool debugViewsVisible = false;

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
   bool wantsPause = false;
   bool wantsResume = false;
   bool wantsStepOver = false;
   bool wantsStepInto = false;

   if (debugViewsVisible) {
      auto userModule = coreinit::internal::getUserModule();
      if (firstActivation && userModule) {
         // Place the views somewhere sane to start
         sMemoryView.gotoAddress(userModule->entryPoint);
         sDisassemblyView.gotoAddress(userModule->entryPoint);

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
         if (ImGui::MenuItem("Disassembly", "CTRL+I", sDisassemblyView.isVisible, true)) {
            sDisassemblyView.isVisible = !sDisassemblyView.isVisible;
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
   }
}

} // namespace ui

} // namespace debugger
