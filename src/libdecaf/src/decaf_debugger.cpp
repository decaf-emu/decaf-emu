#include "debugger/opengl/debugger_ui_opengl.h"
#include "debugger/vulkan/debugger_ui_vulkan.h"
#include "decaf_debugger.h"
#include <common/decaf_assert.h>

namespace decaf
{

static DebugUiRenderer *
sUiRenderer = nullptr;

DebugUiRenderer *
createDebugGLRenderer()
{
#ifdef DECAF_GL
   return new ::debugger::ui::RendererOpenGL { };
#else
   decaf_abort("libdecaf was built with OpenGL support disabled");
#endif // ifdef DECAF_GL
}

DebugUiRenderer *
createDebugVulkanRenderer()
{
#ifdef DECAF_VULKAN
   return new ::debugger::ui::RendererVulkan { };
#else
   decaf_abort("libdecaf was built with Vulkan support disabled");
#endif // ifdef DECAF_VULKAN
}

void
setDebugUiRenderer(DebugUiRenderer *renderer)
{
   sUiRenderer = renderer;
}

DebugUiRenderer *
getDebugUiRenderer()
{
   return sUiRenderer;
}

} // namespace decaf