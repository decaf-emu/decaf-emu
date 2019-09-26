#pragma once
#include <array>

namespace gpu7
{

struct DisplayLayout
{
   struct Display
   {
      bool visible;
      float x;
      float y;
      float width;
      float height;
   };

   Display tv;
   Display drc;
   std::array<float, 4> backgroundColour;
};

void
updateDisplayLayout(DisplayLayout &layout,
                    float windowWidth,
                    float windowHeight);

} // namespace gpu7
