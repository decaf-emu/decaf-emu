#include "sdl_window.h"
#include "clilog.h"
#include "config.h"

#include "replay_ringbuffer.h"
#include "replay_parser_pm4.h"

#include <array>
#include <fstream>
#include <common/log.h>
#include <common/platform_dir.h>
#include <libgpu/gpu_config.h>
#include <SDL_syswm.h>

using namespace latte::pm4;

SDLWindow::~SDLWindow()
{
}

bool
SDLWindow::initCore()
{
   if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) != 0) {
      gCliLog->error("Failed to initialize SDL: {}", SDL_GetError());
      return false;
   }

   return true;
}

bool
SDLWindow::initGraphics()
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
      mRendererName = "Vulkan";
      break;
   case gpu::GraphicsDriverType::OpenGL:
      mRendererName = "OpenGL";
      break;
   case gpu::GraphicsDriverType::Null:
      mRendererName = "Null";
      break;
   default:
      mRendererName = "Unknown";
   }

   return true;
}

bool
SDLWindow::run(const std::string &tracePath)
{
   auto shouldQuit = false;

   // Setup some basic window stuff
   mWindow =
      SDL_CreateWindow("pm4-replay",
                       SDL_WINDOWPOS_UNDEFINED,
                       SDL_WINDOWPOS_UNDEFINED,
                       WindowWidth, WindowHeight,
                       SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);

   if (gpu::config()->display.screenMode == gpu::DisplaySettings::Fullscreen) {
      SDL_SetWindowFullscreen(mWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
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
      decaf_abort(fmt::format("Unsupported SDL window subsystem {}", sysWmInfo.subsystem));
   }

   mGraphicsDriver->setWindowSystemInfo(wsi);

   // Setup replay parser
   auto replayHeap = phys_cast<cafe::TinyHeapPhysical *>(phys_addr { 0x34000000 });
   cafe::TinyHeap_Setup(replayHeap,
                        0x430,
                        phys_cast<void *>(phys_addr { 0x34000000 + 0x430 }),
                        0x1C000000 - 0x430);
   auto ringBuffer = std::make_unique<RingBuffer>(replayHeap);
   auto parser = ReplayParserPM4::Create(mGraphicsDriver, ringBuffer.get(), replayHeap, tracePath);
   if (!parser) {
      return false;
   }

   // Set swap interval to 1 otherwise frames will render super fast!
   SDL_GL_SetSwapInterval(1);

   while (!shouldQuit) {
      auto event = SDL_Event { };

      while (SDL_PollEvent(&event)) {
         switch (event.type) {
         case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
               shouldQuit = true;
            }

            break;
         case SDL_KEYUP:
            if (event.key.keysym.sym == SDLK_ESCAPE) {
               shouldQuit = true;
            }
            break;
         case SDL_QUIT:
            shouldQuit = true;
            break;
         }
      }

      if (!parser->readFrame()) {
         shouldQuit = true;
         break;
      }

      mGraphicsDriver->runUntilFlip();
   }

   return true;
}
