#include "gpu7_displaylayout.h"
#include "gpu_config.h"

#include <algorithm>

namespace gpu7
{

void
updateDisplayLayout(DisplayLayout &layout,
                    float windowWidth,
                    float windowHeight)
{
   // Sizes for calculating aspect ratio
   constexpr auto TvWidth = 1280.0f;
   constexpr auto TvHeight = 720.0f;
   constexpr auto DrcWidth = 854.0f;
   constexpr auto DrcHeight = 480.0f;
   constexpr auto SplitCombinedWidth = std::max(TvWidth, DrcWidth);
   constexpr auto SplitCombinedHeight = TvHeight + DrcHeight;

   const auto &displaySettings = gpu::config()->display;
   const auto splitScreenSeparation = static_cast<float>(displaySettings.splitSeperation);

   auto maintainAspectRatio =
      [&](DisplayLayout::Display &display,
          float width, float height,
          float referenceWidth, float referenceHeight)
      {
         const auto widthRatio = static_cast<float>(width) / referenceWidth;
         const auto heightRatio = static_cast<float>(height) / referenceHeight;
         const auto aspectRatio = std::min(widthRatio, heightRatio);
         display.width = aspectRatio * referenceWidth;
         display.height = aspectRatio * referenceHeight;
      };

   if (displaySettings.viewMode == gpu::DisplaySettings::TV) {
      layout.tv.visible = true;
      layout.drc.visible = false;

      if (displaySettings.maintainAspectRatio) {
         maintainAspectRatio(layout.tv, windowWidth, windowHeight, TvWidth, TvHeight);
         layout.tv.x = (windowWidth - layout.tv.width) / 2.0f;
         layout.tv.y = (windowHeight - layout.tv.height) / 2.0f;
      } else {
         layout.tv.x = 0.0f;
         layout.tv.y = 0.0f;
         layout.tv.width = windowWidth;
         layout.tv.height = windowHeight;
      }
   } else if (displaySettings.viewMode == gpu::DisplaySettings::Gamepad1) {
      layout.tv.visible = false;
      layout.drc.visible = true;

      if (displaySettings.maintainAspectRatio) {
         maintainAspectRatio(layout.drc, windowWidth, windowHeight, DrcWidth, DrcHeight);
         layout.drc.x = (windowWidth - layout.drc.width) / 2.0f;
         layout.drc.y = (windowHeight - layout.drc.height) / 2.0f;
      } else {
         layout.drc.x = 0.0f;
         layout.drc.y = 0.0f;
         layout.drc.width = windowWidth;
         layout.drc.height = windowHeight;
      }
   } else if (displaySettings.viewMode == gpu::DisplaySettings::Split) {
      layout.tv.visible = true;
      layout.drc.visible = true;

      if (displaySettings.maintainAspectRatio) {
         auto combined = DisplayLayout::Display { };
         maintainAspectRatio(combined, windowWidth, windowHeight,
                             SplitCombinedWidth,
                             SplitCombinedHeight + splitScreenSeparation);

         layout.tv.width = combined.width * (TvWidth / SplitCombinedWidth);
         layout.tv.height = combined.height * (TvHeight / SplitCombinedHeight) - (splitScreenSeparation / 2.0f);
         layout.tv.x = (windowWidth - layout.tv.width) / 2.0f;
         layout.tv.y = (windowHeight - combined.height) / 2.0f;

         layout.drc.width = combined.width * (DrcWidth / SplitCombinedWidth);
         layout.drc.height = combined.height * (DrcHeight / SplitCombinedHeight) - (splitScreenSeparation / 2.0f);
         layout.drc.x = (windowWidth - layout.drc.width) / 2.0f;
         layout.drc.y = layout.tv.y + layout.tv.height + splitScreenSeparation;
      } else {
         layout.tv.x = 0.0f;
         layout.tv.y = 0.0f;
         layout.tv.width = windowWidth;
         layout.tv.height = windowHeight * (TvHeight / SplitCombinedHeight);

         layout.drc.x = 0.0f;
         layout.drc.y = layout.tv.height;
         layout.drc.width = windowWidth;
         layout.drc.height = windowHeight * (DrcHeight / SplitCombinedHeight);
      }
   }

   layout.backgroundColour[0] = displaySettings.backgroundColour[0] / 255.0f;
   layout.backgroundColour[1] = displaySettings.backgroundColour[1] / 255.0f;
   layout.backgroundColour[2] = displaySettings.backgroundColour[2] / 255.0f;
   layout.backgroundColour[3] = 1.0f;

   // TODO: Only do if SRGB
   layout.backgroundColour[0] = pow(layout.backgroundColour[0], 2.2f);
   layout.backgroundColour[1] = pow(layout.backgroundColour[1], 2.2f);
   layout.backgroundColour[2] = pow(layout.backgroundColour[2], 2.2f);
}

} // namespace gpu7
