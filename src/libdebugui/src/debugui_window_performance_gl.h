#pragma once
#ifdef DECAF_GL
#include "debugui_window.h"
#include "debugui_window_performance.h"
#include <libgpu/gpu_opengldriver.h>

namespace debugui
{

class PerformanceWindowGL : public PerformanceWindow
{
public:
   PerformanceWindowGL(const std::string &name);
   virtual ~PerformanceWindowGL() = default;

   void drawBackendInfo() override;

private:
   gpu::OpenGLDriverDebugInfo mInfo;
};

} // namespace debugui

#endif // ifdef DECAF_GL