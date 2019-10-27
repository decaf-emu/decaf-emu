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

struct DisplayTouchEvent
{
   enum Screen
   {
      None,
      Tv,
      Drc1,
      Drc2,
   };

   Screen screen = None;
   float x = 0;
   float y = 0;
};

void
updateDisplayLayout(DisplayLayout &layout,
                    float windowWidth,
                    float windowHeight);

DisplayTouchEvent
translateDisplayTouch(DisplayLayout &layout,
                      float x,
                      float y);

static inline DisplayLayout
getDisplayLayout(float width, float height)
{
   DisplayLayout layout;
   updateDisplayLayout(layout, width, height);
   return layout;
}

} // namespace gpu7
