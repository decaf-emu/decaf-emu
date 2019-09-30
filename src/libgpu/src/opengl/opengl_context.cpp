#pragma optimize("", off)
#if DECAF_GL
#include "opengl_context.h"
#include "gpu_config.h"

#include <common/decaf_assert.h>
#include <common/platform.h>
#include <fmt/format.h>
#include <glad/glad.h>

#ifdef PLATFORM_WINDOWS
#include <Windows.h>
#include "wglext.h"
#endif

namespace opengl
{

#ifdef PLATFORM_WINDOWS
static PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = nullptr;
static PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = nullptr;
static PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = nullptr;
static PFNWGLCREATEPBUFFERARBPROC wglCreatePbufferARB = nullptr;
static PFNWGLGETPBUFFERDCARBPROC wglGetPbufferDCARB = nullptr;
static PFNWGLRELEASEPBUFFERDCARBPROC wglReleasePbufferDCARB = nullptr;
static PFNWGLDESTROYPBUFFERARBPROC wglDestroyPbufferARB = nullptr;

static void
loadWglExtensions()
{
   wglSwapIntervalEXT =
      reinterpret_cast<PFNWGLSWAPINTERVALEXTPROC>(wglGetProcAddress("wglSwapIntervalEXT"));
   wglCreateContextAttribsARB = reinterpret_cast<PFNWGLCREATECONTEXTATTRIBSARBPROC>(
      wglGetProcAddress("wglCreateContextAttribsARB"));
   wglChoosePixelFormatARB = reinterpret_cast<PFNWGLCHOOSEPIXELFORMATARBPROC>(
      wglGetProcAddress("wglChoosePixelFormatARB"));
   wglCreatePbufferARB =
      reinterpret_cast<PFNWGLCREATEPBUFFERARBPROC>(wglGetProcAddress("wglCreatePbufferARB"));
   wglGetPbufferDCARB =
      reinterpret_cast<PFNWGLGETPBUFFERDCARBPROC>(wglGetProcAddress("wglGetPbufferDCARB"));
   wglReleasePbufferDCARB =
      reinterpret_cast<PFNWGLRELEASEPBUFFERDCARBPROC>(wglGetProcAddress("wglReleasePbufferDCARB"));
   wglDestroyPbufferARB =
      reinterpret_cast<PFNWGLDESTROYPBUFFERARBPROC>(wglGetProcAddress("wglGetPbufferDCARB"));
}

class WglContext : public GLContext
{
public:
   virtual ~WglContext() = default;

   static HGLRC createCoreContext(HDC dc)
   {
      int attribs[] = {
         WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
         WGL_CONTEXT_FLAGS_ARB,
            WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB |
            (gpu::config()->debug.debug_enabled ? WGL_CONTEXT_DEBUG_BIT_ARB : 0),
         WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
         WGL_CONTEXT_MINOR_VERSION_ARB, 6,
         0, 0
      };

      if (!wglCreateContextAttribsARB) {
         loadWglExtensions();

         if (!wglCreateContextAttribsARB) {
            return nullptr;
         }
      }

      return wglCreateContextAttribsARB(dc, nullptr, attribs);
   }

   static std::unique_ptr<GLContext> create(const gpu::WindowSystemInfo &wsi)
   {
      auto self = std::make_unique<WglContext>();
      self->mWindowHandle = reinterpret_cast<HWND>(wsi.renderSurface);
      self->mDeviceContext = ::GetDC(self->mWindowHandle);
      if (!self->mDeviceContext) {
         return nullptr;
      }

      static constexpr auto pfd = PIXELFORMATDESCRIPTOR {
         sizeof(PIXELFORMATDESCRIPTOR),
         1,
         PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
         PFD_TYPE_RGBA,
         32,
         0,
         0, 0, 0, 0, 0,
         0,
         0,
         0,
         0, 0, 0, 0,
         0,
         0,
         0,
         PFD_MAIN_PLANE,
         0,
         0, 0, 0,
      };

      auto pixelFormat = ::ChoosePixelFormat(self->mDeviceContext, &pfd);
      if (!pixelFormat) {
         return false;
      }

      if (!::SetPixelFormat(self->mDeviceContext, pixelFormat, &pfd)) {
         return false;
      }

      self->mContext = ::wglCreateContext(self->mDeviceContext);
      ::wglMakeCurrent(self->mDeviceContext, self->mContext);
      loadWglExtensions();

      if (auto coreContext = createCoreContext(self->mDeviceContext)) {
         ::wglMakeCurrent(self->mDeviceContext, coreContext);
         ::wglDeleteContext(self->mContext);
         self->mContext = coreContext;
         coreContext = nullptr;
      }

      return self;
   }

   void makeCurrent() override
   {
      ::wglMakeCurrent(mDeviceContext, mContext);
   }

   void clearCurrent() override
   {
      ::wglMakeCurrent(mDeviceContext, nullptr);
   }

   std::pair<int, int> getDimensions() override
   {
      RECT rect;
      GetClientRect(mWindowHandle, &rect);
      return { rect.right - rect.left, rect.bottom - rect.top };
   }

   void setSwapInterval(int interval) override
   {
      if (wglSwapIntervalEXT) {
         wglSwapIntervalEXT(interval);
      }
   }

   void swapBuffers() override
   {
      ::SwapBuffers(mDeviceContext);
   }

   std::unique_ptr<GLContext> createSharedContext() override
   {
      auto wsi = gpu::WindowSystemInfo { };
      wsi.renderSurface = mWindowHandle;
      auto sharedContext = create(wsi);
      auto sharedWglContext = static_cast<WglContext *>(sharedContext.get());
      ::wglShareLists(mContext, sharedWglContext->mContext);
      return sharedContext;
   }

private:
   HWND mWindowHandle = nullptr;
   HDC mDeviceContext = nullptr;
   HGLRC mContext = nullptr;
   int mWidth = 0;
   int mHeight = 0;
};
#endif // ifdef PLATFORM_WINDOWS

std::unique_ptr<GLContext>
createContext(const gpu::WindowSystemInfo &wsi)
{
   switch (wsi.type) {
#ifdef PLATFORM_WINDOWS
   case gpu::WindowSystemType::Windows:
   {
      return WglContext::create(wsi);
   }
#endif
   default:
      decaf_abort(fmt::format("Unsupported OpenGL WindowSystemType {}", wsi.type));
      return { };
   }
}

} // namespace opengl

#endif // if DECAF_GL
