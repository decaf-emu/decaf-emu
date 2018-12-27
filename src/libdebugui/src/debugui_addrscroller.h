#pragma once
#include <algorithm>
#include <cinttypes>
#include <cmath>
#include <cstdlib>
#include <imgui.h>

namespace debugui
{

class AddressScroller
{
public:
   AddressScroller() :
      mNumColumns(1),
      mScrollPos(0x00000000),
      mScrollToAddress(0xFFFFFFFF)
   {
   }

   void
   begin(int64_t numColumns,
         ImVec2 size)
   {
      auto flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
      ImGui::BeginChild("##scrolling", size, false, flags);
      ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2 { 0.0f, 0.0f });
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2 { 0.0f, 0.0f });

      // There is a differentiation between visible and rendered lines as if
      //  a line is half-visible, we don't want to count it as being a 'visible'
      //  line, on the other hand, we should render the half-line if we can.
      auto lineHeight = ImGui::GetTextLineHeight();
      auto clipHeight = ImGui::GetWindowHeight();
      auto numVisibleLines = static_cast<int64_t>(std::floor(clipHeight / lineHeight));
      auto numDrawLines = static_cast<int64_t>(std::ceil(clipHeight / lineHeight));

      if (ImGui::IsWindowHovered()) {
         auto &io = ImGui::GetIO();
         auto scrollDelta = -static_cast<int64_t>(io.MouseWheel);
         setScrollPos(mScrollPos + scrollDelta * numColumns);
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
            setScrollPos(mScrollToAddress - mNumColumns);
         } else {
            // Try to just scroll the view so that the user can keep track
            //  of the icon easier after scrolling...
            const auto minVisBound = mNumColumns;
            const auto maxVisBound = (mNumVisibleLines * mNumColumns) - mNumColumns - mNumColumns;

            if (mScrollToAddress < mScrollPos + minVisBound) {
               setScrollPos(mScrollToAddress - minVisBound);
            }

            if (mScrollToAddress >= mScrollPos + maxVisBound) {
               setScrollPos(mScrollToAddress - maxVisBound);
            }
         }

         mScrollToAddress = -1;
      }
   }

   void
   end()
   {
      ImGui::PopStyleVar(2);
      ImGui::EndChild();
   }

   uint32_t
   reset()
   {
      mIterPos = mScrollPos;
      return static_cast<uint32_t>(mIterPos);
   }

   uint32_t
   advance()
   {
      mIterPos += mNumColumns;
      return static_cast<uint32_t>(mIterPos);
   }

   bool
   hasMore() const
   {
      return mIterPos < mScrollPos + mNumDrawLines * mNumColumns && mIterPos < 0x100000000;
   }

   bool
   isValidOffset(uint32_t offset) const
   {
      return mIterPos + static_cast<int64_t>(offset) < 0x100000000;
   }

   void
   scrollTo(uint32_t address)
   {
      mScrollToAddress = address;
   }

private:
   void setScrollPos(int64_t position)
   {
      auto numTotalLines = (0x100000000 + mNumColumns - 1) / mNumColumns;

      // Make sure we stay within the bounds of our memory
      auto maxScrollPos = (numTotalLines - mNumVisibleLines) * mNumColumns;
      position = std::max<int64_t>(0, position);
      position = std::min<int64_t>(position, maxScrollPos);

      // Remap the scroll position to the closest line start
      position -= position % mNumColumns;
      mScrollPos = position;
   }

private:
   int64_t mNumColumns = 1;
   int64_t mScrollPos = 0;
   int64_t mIterPos = 0;
   int64_t mNumDrawLines = 0;
   int64_t mNumVisibleLines = 0;
   int64_t mScrollToAddress = 0xFFFFFFFF;
};

} // namespace debugui
