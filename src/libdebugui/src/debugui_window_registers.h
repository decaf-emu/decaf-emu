#pragma once
#include "debugui_window.h"

#include <libcpu/state.h>
#include <libdecaf/decaf_debug_api.h>

namespace debugui
{

class RegistersWindow : public Window
{
public:
   RegistersWindow(const std::string &name);
   virtual ~RegistersWindow() = default;

   virtual void draw();

private:
   decaf::debug::CafeThread mPreviousThreadState = { };
};

} // namespace debugui
