#include "gpu_graphicsdriver.h"
#include "gpu_config.h"

#include "null/null_driver.h"
#include "opengl/opengl_driver.h"
#include "vulkan/vulkan_driver.h"

namespace gpu
{

GraphicsDriver *
createGraphicsDriver()
{
   switch (config()->display.backend) {
   case DisplaySettings::Null:
      return createGraphicsDriver(GraphicsDriverType::Null);
   case DisplaySettings::OpenGL:
      return createGraphicsDriver(GraphicsDriverType::OpenGL);
   case DisplaySettings::Vulkan:
      return createGraphicsDriver(GraphicsDriverType::Vulkan);
   default:
      return nullptr;
   }
}

GraphicsDriver *
createGraphicsDriver(GraphicsDriverType type)
{
   switch (type) {
   case GraphicsDriverType::Null:
      return new null::Driver{};
#ifdef DECAF_GL
   case GraphicsDriverType::OpenGL:
      return new opengl::GLDriver{};
#endif
#ifdef DECAF_VULKAN
   case GraphicsDriverType::Vulkan:
      return new vulkan::Driver{};
#endif
   default:
      return nullptr;
   }
}

} // namespace gpu
