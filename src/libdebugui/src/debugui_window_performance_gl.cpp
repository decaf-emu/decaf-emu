#ifdef DECAF_GL
#include "debugui_window_performance_gl.h"

#include <common/decaf_assert.h>
#include <imgui.h>
#include <inttypes.h>
#include <libdecaf/decaf_graphics.h>
#include <libgpu/gpu_opengldriver.h>

namespace debugui
{

PerformanceWindowGL::PerformanceWindowGL(const std::string &name) :
   PerformanceWindow(name)
{
   auto graphicsDriver = decaf::getGraphicsDriver();
   decaf_check(graphicsDriver->type() == gpu::GraphicsDriverType::OpenGL);

   auto glGraphicsDriver = reinterpret_cast<gpu::OpenGLDriver *>(graphicsDriver);
   mInfo = glGraphicsDriver->getDebuggerInfo();
}

void PerformanceWindowGL::drawBackendInfo()
{
   if (!mInfo) {
      return;
   }

   ImGui::Columns(2);

   drawTextAndValue("Vertex Shaders:", mInfo->numVertexShaders);
   drawTextAndValue("Pixel  Shaders:", mInfo->numPixelShaders);
   drawTextAndValue("Fetch  Shaders:", mInfo->numFetchShaders);

   ImGui::NextColumn();

   drawTextAndValue("Shader Pipelines:", mInfo->numShaderPipelines);
   drawTextAndValue("Surfaces:", mInfo->numSurfaces);
   drawTextAndValue("Data Buffers:", mInfo->numDataBuffers);
}

} // namespace debugui

#endif // ifdef DECAF_GL
