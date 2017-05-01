#pragma once
#include "debugger_ui_window.h"
#include "debugger_ui_window_performance.h"

namespace debugger
{

namespace ui
{

class PerformanceWindowGL : public PerformanceWindow
{
public:
   PerformanceWindowGL(const std::string &name);
   virtual ~PerformanceWindowGL() = default;

   void draw() override;
};

} // namespace ui

} // namespace debugger
