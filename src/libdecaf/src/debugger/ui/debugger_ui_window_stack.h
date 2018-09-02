#pragma once
#include "debugger_ui_window.h"
#include "debugger_ui_addrscroller.h"

#include <functional>
#include <map>

namespace debugger
{

namespace ui
{

enum class StackGlyph : uint32_t
{
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

class StackWindow : public Window
{
public:
   StackWindow(const std::string &name);
   virtual ~StackWindow() = default;

   virtual void
   draw();

   void
   update();

   void
   gotoAddress(uint32_t address);

private:
   StackGlyph
   getStackGlyph(uint32_t addr);

private:
   AddressScroller mAddressScroller;
   int64_t mSelectedAddr = -1;
   uint32_t mStackFrameCacheAddr = -1;
   std::map<uint32_t, StackFrame, std::greater<uint32_t>> mStackFrames;
};

} // namespace ui

} // namespace debugger
