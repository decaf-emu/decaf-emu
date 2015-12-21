#include "platform_sdl.h"
#include "platform_ui.h"
#include "gpu/driver.h"
#include "utils/log.h"
#include <SDL.h>

static const auto DrcWidth = 854.0f;
static const auto DrcHeight = 480.0f;
static const auto DrcScaleFactor = 0.5f;

static const auto TvWidth = 1280.0f;
static const auto TvHeight = 720.0f;
static const auto TvScaleFactor = 0.65f;

static SDL_GLContext tvContext = nullptr;
static SDL_GLContext drcContext = nullptr;

namespace platform
{

namespace sdl
{

bool shouldQuit = false;

SDL_Window *tvWindow = nullptr;
// TODO: Separate window not yet implemented.
SDL_Window *drcWindow = nullptr;

} // namespace sdl

namespace ui
{

bool
init()
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
createWindow(const std::string &title)
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

   uint32_t flags = SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI;
   sdl::tvWindow = SDL_CreateWindow(
      title.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
      getWindowWidth(), getWindowHeight(), flags);
   if (!sdl::tvWindow) {
      gLog->error("Failed to create TV window: {}", SDL_GetError());
      return false;
   }

   tvContext = SDL_GL_CreateContext(sdl::tvWindow);
   if (!tvContext) {
      gLog->error("Failed to create OpenGL context for TV window: {}",
                  SDL_GetError());
      SDL_DestroyWindow(sdl::tvWindow);
      sdl::tvWindow = nullptr;
      return false;
   }

   return true;
}

void
run()
{
   while (!sdl::shouldQuit) {
      SDL_Event event;
      if (!SDL_WaitEvent(&event)) {
         gLog->error("Error waiting for event: {}", SDL_GetError());
         continue;
      }
      sdl::handleEvent(&event);
   }
}

void
shutdown()
{
   SDL_Quit();
}

void
swapBuffers()
{
   SDL_GL_SwapWindow(sdl::tvWindow);
}

void
activateContext()
{
   SDL_GL_MakeCurrent(sdl::tvWindow, tvContext);
}

void
releaseContext()
{
   SDL_GL_MakeCurrent(sdl::tvWindow, nullptr);
}

int
getWindowWidth()
{
   return std::max(getTvWidth(), getDrcWidth());
}

int
getWindowHeight()
{
   return getTvHeight() + getDrcHeight();
}

int
getDrcWidth()
{
   return static_cast<int>(854.0f * DrcScaleFactor);
}

int
getDrcHeight()
{
   return static_cast<int>(480.0f * DrcScaleFactor);
}

int
getTvWidth()
{
   return static_cast<int>(1280.0f * TvScaleFactor);
}

int
getTvHeight()
{
   return static_cast<int>(720.0f * TvScaleFactor);
}

} // namespace ui

} // namespace platform
