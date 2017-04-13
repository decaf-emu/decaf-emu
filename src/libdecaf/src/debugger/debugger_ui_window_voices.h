#pragma once
#include "debugger_ui_window.h"

namespace debugger
{

namespace ui
{

class VoicesWindow : public Window
{
public:
   VoicesWindow(const std::string &name);
   virtual ~VoicesWindow() = default;

   virtual void draw();

private:
};

} // namespace ui

} // namespace debugger
