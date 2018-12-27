#pragma once
#include "debugui_window.h"
#include <string>

namespace debugui
{

class VoicesWindow : public Window
{
public:
   VoicesWindow(const std::string &name);
   virtual ~VoicesWindow() = default;

   virtual void draw();

private:
};

} // namespace debugui
