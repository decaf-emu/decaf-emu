#pragma once
#include "debugger_ui_window.h"
#include <vector>

namespace debugger
{

namespace ui
{

struct Segment
{
   std::string name;
   uint32_t start;
   uint32_t end;
   std::vector<Segment> items;
};

class SegmentsWindow : public Window
{
public:
   SegmentsWindow(const std::string &name);
   virtual ~SegmentsWindow() = default;

   virtual void draw();

   void
   drawSegments(const std::vector<Segment> &segments,
                std::string tabs);

private:
   std::vector<Segment> mSegmentsCache;
};

} // namespace ui

} // namespace debugger
