#include "debugui_window_stack.h"

#include <imgui.h>
#include <libcpu/mem.h>

namespace debugui
{

static const ImVec4 DataColor = HEXTOIMV4(0xB0C4CE, 1.0f);
static const ImVec4 FrameColor = HEXTOIMV4(0xAB47BC, 1.0f);
static const ImVec4 FrameExtColor = HEXTOIMV4(0xCFD8DC, 1.0f);
static const ImVec4 SpColor = HEXTOIMV4(0x000000, 1.0f);
static const ImVec4 SpBgColor = HEXTOIMV4(0x00E676, 1.0f);
static const ImVec4 SelBgColor = HEXTOIMV4(0x263238, 1.0f);

StackWindow::StackWindow(const std::string &name) :
   Window(name)
{
}

void
StackWindow::gotoAddress(uint32_t address)
{
   mAddressScroller.scrollTo(address);
   mSelectedAddr = address;
}

void
StackWindow::update()
{
   auto &activeThread = mStateTracker->getActiveThread();
   if (!activeThread.gpr[1] || !activeThread.stackStart) {
      return;
   }

   auto stackAddr = activeThread.gpr[1];
   auto stackStart = activeThread.stackStart;

   if (mStackFrameCacheAddr != stackAddr) {
      StackFrame frame;
      mStackFrames.clear();

      // Skip the initial LR space
      auto addr = stackAddr + 8;

      // Create the stack frame for the top piece
      while (true) {
         if (addr < stackAddr || !cpu::isValidAddress(cpu::VirtualAddress { addr })) {
            // If we somehow jump outside of our valid range, lets stop
            break;
         }

         frame.start = addr;
         frame.end = mem::read<uint32_t>(addr - 8) + 8;

         if (frame.end > stackStart) {
            // Stop when we go outside of the stack
            break;
         }

         mStackFrames.emplace(frame.start, frame);
         addr = frame.end;
      }

      mStackFrameCacheAddr = stackAddr;
   }
}

StackGlyph
StackWindow::getStackGlyph(uint32_t addr)
{
   auto frameIter = mStackFrames.lower_bound(addr);

   if (frameIter != mStackFrames.end()) {
      if (addr == frameIter->second.start) {
         return StackGlyph::Start;
      } else if (addr == frameIter->second.end - 4) {
         return StackGlyph::End;
      } else if (addr == frameIter->second.end - 8) {
         return StackGlyph::Backchain;
      } else if (addr == frameIter->second.end - 12) {
         return StackGlyph::DataEnd;
      } else if (addr >= frameIter->second.start && addr < frameIter->second.end) {
         return StackGlyph::DataMiddle;
      }
   }

   return StackGlyph::None;
}

void
StackWindow::draw()
{
   ImGui::SetNextWindowSize(ImVec2 { 300, 400 }, ImGuiSetCond_FirstUseEver);

   if (!ImGui::Begin(mName.c_str(), &mVisible)) {
      ImGui::End();
      return;
   }

   update();

   // Check if we need to move around or scroll or mark stuff
   if (mSelectedAddr != -1) {
      auto originalAddress = mSelectedAddr;

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
      if (!cpu::isValidAddress(cpu::VirtualAddress { static_cast<uint32_t>(mSelectedAddr) })) {
         mSelectedAddr = -1;
      }

      // Make sure that the address always stays visible!  We do this before
      //  checking for valid memory so you can still goto an invalid address.
      if (mSelectedAddr != originalAddress && mSelectedAddr != -1) {
         mAddressScroller.scrollTo(static_cast<uint32_t>(mSelectedAddr));
      }
   }

   // We use this 'hack' to get the true with without line-advance offsets.
   auto lineHeight = ImGui::GetTextLineHeight();
   auto glyphWidth = ImGui::CalcTextSize("FF").x - ImGui::CalcTextSize("F").x;

   // We precalcalculate
   auto addrAdvance = glyphWidth * 10.0f;
   auto dataAdvance = glyphWidth * 9.0f;
   auto frameLineAdvance = glyphWidth * 2.0f;

   // We grab some stuff to do some custom rendering...
   auto drawList = ImGui::GetWindowDrawList();
   auto wndWidth = ImGui::GetWindowContentRegionWidth();

   auto &activeThread = mStateTracker->getActiveThread();
   auto activeThreadStack = activeThread.gpr[1];
   mAddressScroller.begin(4, ImVec2 { 0, -ImGui::GetFrameHeightWithSpacing() });

   for (auto addr = mAddressScroller.reset(); mAddressScroller.hasMore(); addr = mAddressScroller.advance()) {
      auto rootPos = ImGui::GetCursorScreenPos();
      auto linePos = ImGui::GetCursorPos();
      auto lineMin = ImVec2 { rootPos.x, rootPos.y };
      auto lineMax = ImVec2 { rootPos.x + wndWidth, rootPos.y + lineHeight };

      // Handle a new address being selected
      if (ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(lineMin, lineMax)) {
         if (ImGui::IsMouseClicked(0) || ImGui::IsMouseDown(0)) {
            mSelectedAddr = addr;
         }
         // Maybe render a 'highlight' here?
      }

      // Draw the rectangle for the current line selection
      if (static_cast<int64_t>(addr) == mSelectedAddr) {
         auto lineMinB = ImVec2 { lineMin.x - 1, lineMin.y };
         auto lineMaxB = ImVec2 { lineMax.x + 1, lineMax.y };
         drawList->AddRectFilled(lineMinB, lineMaxB, ImColor { SelBgColor }, 2.0f);
      }

      // Check if this is the current instruction
      // This should be simpler...  gActiveCoreIdx instead maybe?
      if (addr == activeThread.gpr[1]) {
         // Render a green BG behind the address
         auto lineMinC = ImVec2 { lineMin.x - 1, lineMin.y };
         auto lineMaxC = ImVec2 { lineMin.x + (glyphWidth * 8) + 2, lineMax.y };
         drawList->AddRectFilled(lineMinC, lineMaxC, ImColor { SpBgColor }, 2.0f);

         // Render the address for this line
         //  (custom colored text so it is easy to see)
         ImGui::TextColored(SpColor, "%08X:", addr);
      } else {
         // Render the address for this line in normal color
         ImGui::Text("%08X:", addr);
      }
      linePos.x += addrAdvance;

      // Stop drawing if this is outside the threads stack range
      if (activeThread.gpr[1] && activeThread.stackStart) {
         // Make sure we are inside the stack first
         if (activeThread.gpr[1] >= activeThread.stackEnd &&
             activeThread.gpr[1] < activeThread.stackStart) {
            if (addr < activeThread.stackEnd || addr >= activeThread.stackStart) {
               continue;
            }
         }
      }

      // Stop drawing if this is invalid memory
      if (!cpu::isValidAddress(cpu::VirtualAddress { addr })) {
         continue;
      }

      // Render the instructions bytes
      ImGui::SetCursorPos(linePos);
      ImGui::TextColored(DataColor, "%02x%02x%02x%02x",
                         mem::read<unsigned char>(addr + 0),
                         mem::read<unsigned char>(addr + 1),
                         mem::read<unsigned char>(addr + 2),
                         mem::read<unsigned char>(addr + 3));
      linePos.x += dataAdvance;

      auto glyph = getStackGlyph(addr);
      if (glyph != StackGlyph::None) {
         ImGui::SetCursorPos(linePos);
         if (glyph == StackGlyph::Start) {
            ImGui::TextColored(FrameColor, u8"\u250f");
         } else if (glyph == StackGlyph::DataMiddle) {
            ImGui::TextColored(FrameColor, u8"\u2503");
         } else if (glyph == StackGlyph::DataEnd) {
            ImGui::TextColored(FrameColor, u8"\u2521");
         } else if (glyph == StackGlyph::Backchain) {
            ImGui::TextColored(FrameExtColor, u8"\u2502");
         } else if (glyph == StackGlyph::End) {
            ImGui::TextColored(FrameExtColor, u8"\u2515");
         }
      }
      linePos.x += frameLineAdvance;

      // Find symbol for value in stack
      static std::array<char, 256> symbolNameBuffer;
      static std::array<char, 256> moduleNameBuffer;

      auto data = mem::read<uint32_t>(addr);
      if (!data || data > 0x10000000) {
         continue;
      }

      auto symbolDistance = uint32_t { 0 };
      auto error =
         decaf::debug::findClosestSymbol(
            data,
            &symbolDistance,
            symbolNameBuffer.data(),
            static_cast<uint32_t>(symbolNameBuffer.size()),
            moduleNameBuffer.data(),
            static_cast<uint32_t>(moduleNameBuffer.size()));

      if (!error && moduleNameBuffer[0] && symbolNameBuffer[0]) {
         ImGui::SetCursorPos(linePos);
         ImGui::Text("<%s|%s+0x%X>",
                     symbolNameBuffer.data(),
                     moduleNameBuffer.data(),
                     symbolDistance);
      }
   }
   mAddressScroller.end();

   ImGui::Separator();

   // Render the bottom bar for the window
   ImGui::AlignTextToFramePadding();
   ImGui::Text("Showing stack from start %08x to end %08x",
               activeThread.stackStart, activeThread.stackEnd);
   ImGui::End();
}

} // namespace debugui
