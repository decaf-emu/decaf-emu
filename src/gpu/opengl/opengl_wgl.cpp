#include "opengl_driver.h"
#include "platform/platform_ui.h"
#include <Windows.h>

namespace gpu
{

namespace opengl
{

void GLDriver::setupWindow()
{
   int pixelFormat;

   PIXELFORMATDESCRIPTOR pfd =
   {
      sizeof(PIXELFORMATDESCRIPTOR),
      1,
      PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
      PFD_TYPE_RGBA,             // The kind of framebuffer. RGBA or palette.
      32,                        // Colordepth of the framebuffer.
      0, 0, 0, 0, 0, 0,
      0,
      0,
      0,
      0, 0, 0, 0,
      24,                        // Number of bits for the depthbuffer
      8,                         // Number of bits for the stencilbuffer
      0,                         // Number of Aux buffers in the framebuffer.
      PFD_MAIN_PLANE,
      0,
      0, 0, 0
   };


   auto handle = reinterpret_cast<HWND>(platform::ui::getWindowHandle());
   auto dc = GetDC(handle);
   pixelFormat = ChoosePixelFormat(dc, &pfd);
   SetPixelFormat(dc, pixelFormat, &pfd);

   mDeviceContext = reinterpret_cast<uint64_t>(dc);
   mOpenGLContext = reinterpret_cast<uint64_t>(wglCreateContext(dc));
}

void GLDriver::swapBuffers()
{
   auto dc = reinterpret_cast<HDC>(mDeviceContext);
   SwapBuffers(dc);
}

void GLDriver::activateDeviceContext()
{
   auto dc = reinterpret_cast<HDC>(mDeviceContext);
   auto gc = reinterpret_cast<HGLRC>(mOpenGLContext);
   wglMakeCurrent(dc, gc);
}

} // namespace opengl

} // namespace gpu
