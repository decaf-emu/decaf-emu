#include "decaf_graphics.h"
#include "gpu/opengl/opengl_driver.h"
#include "gpu/dx12/dx12_driver.h"

namespace decaf
{

static GraphicsDriver *
sGraphicsDriver = nullptr;

GraphicsDriver *
createGLDriver()
{
#ifndef DECAF_NOGL
   return new gpu::opengl::GLDriver();
#else
   decaf_abort("libdecaf was built with OpenGL support disabled");
#endif
}

GraphicsDriver *
createDX12Driver()
{
#ifdef DECAF_DX12
   return new gpu::dx12::Driver();
#else
   decaf_abort("libdecaf was built without DirectX 12 support");
#endif
}

void
setGraphicsDriver(GraphicsDriver *driver)
{
   sGraphicsDriver = driver;
}

GraphicsDriver *
getGraphicsDriver()
{
   return sGraphicsDriver;
}

} // namespace decaf
