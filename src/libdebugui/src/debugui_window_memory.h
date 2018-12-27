#pragma once
#include "debugui_addrscroller.h"
#include "debugui_window.h"
#include <cstdint>

namespace debugui
{

class MemoryWindow : public Window
{
public:
   MemoryWindow(const std::string &name);
   virtual ~MemoryWindow() = default;

   virtual void    draw();

   void gotoAddress(uint32_t address);

private:
   AddressScroller mAddressScroller;
   bool mEditingEnabled = true;
   int64_t mEditAddress = -1;
   int64_t mLastEditAddress = -1;
};

} // namespace debugui
