#include "clilog.h"
#include "config.h"
#include "decafsdl.h"

#include <common/decaf_assert.h>
#include <fmt/format.h>

#include <libgpu/gpu_graphicsdriver.h>
#include <SDL_syswm.h>

static std::string
sActiveGfx = "NOGFX";

void setWindowIcon(SDL_Window *window);

DecafSDL::~DecafSDL()
{
}

bool
DecafSDL::initCore()
{
   if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) != 0) {
      gCliLog->error("Failed to initialize SDL: {}", SDL_GetError());
      return false;
   }

   return true;
}

bool
DecafSDL::initGraphics()
{
   mGraphicsDriver = gpu::createGraphicsDriver();
   if (!mGraphicsDriver) {
      return false;
   }

   switch (mGraphicsDriver->type()) {
   case gpu::GraphicsDriverType::Vulkan:
      sActiveGfx = "Vulkan";
      break;
   case gpu::GraphicsDriverType::OpenGL:
      sActiveGfx = "OpenGL";
      break;
   case gpu::GraphicsDriverType::Null:
      sActiveGfx = "Null";
      break;
   default:
      sActiveGfx = "Unknown";
   }

   return true;
}

bool
DecafSDL::initSound()
{
   if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
      gCliLog->error("Failed to initialize SDL audio: {}", SDL_GetError());
      return false;
   }

   mSoundDriver = new DecafSDLSound;
   return true;
}

bool
DecafSDL::run(const std::string &gamePath)
{
   auto shouldQuit = false;

   // Setup some basic window stuff
   mWindow =
      SDL_CreateWindow("Decaf",
                       SDL_WINDOWPOS_UNDEFINED,
                       SDL_WINDOWPOS_UNDEFINED,
                       WindowWidth, WindowHeight,
                       SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);
   setWindowIcon(mWindow);

   if (gpu::config()->display.screenMode == gpu::DisplaySettings::Fullscreen) {
      SDL_SetWindowFullscreen(mWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
   }

   // Setup graphics driver
   auto wsi = gpu::WindowSystemInfo { };
   auto sysWmInfo = SDL_SysWMinfo { };

   SDL_GetWindowWMInfo(mWindow, &sysWmInfo);
   switch (sysWmInfo.subsystem) {
#ifdef SDL_VIDEO_DRIVER_WINDOWS
   case SDL_SYSWM_WINDOWS:
      wsi.type = gpu::WindowSystemType::Windows;
      wsi.renderSurface = static_cast<void *>(sysWmInfo.info.win.window);
      break;
#endif
#ifdef SDL_VIDEO_DRIVER_X11
   case SDL_SYSWM_X11:
      wsi.type = gpu::WindowSystemType::X11;
      wsi.renderSurface = static_cast<void *>(sysWmInfo.info.x11.window);
      wsi.displayConnection = static_cast<void *>(sysWmInfo.info.x11.display);
      break;
#endif
#ifdef SDL_VIDEO_DRIVER_COCOA
   case SDL_SYSWM_COCOA:
      wsi.type = gpu::WindowSystemType::Cocoa;
      wsi.renderSurface = static_cast<void *>(sysWmInfo.info.cocoa.window);
      break;
#endif
#ifdef SDL_VIDEO_DRIVER_WAYLAND
   case SDL_SYSWM_WAYLAND:
      wsi.type = gpu::WindowSystemType::Wayland;
      wsi.renderSurface = static_cast<void *>(sysWmInfo.info.wl.surface);
      wsi.displayConnection = static_cast<void *>(sysWmInfo.info.wl.display);
      break;
#endif
   default:
      decaf_abort(fmt::format("Unsupported SDL window subsystem %d", sysWmInfo.subsystem));
   }

   mGraphicsDriver->setWindowSystemInfo(wsi);
   mGraphicsThread = std::thread { [this]() { mGraphicsDriver->run(); } };
   decaf::setGraphicsDriver(mGraphicsDriver);

   // Set input provider
   decaf::setInputDriver(this);
   decaf::addEventListener(this);
   openInputDevices();

   // Set sound driver
   decaf::setSoundDriver(mSoundDriver);

   // Initialise emulator
   if (!decaf::initialise(gamePath)) {
      return false;
   }

   // Start emulator
   decaf::start();

   while (!shouldQuit && !decaf::hasExited()) {
      if (mVpad0Controller) {
         SDL_GameControllerUpdate();
      }

      auto event = SDL_Event { };
      while (SDL_PollEvent(&event)) {
         switch (event.type) {
         case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
               shouldQuit = true;
            } else if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
               mGraphicsDriver->windowSizeChanged(event.window.data1, event.window.data2);
            }

            break;
         case SDL_KEYUP:
            if (event.key.keysym.sym == SDLK_TAB) {
               mToggleDRC = !mToggleDRC;
            }

            if (event.key.keysym.sym == SDLK_ESCAPE) {
               shouldQuit = true;
            }
            break;
         case SDL_QUIT:
            shouldQuit = true;
            break;
         }
      }
   }

   // Shut down decaf
   decaf::shutdown();

   // Shut down graphics
   mGraphicsDriver->stop();
   mGraphicsThread.join();

   return true;
}

void
DecafSDL::onGameLoaded(const decaf::GameInfo &info)
{
   fmt::memory_buffer title;
   fmt::format_to(title, "decaf-sdl ({})", sActiveGfx);

   if (info.titleId) {
      fmt::format_to(title, " {:016X}", info.titleId);
   }

   if (!info.executable.empty()) {
      fmt::format_to(title, " {}", info.executable);
   }

   auto titleStr = std::string { title.data(), title.size() };
   SDL_SetWindowTitle(mWindow, titleStr.c_str());

   // We have to be careful not to start rendering until the game is
   // fully loaded, or we will block window messaging.
   mGameLoaded = true;
}
