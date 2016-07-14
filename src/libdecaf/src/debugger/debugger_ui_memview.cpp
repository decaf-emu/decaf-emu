#include "debugger_ui_internal.h"
#include "decaf_config.h"
#include "imgui_addrscroll.h"
#include <imgui.h>
#include <sstream>

namespace debugger
{

namespace ui
{

namespace MemView
{

static bool
sActivateFocus = false;

static AddressScroller
sScroller;

static bool
sEditingEnabled = true;

static int64_t
sEditAddress = -1;

static int64_t
sLastEditAddress = -1;

static int64_t
sGotoTargetAddr = -1;

static char
sAddressInput[32] = { 0 };

static char
sDataInput[32] = { 0 };

static void
gotoAddress(uint32_t address)
{
   sScroller.ScrollTo(address);
   sEditAddress = address;
}

void
displayAddress(uint32_t address)
{
   gotoAddress(address);
   sActivateFocus = true;
   decaf::config::debugger::show_mem_view = true;
}

void
draw()
{
   if (!decaf::config::debugger::show_mem_view) {
      return;
   }

   if (sActivateFocus) {
      ImGui::SetNextWindowFocus();
      sActivateFocus = false;
   }

   if (!ImGui::Begin("Memory", &decaf::config::debugger::show_mem_view)) {
      ImGui::End();
      return;
   }

   // We use this 'hack' to get the true with without line-advance offsets.
   auto glyphWidth = ImGui::CalcTextSize("FF").x - ImGui::CalcTextSize("F").x;
   auto cellWidth = glyphWidth * 3.0f;

   // We precalcalculate these so we can reverse engineer our column
   //  count and line render locations.
   auto addrAdvance = glyphWidth * 10.0f;
   auto cellAdvance = glyphWidth * 2.5f;
   auto gapAdvance = glyphWidth * 1.0f;
   auto charAdvance = glyphWidth * 1.0f;
   auto wndDynRegionSize = ImGui::GetWindowContentRegionWidth() - addrAdvance - gapAdvance;
   auto numColumns = static_cast<int64_t>(wndDynRegionSize / (cellAdvance + charAdvance));

   // Impose a limit on the number of columns in case the window
   //  does not yet have a size.
   numColumns = std::max<int64_t>(1, std::min<int64_t>(numColumns, 64));

   // Clear the last edit address whenever our current address is cleared
   if (sEditAddress == -1) {
      sLastEditAddress = -1;
   }

   // Check if we need to move around or scroll
   if (sEditAddress != -1)
   {
      auto originalAddress = sEditAddress;

      // Check if the user wants to move
      if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_UpArrow))) {
         sEditAddress -= numColumns;
      } else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_DownArrow))) {
         sEditAddress += numColumns;
      } else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_LeftArrow))) {
         sEditAddress--;
      } else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_RightArrow))) {
         sEditAddress++;
      }

      // Clamp the edit address
      sEditAddress = std::max<int64_t>(0, sEditAddress);
      sEditAddress = std::min<int64_t>(sEditAddress, 0xFFFFFFFF);

      // Before we start processing an edit, lets make sure it's valid memory to be editing...
      if (!mem::valid(static_cast<uint32_t>(sEditAddress))) {
         sEditAddress = -1;
      }

      // Make sure that the address always stays visible!  We do this before
      //  checking for valid memory so you can still goto an invalid address.
      if (sEditAddress != originalAddress && sEditAddress != -1) {
         sScroller.ScrollTo(static_cast<uint32_t>(sEditAddress));
      }
   }

   auto editAddress = sEditAddress;
   sScroller.Begin(numColumns, ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()));

   for (auto addr = sScroller.Reset(); sScroller.HasMore(); addr = sScroller.Advance()) {
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
            if (sLastEditAddress != editAddress) {
               ImGui::SetKeyboardFocusHere();

               uint32_t targetAddress = static_cast<uint32_t>(editAddress);
               snprintf(sAddressInput, 32, "%08X", targetAddress);
               snprintf(sDataInput, 32, "%02X", mem::read<unsigned char>(targetAddress));

               sLastEditAddress = editAddress;
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

            auto getCursorPos =
               [](auto data) {
                  auto cursorPosPtr = static_cast<int*>(data->UserData);

                  if (!data->HasSelection()) {
                     *cursorPosPtr = data->CursorPos;
                  }

                  return 0;
               };

            if (ImGui::InputText("##data", sDataInput, 32, flags, getCursorPos, &cursorPos)) {
               sEditAddress += 1;
            } else if (!newlyFocused && !ImGui::IsItemActive()) {
               sEditAddress = -1;
            }

            if (cursorPos >= 2) {
               sEditAddress += 1;
            }

            ImGui::PopItemWidth();
            ImGui::PopID();

            if (sEditAddress != sLastEditAddress) {
               // If the edit address has changed, we need to update
               //  the memory itself.
               std::istringstream is(sDataInput);
               int data;

               if (is >> std::hex >> data) {
                  mem::write(addr + i, static_cast<unsigned char>(data));
               }
            }
         } else {
            ImGui::SetCursorPos(linePos);

            if (sScroller.IsValidOffset(i) && mem::valid(addr + i)) {
               ImGui::Text("%02X ", mem::read<unsigned char>(addr + i));

               if (sEditingEnabled && ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
                  sEditAddress = addr + i;
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
      for (auto i = 0u; i < numColumns; ++i) {
         if (sScroller.IsValidOffset(i) && mem::valid(addr + i)) {
            ImGui::SetCursorPos(linePos);
            unsigned char c = mem::read<unsigned char>(addr + i);
            ImGui::Text("%c", (c >= 32 && c < 128) ? c : '.');
         }

         linePos.x += charAdvance;
      }
   }

   sScroller.End();
   ImGui::Separator();

   // Render the bottom bar for the window
   ImGui::AlignFirstTextHeightToWidgets();
   ImGui::Text("Go To Address: ");
   ImGui::SameLine();
   ImGui::PushItemWidth(70);

   if (ImGui::InputText("##addr", sAddressInput, 32, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue)) {
      std::istringstream is(sAddressInput);
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

} // namespace MemView

} // namespace ui

} // namespace debugger
