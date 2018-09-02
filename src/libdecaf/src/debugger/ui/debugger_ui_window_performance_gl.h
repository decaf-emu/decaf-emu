#pragma once
#ifdef DECAF_GL
#include "debugger_ui_window.h"
#include "debugger_ui_window_performance.h"
#include <libgpu/gpu_opengldriver.h>

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
   gpu::OpenGLDriver::DebuggerInfo *mInfo;
};

} // namespace ui

} // namespace debugger

#endif // ifdef DECAF_GL