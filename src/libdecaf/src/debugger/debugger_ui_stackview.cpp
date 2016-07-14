#include "debugger_ui_internal.h"
#include "imgui_addrscroll.h"
#include "kernel/kernel_loader.h"
#include <imgui.h>
#include <map>

namespace debugger
{

namespace ui
{

namespace StackView
{

static const ImVec4 DataColor = HEXTOIMV4(0xB0C4CE, 1.0f);
static const ImVec4 FrameColor = HEXTOIMV4(0xAB47BC, 1.0f);
static const ImVec4 FrameExtColor = HEXTOIMV4(0xCFD8DC, 1.0f);
static const ImVec4 SpColor = HEXTOIMV4(0x000000, 1.0f);
static const ImVec4 SpBgColor = HEXTOIMV4(0x00E676, 1.0f);
static const ImVec4 SelBgColor = HEXTOIMV4(0x263238, 1.0f);

enum class StackGlyph : uint32_t {
   None,
   Start,
   DataMiddle,
   DataEnd,
   Backchain,
   End
};

struct StackFrame
{
   uint32_t start;
   uint32_t end;
};

bool
gIsVisible = true;

static bool
sActivateFocus = false;

static AddressScroller
sScroller;

static int64_t
sSelectedAddr = -1;

static uint32_t
sStackFrameCacheAddr = -1;

static std::map<uint32_t, StackFrame, std::greater<uint32_t>>
sStackFrames;

static void
gotoAddress(uint32_t address)
{
   sScroller.ScrollTo(address);
   sSelectedAddr = address;
}

void
displayAddress(uint32_t address)
{
   gotoAddress(address);
   sActivateFocus = true;
   gIsVisible = true;
}

static void
updateFrameCache(coreinit::OSThread *thread, uint32_t sp, uint32_t stackStart)
{
   StackFrame frame;

   // Reset the cache
   sStackFrames.clear();

   // Skip the initial LR space
   uint32_t addr = sp + 8;

   // Create the stack frame for the top piece
   while (true) {
      if (addr < sp) {
         // If we somehow jump outside of our valid range, lets stop
         break;
      }
      frame.start = addr;
      frame.end = mem::read<uint32_t>(addr - 8) + 8;
      if (frame.end > stackStart) {
         // Stop when we go outside of the stack
         break;
      }
      sStackFrames.emplace(frame.start, frame);
      addr = frame.end;
   }

}

static StackGlyph
getStackGlyph(uint32_t addr)
{
   auto frameIter = sStackFrames.lower_bound(addr);
   if (frameIter != sStackFrames.end()) {
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
draw()
{
   if (!gIsVisible) {
      return;
   }

   if (sActivateFocus) {
      ImGui::SetNextWindowFocus();
      sActivateFocus = false;
   }

   std::string windowKey = "Stack";
   if (!ImGui::Begin(windowKey.c_str(), &gIsVisible)) {
      ImGui::End();
      return;
   }

   auto activeThread = getActiveThread();

   if (activeThread) {
      auto activeSp = getThreadStack(activeThread);
      auto stackStart = activeThread->stackStart.getAddress();
      if (sStackFrameCacheAddr != activeSp) {
         updateFrameCache(activeThread, activeSp, stackStart);
         sStackFrameCacheAddr = activeSp;
      }
   }

   // Check if we need to move around or scroll or mark stuff
   if (sSelectedAddr != -1)
   {
      auto originalAddress = sSelectedAddr;

      // Check if the user wants to move
      if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_UpArrow))) {
         sSelectedAddr -= 4;
      } else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_DownArrow))) {
         sSelectedAddr += 4;
      }

      // Clamp the edit address
      sSelectedAddr = std::max<int64_t>(0, sSelectedAddr);
      sSelectedAddr = std::min<int64_t>(sSelectedAddr, 0xFFFFFFFF);

      // Before we start processing an edit, lets make sure it's valid memory to be editing...
      if (!mem::valid(static_cast<uint32_t>(sSelectedAddr))) {
         sSelectedAddr = -1;
      }

      // Make sure that the address always stays visible!  We do this before
      //  checking for valid memory so you can still goto an invalid address.
      if (sSelectedAddr != originalAddress && sSelectedAddr != -1) {
         sScroller.ScrollTo(static_cast<uint32_t>(sSelectedAddr));
      }
   }

   // We use this 'hack' to get the true with without line-advance offsets.
   auto lineHeight = ImGui::GetTextLineHeight();
   float glyphWidth = ImGui::CalcTextSize("FF").x - ImGui::CalcTextSize("F").x;

   // We precalcalculate
   float addrAdvance = glyphWidth * 10.0f;
   float dataAdvance = glyphWidth * 9.0f;
   float frameLineAdvance = glyphWidth * 2.0f;

   // We grab some stuff to do some custom rendering...
   auto drawList = ImGui::GetWindowDrawList();
   auto wndWidth = ImGui::GetWindowContentRegionWidth();

   sScroller.Begin(4, ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()));

   for (auto addr = sScroller.Reset(); sScroller.HasMore(); addr = sScroller.Advance()) {
      auto rootPos = ImGui::GetCursorScreenPos();
      auto linePos = ImGui::GetCursorPos();

      auto lineMin = ImVec2(rootPos.x, rootPos.y);
      auto lineMax = ImVec2(rootPos.x + wndWidth, rootPos.y + lineHeight);

      // Handle a new address being selected
      if (ImGui::IsMouseHoveringRect(lineMin, lineMax)) {
         if (ImGui::IsMouseClicked(0) || ImGui::IsMouseDown(0)) {
            sSelectedAddr = addr;
         }
         // Maybe render a 'highlight' here?
      }

      // Draw the rectangle for the current line selection
      if (static_cast<int64_t>(addr) == sSelectedAddr) {
         auto lineMinB = ImVec2(lineMin.x - 1, lineMin.y);
         auto lineMaxB = ImVec2(lineMax.x + 1, lineMax.y);
         drawList->AddRectFilled(lineMinB, lineMaxB, ImColor(SelBgColor), 2.0f);
      }

      // Check if this is the current instruction
      // This should be simpler...  gActiveCoreIdx instead maybe?
      if (activeThread && addr == getThreadStack(activeThread)) {
         // Render a green BG behind the address
         auto lineMinC = ImVec2(lineMin.x - 1, lineMin.y);
         auto lineMaxC = ImVec2(lineMin.x + (glyphWidth * 8) + 2, lineMax.y);
         drawList->AddRectFilled(lineMinC, lineMaxC, ImColor(SpBgColor), 2.0f);

         // Render the address for this line
         //  (custom colored text so it is easy to see)
         ImGui::TextColored(SpColor, "%08X:", addr);
      } else {
         // Render the address for this line in normal color
         ImGui::Text("%08X:", addr);
      }
      linePos.x += addrAdvance;

      // Stop drawing if this is outside the threads stack range
      if (activeThread) {
         auto activeSp = getThreadStack(activeThread);
         auto stackStart = activeThread->stackStart.getAddress();
         auto stackEnd = activeThread->stackEnd.getAddress();
         // Make sure we are inside the stack first
         if (activeSp >= stackEnd && activeSp < stackStart) {
            if (addr < stackEnd || addr >= stackStart) {
               continue;
            }
         }
      }

      // Stop drawing if this is invalid memory
      if (!mem::valid(addr)) {
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

      // Check if this is a symbol?
      auto data = mem::read<uint32_t>(addr);
      auto sec = kernel::loader::findSectionForAddress(data);
      if (sec && sec->type == kernel::loader::LoadedSectionType::Code) {
         auto symInfo = kernel::loader::findNearestSymbolNameForAddress(data);
         ImGui::SetCursorPos(linePos);
         ImGui::Text("<%s>", symInfo.c_str());
      }
   }
   sScroller.End();

   ImGui::Separator();

   // Render the bottom bar for the window
   ImGui::AlignFirstTextHeightToWidgets();
   if (activeThread) {
      auto stackStart = activeThread->stackStart.getAddress();
      auto stackEnd = activeThread->stackEnd.getAddress();
      ImGui::Text("Showing stack from start %08x to end %08x", stackStart, stackEnd);
   } else {
      ImGui::Text("Showing stack from start ???????? to end ????????");
   }


   ImGui::End();
}

} // namespace StackView

} // namespace ui

} // namespace debugger
