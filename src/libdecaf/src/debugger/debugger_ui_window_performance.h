#pragma once
#include "debugger_ui_window.h"

namespace debugger
{

namespace ui
{

class PerformanceWindow : public Window
{
public:
   PerformanceWindow(const std::string &name);
   virtual ~PerformanceWindow() = default;

   void draw() override;

   void drawGraphs();
   static PerformanceWindow* create(const std::string &name);
};

} // namespace ui

} // namespace debugger
