#pragma once
#include "debugger_ui_window.h"
#include "debugger_ui_window_performance.h"
#include "decaf_graphics_info.h"

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

private:
   std::shared_ptr<decaf::GraphicsDebugInfoGL> mDebugInfo;
};

} // namespace ui

} // namespace debugger
