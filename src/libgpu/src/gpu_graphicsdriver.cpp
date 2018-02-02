#include "gpu_graphicsdriver.h"

#include "null/null_driver.h"
#include "opengl/opengl_driver.h"
#include "vulkan/vulkan_driver.h"

namespace gpu
{

GraphicsDriver *
createGLDriver()
{
#ifdef DECAF_GL
   return new opengl::GLDriver {};
#else
   return nullptr;
#endif
}

GraphicsDriver *
createNullDriver()
{
   return new null::Driver {};
}

GraphicsDriver *
createVulkanDriver()
{
#ifdef DECAF_VULKAN
   return new vulkan::Driver {};
#else
   return nullptr;
#endif
}

} // namespace gpu
