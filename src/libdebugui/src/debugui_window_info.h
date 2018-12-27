#pragma once
#include "debugui_window.h"

namespace debugui
{

class InfoWindow : public Window
{
public:
   InfoWindow(const std::string &name);
   virtual ~InfoWindow() = default;

   virtual void draw();
};

} // namespace debugui
