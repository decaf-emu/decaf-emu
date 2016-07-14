#pragma once
#include <algorithm>
#include <cinttypes>
#include <cmath>
#include <imgui.h>

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
