#include "gpu_graphicsdriver.h"

#include "dx12/dx12_driver.h"
#include "null/null_driver.h"
#include "opengl/opengl_driver.h"

namespace gpu
{

GraphicsDriver *
createGLDriver()
{
#ifdef DECAF_NOGL
   return nullptr;
#else
   return new opengl::GLDriver {};
#endif
}

GraphicsDriver *
createNullDriver()
{
   return new null::Driver {};
}

GraphicsDriver *
createDX12Driver()
{
#ifdef DECAF_DX12
   return new dx12::Driver { };
#else
   return nullptr;
#endif
}

} // namespace gpu
