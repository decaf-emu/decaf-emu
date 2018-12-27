#pragma once
#include "debugui_window.h"
#include <vector>

namespace debugui
{

using CafeMemorySegment = decaf::debug::CafeMemorySegment;

class SegmentsWindow : public Window
{
public:
   SegmentsWindow(const std::string &name);
   virtual ~SegmentsWindow() = default;

   virtual void draw();

private:
   std::vector<CafeMemorySegment> mSegmentsCache;
};

} // namespace debugui
