#include "debugui_window_memory.h"

#include <algorithm>
#include <imgui.h>
#include <libcpu/mem.h>
#include <libcpu/mmu.h>
#include <libdecaf/src/cafe/kernel/cafe_kernel_mmu.h>
#include <sstream>

namespace debugui
{

MemoryWindow::MemoryWindow(const std::string &name) :
   Window(name)
{
   mAddressScroller.scrollTo(cafe::kernel::getVirtualMemoryMap(cafe::kernel::VirtualMemoryRegion::AppData).vaddr.getAddress());
}

void
MemoryWindow::gotoAddress(uint32_t address)
{
   mAddressScroller.scrollTo(address);
   mEditAddress = address;
}

void
MemoryWindow::draw()
{
   char addressInput[32] = { 0 };
   char dataInput[32] = { 0 };

   if (!ImGui::Begin(mName.c_str(), &mVisible, ImGuiWindowFlags_NoScrollbar)) {
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
   if (mEditAddress == -1) {
      mLastEditAddress = -1;
   }

   // Check if we need to move around or scroll
   if (mEditAddress != -1) {
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
      if (!cpu::isValidAddress(cpu::VirtualAddress { static_cast<uint32_t>(mEditAddress) })) {
         mEditAddress = -1;
      }

      // Make sure that the address always stays visible!  We do this before
      //  checking for valid memory so you can still goto an invalid address.
      if (mEditAddress != originalAddress && mEditAddress != -1) {
         mAddressScroller.scrollTo(static_cast<uint32_t>(mEditAddress));
      }
   }

   auto editAddress = mEditAddress;
   mAddressScroller.begin(numColumns, ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));

   for (auto addr = mAddressScroller.reset(); mAddressScroller.hasMore(); addr = mAddressScroller.advance()) {
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
               snprintf(addressInput, 32, "%08X", targetAddress);
               snprintf(dataInput, 32, "%02X", mem::read<unsigned char>(targetAddress));

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

            auto getCursorPos =
               [](auto data) {
               auto cursorPosPtr = static_cast<int*>(data->UserData);

               if (!data->HasSelection()) {
                  *cursorPosPtr = data->CursorPos;
               }

               return 0;
            };

            if (ImGui::InputText("##data", dataInput, 32, flags, getCursorPos, &cursorPos)) {
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
               std::istringstream is { dataInput };
               int data;

               if (is >> std::hex >> data) {
                  mem::write(addr + i, static_cast<unsigned char>(data));
               }
            }
         } else {
            ImGui::SetCursorPos(linePos);

            if (mAddressScroller.isValidOffset(i) && cpu::isValidAddress(cpu::VirtualAddress { addr + i })) {
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
      for (auto i = 0u; i < numColumns; ++i) {
         if (mAddressScroller.isValidOffset(i) && cpu::isValidAddress(cpu::VirtualAddress { addr + i })) {
            ImGui::SetCursorPos(linePos);
            unsigned char c = mem::read<unsigned char>(addr + i);
            ImGui::Text("%c", (c >= 32 && c < 128) ? c : '.');
         }

         linePos.x += charAdvance;
      }
   }

   mAddressScroller.end();
   ImGui::Separator();

   // Render the bottom bar for the window
   ImGui::AlignTextToFramePadding();
   ImGui::Text("Go To Address: ");
   ImGui::SameLine();
   ImGui::PushItemWidth(70);

   if (ImGui::InputText("##addr", addressInput, 32, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue)) {
      std::istringstream is { addressInput };
      uint32_t goto_addr;

      if ((is >> std::hex >> goto_addr)) {
         gotoAddress(goto_addr);
      }
   }

   ImGui::PopItemWidth();
   ImGui::SameLine();
   ImGui::Text("Showing %d Columns", static_cast<int>(numColumns));
   ImGui::End();
}

} // namespace debugui
