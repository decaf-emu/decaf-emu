#ifdef DECAF_SDL
#include <SDL.h>
#include <glbinding/gl/gl.h>
#include "config.h"
#include "platform.h"
#include "platform_sdl.h"
#include "platform_ui.h"
#include "gpu/driver.h"
#include "utils/log.h"

#if defined(PLATFORM_WINDOWS)
   #include <windows.h>  // For GetSystemMetrics()
#elif defined(PLATFORM_LINUX)
   #include <SDL_syswm.h>
   #include <X11/Xatom.h>
   #include <X11/Xlib.h>
#endif

static const auto DrcWidth = 854.0f;
static const auto DrcHeight = 480.0f;
static const auto DrcScaleFactor = 0.5f;

static const auto TvWidth = 1280.0f;
static const auto TvHeight = 720.0f;
static const auto TvScaleFactor = 0.65f;

namespace platform
{

static int
getWindowBorderHeight(SDL_Window *window)
{
   int height = 50;  // A hopefully reasonable default.

#if defined(PLATFORM_WINDOWS)
   // TODO: Is this correct?
   height = GetSystemMetrics(SM_CYBORDER) * 2 + GetSystemMetrics(SM_CYCAPTION);

#elif defined(PLATFORM_LINUX)
   // Linux (more accurately X11) doesn't have any reliable way to get the
   // border size of a window before it has been shown (or, for that matter,
   // at any given time _after_ it has been shown).  What we can do is look
   // through the window list for a window with _NET_FRAME_EXTENTS set, and
   // assume that the border widths reported there are used for all windows.
   // Since we only use this for setting initial window positions, it
   // shouldn't be too bad even if we get the value wrong.

   SDL_SysWMinfo info;
   SDL_VERSION(&info.version);

   if (window && SDL_GetWindowWMInfo(sdl::tvWindow, &info)) {
      if (info.subsystem == SDL_SYSWM_X11) {
         Display *display = info.info.x11.display;
         Atom atom_NET_FRAME_EXTENTS = XInternAtom(
            display, "_NET_FRAME_EXTENTS", True);
         if (atom_NET_FRAME_EXTENTS) {
            Window *windows;
            unsigned int numWindows = 0;
            Window dummyWindow;
            XQueryTree(display, DefaultRootWindow(display), &dummyWindow,
                       &dummyWindow, &windows, &numWindows);
            for (unsigned int i = 0; i < numWindows; i++) {
               Atom type;
               int format;
               unsigned long numItems, bytesAfter;
               unsigned char *value;
               int result = XGetWindowProperty(
                  display, windows[i], atom_NET_FRAME_EXTENTS, 0, 4, False,
                  AnyPropertyType, &type, &format, &numItems, &bytesAfter,
                  &value);
               if (result == Success && format == 32 && numItems == 4) {
                  auto data = reinterpret_cast<const unsigned long *>(value);
                  height = data[2] + data[3];
                  height = std::min(height, 100);  // Sanity check, just in case.
                  break;
               }
            }
         }
      } else {
         gLog->warn("Unhandled window system {}, guessing border height",
                    info.subsystem);
      }
   }
#endif  // TODO: Need OS X code.

   return height;
}

bool
PlatformSDL::init()
{
   if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) != 0) {
      gLog->error("Failed to initialize SDL: {}", SDL_GetError());
      return false;
   }
   if (SDL_GL_LoadLibrary(NULL) != 0) {
      gLog->error("Failed to load OpenGL library: {}", SDL_GetError());
      return false;
   }

   SDL_JoystickEventState(SDL_IGNORE);

   return true;
}

bool
PlatformSDL::createWindows(const std::string &tvTitle, const std::string &drcTitle)
{
   SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
   SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
   SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
   SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

#ifdef PLATFORM_APPLE
   // If we don't explicitly request an OpenGL version on OS X, we get the
   // ancient OpenGL 2.1 interface.
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                       SDL_GL_CONTEXT_PROFILE_CORE);
#endif

   const uint32_t flags = SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI;
   auto tvWidth = getTvWidth();
   auto tvHeight = getTvHeight();
   auto tvX = SDL_WINDOWPOS_UNDEFINED;
   auto tvY = SDL_WINDOWPOS_UNDEFINED;
   auto drcWidth = getDrcWidth();
   auto drcHeight = getDrcHeight();
   auto drcX = SDL_WINDOWPOS_UNDEFINED;
   auto drcY = SDL_WINDOWPOS_UNDEFINED;
   auto positionsLoaded = false;

   if (config::ui::tv_window_x != INT_MIN) {
      tvX = config::ui::tv_window_x;
      tvY = config::ui::tv_window_y;
      drcX = config::ui::drc_window_x;
      drcY = config::ui::drc_window_y;
      positionsLoaded = true;
   } else {
      // If no position was set in the config, add the DRC window height to the
      // TV window and then resize after opening the TV window, so the DRC window
      // is guaranteed to fit on screen.
      tvHeight += getWindowBorderHeight(nullptr) + getDrcHeight();
   }

   // Create TV window
   mTvWindow = SDL_CreateWindow(tvTitle.c_str(), tvX, tvY, tvWidth, tvHeight, flags);

   if (!mTvWindow) {
      gLog->error("Failed to create TV window: {}", SDL_GetError());
      return false;
   }

   // Set default DRC position
   if (!positionsLoaded) {
      SDL_SetWindowSize(mTvWindow, getTvWidth(), getTvHeight());
      SDL_GetWindowPosition(mTvWindow, &tvX, &tvY);
      drcX = tvX + (getTvWidth() - getDrcWidth()) / 2;
      drcY = tvY + getTvHeight() + getWindowBorderHeight(mTvWindow);
   }

   // Create DRC window
   mDrcWindow = SDL_CreateWindow(drcTitle.c_str(), drcX, drcY, drcWidth, drcHeight, flags);

   if (!mDrcWindow) {
      gLog->error("Failed to create DRC window: {}", SDL_GetError());
      SDL_DestroyWindow(mTvWindow);
      mTvWindow = nullptr;
      return false;
   }

   // Create OpenGL context
   mContext = SDL_GL_CreateContext(mTvWindow);

   if (!mContext) {
      gLog->error("Failed to create OpenGL context: {}", SDL_GetError());

      SDL_DestroyWindow(mDrcWindow);
      mDrcWindow = nullptr;

      SDL_DestroyWindow(mTvWindow);
      mTvWindow = nullptr;
      return false;
   }

   // SDL won't let the rendering thread take over the context unless we
   // explicitly release it from this thread.
   SDL_GL_MakeCurrent(mTvWindow, NULL);

   return true;
}

void
PlatformSDL::run()
{
   while (!mShouldQuit) {
      SDL_Event event;

      if (!SDL_WaitEvent(&event)) {
         gLog->error("Error waiting for event: {}", SDL_GetError());
         continue;
      }

      handleEvent(&event);
   }
}

void
PlatformSDL::handleEvent(const SDL_Event *event)
{
   switch (event->type) {
   case SDL_WINDOWEVENT:
      if (event->window.event == SDL_WINDOWEVENT_CLOSE) {
         mShouldQuit = true;
      }
      break;

   case SDL_QUIT:
      mShouldQuit = true;
      break;
   }
}

void
PlatformSDL::shutdown()
{
   if (mTvWindow) {
      SDL_GetWindowPosition(mTvWindow,
                            &config::ui::tv_window_x,
                            &config::ui::tv_window_y);
   }

   if (mDrcWindow) {
      SDL_GetWindowPosition(mDrcWindow,
                            &config::ui::drc_window_x,
                            &config::ui::drc_window_y);
   }

   SDL_Quit();
}

void
PlatformSDL::activateContext()
{
   SDL_GL_MakeCurrent(mTvWindow, mContext);
}

void
PlatformSDL::swapBuffers()
{
   SDL_GL_SwapWindow(mTvWindow);
   SDL_GL_SwapWindow(mDrcWindow);
}

void
PlatformSDL::bindDrcWindow()
{
   SDL_GL_MakeCurrent(mDrcWindow, mContext);
   gl::glViewport(0, 0, getDrcWidth(), getDrcHeight());
}

void
PlatformSDL::bindTvWindow()
{
   SDL_GL_MakeCurrent(mTvWindow, mContext);
   gl::glViewport(0, 0, getTvWidth(), getTvHeight());
}

int
PlatformSDL::getDrcWidth()
{
   return static_cast<int>(DrcWidth * DrcScaleFactor);
}

int
PlatformSDL::getDrcHeight()
{
   return static_cast<int>(DrcHeight * DrcScaleFactor);
}

int
PlatformSDL::getTvWidth()
{
   return static_cast<int>(TvWidth * TvScaleFactor);
}

int
PlatformSDL::getTvHeight()
{
   return static_cast<int>(TvHeight * TvScaleFactor);
}

} // namespace platform

#endif // ifdef DECAF_SDL
