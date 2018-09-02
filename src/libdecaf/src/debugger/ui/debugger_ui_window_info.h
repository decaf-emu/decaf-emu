#pragma once
#include "debugger_ui_window.h"

namespace debugger
{

namespace ui
{

class InfoWindow : public Window
{
public:
   InfoWindow(const std::string &name);
   virtual ~InfoWindow() = default;

   virtual void draw();
};

} // namespace ui

} // namespace debugger
