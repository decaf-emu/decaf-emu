#ifdef DECAF_SDL

#include "clilog.h"
#include "config.h"
#include "common/strutils.h"
#include "decaf/decaf.h"
#include "gl_common.h"
#include "input_common.h"
#include "platform/platform.h"
#include <SDL.h>
#include <thread>

#if defined(PLATFORM_WINDOWS)
#include <Windows.h>  // For GetSystemMetrics()
#elif defined(PLATFORM_LINUX)
#include <SDL_syswm.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#endif

using namespace decaf::input;

static int
getWindowBorderHeight(SDL_Window *window)
{
   int height = 50;  // A hopefully reasonable default.

#if defined(PLATFORM_WINDOWS)
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

   if (window && SDL_GetWindowWMInfo(mTvWindow, &info)) {
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

static SDL_Window *
gTvWindow = nullptr;

static SDL_Window *
gDrcWindow = nullptr;

static SDL_GLContext
gContext = nullptr;

static SDL_GLContext
gThreadContext = nullptr;

int sdlStart()
{
   if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) != 0) {
      gLog->error("Failed to initialize SDL: {}", SDL_GetError());
      return false;
   }

   if (SDL_GL_LoadLibrary(NULL) != 0) {
      gLog->error("Failed to load OpenGL library: {}", SDL_GetError());
      return false;
   }


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

   auto tvWndTitle = "Decaf";
   auto tvWndWidth = static_cast<int>(1280 * 0.7f);
   auto tvWndHeight = static_cast<int>(768 * 0.7f);

   auto drcWndTitle = "Decaf - DRC";
   auto drcWndWidth = static_cast<int>(854 * 0.4f);
   auto drcWndHeight = static_cast<int>(480 * 0.4f);

   const uint32_t flags = SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI;
   auto tvWidth = tvWndWidth;
   auto tvHeight = tvWndHeight;
   auto tvX = SDL_WINDOWPOS_UNDEFINED;
   auto tvY = SDL_WINDOWPOS_UNDEFINED;
   auto drcWidth = drcWndWidth;
   auto drcHeight = drcWndHeight;
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
      tvHeight += getWindowBorderHeight(nullptr) + drcWndHeight;
   }

   // Create TV window
   gTvWindow = SDL_CreateWindow(tvWndTitle, tvX, tvY, tvWidth, tvHeight, flags);

   if (!gTvWindow) {
      gLog->error("Failed to create TV window: {}", SDL_GetError());
      return false;
   }

   // Set default DRC position
   if (!positionsLoaded) {
      SDL_SetWindowSize(gTvWindow, tvWndWidth, tvWndHeight);
      SDL_GetWindowPosition(gTvWindow, &tvX, &tvY);
      drcX = tvX + (tvWndWidth - drcWndWidth) / 2;
      drcY = tvY + tvWndHeight + getWindowBorderHeight(gTvWindow);
   }

   // Create DRC window
   gDrcWindow = SDL_CreateWindow(drcWndTitle, drcX, drcY, drcWidth, drcHeight, flags);

   if (!gDrcWindow) {
      gLog->error("Failed to create DRC window: {}", SDL_GetError());
      SDL_DestroyWindow(gTvWindow);
      gTvWindow = nullptr;
      return false;
   }

   SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);

   // Create OpenGL context
   gContext = SDL_GL_CreateContext(gTvWindow);

   if (!gContext) {
      gLog->error("Failed to create Main OpenGL context: {}", SDL_GetError());

      SDL_DestroyWindow(gDrcWindow);
      gDrcWindow = nullptr;

      SDL_DestroyWindow(gTvWindow);
      gTvWindow = nullptr;
      return false;
   }

   gThreadContext = SDL_GL_CreateContext(gTvWindow);

   if (!gThreadContext) {
      gLog->error("Failed to create GPU OpenGL context: {}", SDL_GetError());

      SDL_DestroyWindow(gDrcWindow);
      gDrcWindow = nullptr;

      SDL_DestroyWindow(gTvWindow);
      gTvWindow = nullptr;
      return false;
   }

   SDL_GL_MakeCurrent(gTvWindow, gContext);
   cglContextInitialise();

   decaf::setVpadCoreButtonCallback([](auto channel, auto button) {
      auto key = getButtonMapping(channel, button);
      auto scancode = SDL_GetScancodeFromKey(key);
      int numKeys = 0;
      auto state = SDL_GetKeyboardState(&numKeys);

      if (scancode >= 0 && scancode < numKeys && state[scancode]) {
         return decaf::input::ButtonPressed;
      }

      return decaf::input::ButtonReleased;
   });

   if (!decaf::initialise()) {
      return 1;
   }

   auto gpuThread = std::thread([]() {
      SDL_GL_MakeCurrent(gTvWindow, gThreadContext);
      cglContextInitialise();
      decaf::runGpuDriver();
   });

   decaf::start();

   cglInitialise();

   bool shouldQuit = false;
   while (!shouldQuit) {
      SDL_Event event;

      if (SDL_PollEvent(&event)) {
         switch (event.type) {
         case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
               shouldQuit = true;
            }
            break;

         case SDL_QUIT:
            shouldQuit = true;
            break;
         }
      }

      gl::GLuint tvBuffer;
      gl::GLuint drcBuffer;
      decaf::getSwapBuffers(&tvBuffer, &drcBuffer);

      SDL_GL_MakeCurrent(gTvWindow, gContext);
      gl::glViewport(0, 0, tvWndWidth, tvWndHeight);
      cglDrawScanBuffer(tvBuffer);
      SDL_GL_SwapWindow(gTvWindow);

      SDL_GL_MakeCurrent(gDrcWindow, gContext);
      gl::glViewport(0, 0, drcWndWidth, drcWndHeight);
      cglDrawScanBuffer(drcBuffer);
      SDL_GL_SwapWindow(gDrcWindow);

      static char newTitle[1024];
      snprintf(newTitle, 1024, "Decaf - FPS: %.02f", decaf::getAverageFps());
      SDL_SetWindowTitle(gTvWindow, newTitle);
   }

   // Shut down decaf
   decaf::shutdown();

   // Shut down the GPU
   decaf::shutdownGpuDriver();
   gpuThread.join();

   return 0;
}

#endif // DECAF_SDL
