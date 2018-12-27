#include "debugui.h"
#include "debugui_renderer_opengl.h"
#include "debugui_renderer_vulkan.h"

#include <common/decaf_assert.h>

namespace debugui
{

#ifdef DECAF_GL
OpenGLRenderer *
createOpenGLRenderer(const std::string &configPath)
{
   return new RendererOpenGL { configPath };
}
#endif // ifdef DECAF_GL

#ifdef DECAF_VULKAN
VulkanRenderer *
createVulkanRenderer(const std::string &configPath, VulkanRendererInfo &info)
{
   return new RendererVulkan { configPath, info };
}
#endif // ifdef DECAF_VULKAN

} // namespace debugui
