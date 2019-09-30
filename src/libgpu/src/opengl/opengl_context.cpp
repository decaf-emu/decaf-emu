#pragma optimize("", off)
#if DECAF_GL
#include "opengl_context.h"
#include "gpu_config.h"

#include <common/decaf_assert.h>
#include <common/platform.h>
#include <fmt/format.h>
#include <glad/glad.h>

#ifdef PLATFORM_WINDOWS
#include <glad/glad_wgl.h>
#endif

#ifdef PLATFORM_LINUX
#include <glad/glad_glx.h>
#include <X11/Xlib.h>
#endif

namespace opengl
{

#ifdef PLATFORM_WINDOWS
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
         return nullptr;
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

      gladLoadWGL(self->mDeviceContext);

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

#ifdef PLATFORM_LINUX
class X11Window
{
public:
   ~X11Window()
   {
      XUnmapWindow(mDisplay, mWindow);
      XDestroyWindow(mDisplay, mWindow);
      XFreeColormap(mDisplay, mColourMap);
   }

   std::pair<int, int> getDimensions()
   {
      auto attribs = XWindowAttributes { };
      XGetWindowAttributes(mDisplay, mParentWindow, &attribs);
      XResizeWindow(mDisplay, mWindow, attribs.width, attribs.height);
      return { attribs.width, attribs.height };
   }

   Window getWindow()
   {
      return mWindow;
   }

   static std::unique_ptr<X11Window> create(Display *display, Window parent, XVisualInfo *vi)
   {
      auto self = std::make_unique<X11Window>();
      self->mDisplay = display;
      self->mParentWindow = parent;
      self->mColourMap = XCreateColormap(self->mDisplay, self->mParentWindow, vi->visual, AllocNone);

      auto attribs = XSetWindowAttributes { };
      attribs.colormap = self->mColourMap;

      // Get the dimensions from the parent window.
      auto parentAttribs = XWindowAttributes { };
      XGetWindowAttributes(self->mDisplay, self->mParentWindow, &parentAttribs);

      // Create the window
      self->mWindow =
         XCreateWindow(self->mDisplay, self->mParentWindow, 0, 0, parentAttribs.width,
                       parentAttribs.height, 0, vi->depth, InputOutput,
                       vi->visual, CWColormap, &attribs);
      XSelectInput(self->mDisplay, self->mParentWindow, StructureNotifyMask);
      XMapWindow(self->mDisplay, self->mWindow);
      XSync(self->mDisplay, True);

      return self;
   }

private:
   Display *mDisplay = nullptr;
   Window mParentWindow = 0;
   Colormap mColourMap = 0;
   Window mWindow = 0;
};

class X11Context : public GLContext
{
public:
   virtual ~X11Context() {};

   static std::unique_ptr<GLContext> create(const gpu::WindowSystemInfo &wsi)
   {
      auto self = std::make_unique<X11Context>();
      self->mDisplay = static_cast<Display *>(wsi.displayConnection);

      int fbAttribs[] = {
         GLX_X_RENDERABLE, True,
         GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
         GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
         GLX_RED_SIZE, 8,
         GLX_GREEN_SIZE, 8,
         GLX_BLUE_SIZE, 8,
         GLX_DEPTH_SIZE, 0,
         GLX_STENCIL_SIZE, 0,
         GLX_DOUBLEBUFFER, True,
         None
      };
      auto fbCount = int { 0 };
      auto screen = DefaultScreen(self->mDisplay);
      
      if (!gladLoadGLX(self->mDisplay, screen)) {
         return { };
      }

      auto chosenFbConfig = glXChooseFBConfig(self->mDisplay, screen, fbAttribs, &fbCount);
      if (!chosenFbConfig) {
         return { };
      }
      self->mFBConfig = *chosenFbConfig;

      int contextAttribs[] = {
         GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
         GLX_CONTEXT_MINOR_VERSION_ARB, 5,
         GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
         GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
         None
      };

      self->mContext = glXCreateContextAttribsARB(self->mDisplay, self->mFBConfig, nullptr, True, contextAttribs);
      if (!self->mContext) {
         return { };
      }
      self->mContextAttribs.insert(self->mContextAttribs.end(), std::begin(contextAttribs), std::end(contextAttribs));

      if (!self->createWindowSurface(reinterpret_cast<Window>(wsi.renderSurface))) {
         return { };
      }

      self->makeCurrent();
      return self;
   }

   bool createWindowSurface(Window window)
   {
      if (window) {
         auto info = glXGetVisualFromFBConfig(mDisplay, mFBConfig);
         mX11Window = X11Window::create(mDisplay, window, info);
         mDrawable = static_cast<GLXDrawable>(mX11Window->getWindow());
         XFree(info);
      } else {
         mPBuffer = glXCreateGLXPbufferSGIX(mDisplay, mFBConfig, 1, 1, nullptr);
         if (!mPBuffer) {
            return false;
         }

         mDrawable = static_cast<GLXDrawable>(mPBuffer);
      }

      return true;
   }

   void makeCurrent() override
   {
      glXMakeCurrent(mDisplay, mDrawable, mContext);
   }

   void clearCurrent() override
   {
      glXMakeCurrent(mDisplay, None, nullptr);
   }

   std::pair<int, int> getDimensions() override
   {
      return mX11Window->getDimensions();
   }

   void setSwapInterval(int interval) override
   {
      glXSwapIntervalEXT(mDisplay, mDrawable, interval);
   }
   
   void swapBuffers() override
   {
      glXSwapBuffers(mDisplay, mDrawable);
   }

   std::unique_ptr<GLContext> createSharedContext() override
   {
      auto sharedContext = std::make_unique<X11Context>();
      sharedContext->mDisplay = mDisplay;
      sharedContext->mFBConfig = mFBConfig;
      sharedContext->mContextAttribs = mContextAttribs;
      sharedContext->mContext = glXCreateContextAttribsARB(sharedContext->mDisplay, sharedContext->mFBConfig, mContext, True, sharedContext->mContextAttribs.data());

      if (!sharedContext->mContext) {
         return { };
      }

      if (mPBuffer) {
         sharedContext->createWindowSurface(0);
      }

      return sharedContext;
   }

private:
   Display *mDisplay = nullptr;
   GLXFBConfig mFBConfig = { };
   GLXContext mContext = 0;
   GLXDrawable mDrawable = 0;
   GLXPbufferSGIX mPBuffer = 0;
   std::unique_ptr<X11Window> mX11Window;
   std::vector<int> mContextAttribs;
};

#endif // ifdef PLATFORM_LINUX

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
#ifdef PLATFORM_LINUX
   case gpu::WindowSystemType::Xcb:
   {
      return X11Context::create(wsi);
   }
#endif
   default:
      decaf_abort(fmt::format("Unsupported OpenGL WindowSystemType {}", wsi.type));
      return { };
   }
}

} // namespace opengl

#endif // if DECAF_GL
