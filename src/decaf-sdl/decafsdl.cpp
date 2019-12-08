#include "clilog.h"
#include "config.h"
#include "decafsdl.h"

#include <common/decaf_assert.h>
#include <fmt/core.h>

#include <libgpu/gpu_graphicsdriver.h>
#include <SDL_syswm.h>
#include <thread>

static std::string
sActiveGfx = "NOGFX";

void setWindowIcon(SDL_Window *window);

DecafSDL::~DecafSDL()
{
}

bool
DecafSDL::initCore()
{
   if (SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) != 0) {
      gCliLog->error("Failed to initialize SDL: {}", SDL_GetError());
      return false;
   }

   return true;
}

bool
DecafSDL::initGraphics()
{
   auto videoInitialised = false;

#ifdef SDL_VIDEO_DRIVER_X11
   if (!videoInitialised) {
      videoInitialised = SDL_VideoInit("x11") == 0;
      if (!videoInitialised) {
         gCliLog->error("Failed to initialize SDL Video with x11: {}", SDL_GetError());
      }
   }
#endif

#ifdef SDL_VIDEO_DRIVER_WAYLAND
   if (!videoInitialised) {
      videoInitialised = SDL_VideoInit("wayland") == 0;
      if (!videoInitialised) {
         gCliLog->error("Failed to initialize SDL Video with wayland: {}", SDL_GetError());
      }
   }
#endif

#ifdef SDL_VIDEO_DRIVER_COCOA
   if (!videoInitialised) {
      videoInitialised = SDL_VideoInit("cocoa") == 0;
      if (!videoInitialised) {
         gCliLog->error("Failed to initialize SDL Video with cocoa: {}", SDL_GetError());
      }
   }
#endif

   if (!videoInitialised) {
      if (SDL_VideoInit(NULL) != 0) {
         gCliLog->error("Failed to initialize SDL Video: {}", SDL_GetError());
         return false;
      }
   }

   gCliLog->info("Using SDL video driver {}", SDL_GetCurrentVideoDriver());

   mGraphicsDriver = gpu::createGraphicsDriver();
   if (!mGraphicsDriver) {
      return false;
   }

   switch (mGraphicsDriver->type()) {
   case gpu::GraphicsDriverType::Vulkan:
      sActiveGfx = "Vulkan";
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

static Uint32
windowTitleTimerCallback(Uint32 interval, void *param)
{
   auto event = SDL_Event { 0 };
   event.type = static_cast<Uint32>(reinterpret_cast<uintptr_t>(param));
   event.user.code = 0;
   event.user.data1 = nullptr;
   event.user.data2 = nullptr;
   SDL_PushEvent(&event);
   return interval;
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

   mDecafEventId = SDL_RegisterEvents(2);
   if (mDecafEventId != -1) {
      mUpdateWindowTitleEventId = mDecafEventId + 1;
      mWindowTitleTimerId = SDL_AddTimer(100, &windowTitleTimerCallback,
         reinterpret_cast<void *>(static_cast<uintptr_t>(mUpdateWindowTitleEventId)));
   }

   // Setup graphics driver
   auto wsi = gpu::WindowSystemInfo { };
   auto sysWmInfo = SDL_SysWMinfo { };
   SDL_VERSION(&sysWmInfo.version);
   if (!SDL_GetWindowWMInfo(mWindow, &sysWmInfo)) {
      gCliLog->error("SDL_GetWindowWMInfo failed: {}", SDL_GetError());
   }

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
      wsi.renderSurface = reinterpret_cast<void *>(sysWmInfo.info.x11.window);
      wsi.displayConnection = static_cast<void *>(sysWmInfo.info.x11.display);
      break;
#endif
#ifdef SDL_VIDEO_METAL
   case SDL_SYSWM_COCOA:
      wsi.type = gpu::WindowSystemType::Cocoa;
      wsi.renderSurface =  static_cast<void *>(SDL_Metal_CreateView(mWindow));
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
      decaf_abort(fmt::format("Unsupported SDL window subsystem {}", sysWmInfo.subsystem));
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

   auto event = SDL_Event { };
   while (!shouldQuit && !decaf::stopping() && SDL_WaitEvent(&event)) {
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

      if (event.type == mDecafEventId) {
         auto eventType = static_cast<decaf::EventType>(event.user.code);
         if (eventType == decaf::EventType::GameLoaded) {
            auto info = reinterpret_cast<decaf::GameInfo *>(event.user.data1);
            mGameInfo = *info;
            delete info;
         }
      } else if (event.type == mUpdateWindowTitleEventId) {
         fmt::memory_buffer title;
         fmt::format_to(title, "decaf-sdl -");

         if (mGameInfo.titleId) {
            fmt::format_to(title, " {:08X}-{:08X}",
                           mGameInfo.titleId >> 32,
                           mGameInfo.titleId & 0xFFFFFFFF);
         }

         if (!mGameInfo.executable.empty()) {
            fmt::format_to(title, " {}", mGameInfo.executable);
         }

         fmt::format_to(title, " ({} {} fps)", sActiveGfx,
                        static_cast<int>(mGraphicsDriver->getDebugInfo()->averageFps));
         auto titleStr = std::string { title.data(), title.size() };
         SDL_SetWindowTitle(mWindow, titleStr.c_str());
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
   if (mDecafEventId != -1) {
      auto event = SDL_Event { 0 };
      event.type = mDecafEventId;
      event.user.code = static_cast<Sint32>(decaf::EventType::GameLoaded);
      event.user.data1 = new decaf::GameInfo(info);
      SDL_PushEvent(&event);
   }
}
