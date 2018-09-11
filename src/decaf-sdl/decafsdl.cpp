#include "clilog.h"
#include "config.h"
#include "decafsdl.h"
#include "decafsdl_opengl.h"
#include "decafsdl_vulkan.h"

#include <common/decaf_assert.h>
#include <fmt/format.h>

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
DecafSDL::initGlGraphics()
{
#ifdef DECAF_GL
   mGraphicsDriver = new DecafSDLOpenGL();

   if (!mGraphicsDriver->initialise(WindowWidth, WindowHeight)) {
      gCliLog->error("Failed to create GL graphics window");
      return false;
   }

   sActiveGfx = "GL";
   return true;
#else
   decaf_abort("GL support was not included in this build");
#endif
}

bool
DecafSDL::initVulkanGraphics()
{
#ifdef DECAF_VULKAN
   mGraphicsDriver = new DecafSDLVulkan();

   if (!mGraphicsDriver->initialise(WindowWidth, WindowHeight)) {
      gCliLog->error("Failed to create Vulkan graphics window");
      return false;
   }

   sActiveGfx = "Vulkan";
   return true;
#else
   decaf_abort("Vulkan support was not included in this build");
#endif
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
   auto window = mGraphicsDriver->getWindow();

   setWindowIcon(window);

   if (config::display::mode == config::display::Fullscreen) {
      SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
   }

   // Setup graphics driver
   auto graphicsDriver = mGraphicsDriver->getDecafDriver();
   decaf::setGraphicsDriver(graphicsDriver);

   auto debugUiRenderer = mGraphicsDriver->getDecafDebugUiRenderer();
   decaf::setDebugUiRenderer(debugUiRenderer);

   // Set input provider
   decaf::setInputDriver(this);
   decaf::addEventListener(this);
   openInputDevices();

   // Set sound driver
   decaf::setSoundDriver(mSoundDriver);

   decaf::setClipboardTextCallbacks(
      [](void *context) -> const char * {
         return SDL_GetClipboardText();
      },
      [](void *context, const char *text) {
         SDL_SetClipboardText(text);
      });

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

      SDL_Event event;

      while (SDL_PollEvent(&event)) {
         switch (event.type) {
         case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
               shouldQuit = true;
            } else if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
               mGraphicsDriver->windowResized();
            }

            break;
         case SDL_MOUSEBUTTONDOWN:
            decaf::injectMouseButtonInput(translateMouseButton(event.button.button), decaf::input::MouseAction::Press);
            break;
         case SDL_MOUSEBUTTONUP:
            decaf::injectMouseButtonInput(translateMouseButton(event.button.button), decaf::input::MouseAction::Release);
            break;
         case SDL_MOUSEWHEEL:
            decaf::injectScrollInput(static_cast<float>(event.wheel.x), static_cast<float>(event.wheel.y));
            break;
         case SDL_MOUSEMOTION:
            decaf::injectMousePos(static_cast<float>(event.motion.x), static_cast<float>(event.motion.y));
            break;
         case SDL_KEYDOWN:
            decaf::injectKeyInput(translateKeyCode(event.key.keysym), decaf::input::KeyboardAction::Press);
            break;
         case SDL_KEYUP:
            if (event.key.keysym.sym == SDLK_TAB) {
               mToggleDRC = !mToggleDRC;
            }

            if (event.key.keysym.sym == SDLK_ESCAPE) {
               shouldQuit = true;
            }

            decaf::injectKeyInput(translateKeyCode(event.key.keysym), decaf::input::KeyboardAction::Release);
            break;
         case SDL_TEXTINPUT:
            decaf::injectTextInput(event.text.text);
            break;
         case SDL_QUIT:
            shouldQuit = true;
            break;
         }
      }

      if (mGameLoaded) {
         Viewport tvViewport, drcViewport;
         calculateScreenViewports(tvViewport, drcViewport);
         mGraphicsDriver->renderFrame(tvViewport, drcViewport);
      }
   }

   // Shut down decaf
   decaf::shutdown();

   // Shut down graphics
   mGraphicsDriver->shutdown();

   return true;
}

void
DecafSDL::calculateScreenViewports(Viewport &tv, Viewport &drc)
{
   int TvWidth = 1280;
   int TvHeight = 720;

   int DrcWidth = 854;
   int DrcHeight = 480;

   int OuterBorder = 0;
   int ScreenSeparation = 5;

   int windowWidth, windowHeight;
   int nativeHeight, nativeWidth;
   int tvLeft, tvBottom, tvTop, tvRight;
   int drcLeft, drcBottom, drcTop, drcRight;

   auto tvVisible = true;
   auto drcVisible = true;

   mGraphicsDriver->getWindowSize(&windowWidth, &windowHeight);

   if (config::display::layout == config::display::Toggle) {
      // For toggle mode only one screen is visible at a time, so we calculate the
      // screen position as if only the TV exists here
      nativeHeight = TvHeight;
      nativeWidth = TvWidth;
      DrcWidth = 0;
      DrcHeight = 0;
      ScreenSeparation = 0;
   } else {
      nativeHeight = DrcHeight + TvHeight + ScreenSeparation + 2 * OuterBorder;
      nativeWidth = std::max(DrcWidth, TvWidth) + 2 * OuterBorder;
   }

   if (windowWidth * nativeHeight >= windowHeight * nativeWidth) {
      // Align to height
      int drcBorder = (windowWidth * nativeHeight - windowHeight * DrcWidth + nativeHeight) / nativeHeight / 2;
      int tvBorder = config::display::stretch ? 0 : (windowWidth * nativeHeight - windowHeight * TvWidth + nativeHeight) / nativeHeight / 2;

      drcBottom = OuterBorder;
      drcTop = OuterBorder + (DrcHeight * windowHeight + nativeHeight / 2) / nativeHeight;
      drcLeft = drcBorder;
      drcRight = windowWidth - drcBorder;

      tvBottom = windowHeight - OuterBorder - (TvHeight * windowHeight + nativeHeight / 2) / nativeHeight;
      tvTop = windowHeight - OuterBorder;
      tvLeft = tvBorder;
      tvRight = windowWidth - tvBorder;
   } else {
      // Align to width
      int heightBorder = (windowHeight * nativeWidth - windowWidth * (DrcHeight + TvHeight + ScreenSeparation) + nativeWidth) / nativeWidth / 2;
      int drcBorder = (windowWidth - DrcWidth * windowWidth / nativeWidth + 1) / 2;
      int tvBorder = (windowWidth - TvWidth * windowWidth / nativeWidth + 1) / 2;

      drcBottom = heightBorder;
      drcTop = heightBorder + (DrcHeight * windowWidth + nativeWidth / 2) / nativeWidth;
      drcLeft = drcBorder;
      drcRight = windowWidth - drcBorder;

      tvTop = windowHeight - heightBorder;
      tvBottom = windowHeight - heightBorder - (TvHeight * windowWidth + nativeWidth / 2) / nativeWidth;
      tvLeft = tvBorder;
      tvRight = windowWidth - tvBorder;
   }

   if (config::display::layout == config::display::Toggle) {
      // In toggle mode, DRC and TV size are the same
      drcLeft = tvLeft;
      drcRight = tvRight;
      drcTop = tvTop;
      drcBottom = tvBottom;

      if (mToggleDRC) {
         drcVisible = true;
         tvVisible = false;
      } else {
         drcVisible = false;
         tvVisible = true;
      }
   }

   if (drcVisible) {
      drc.x = static_cast<float>(drcLeft);
      drc.y = static_cast<float>(drcBottom);
      drc.width = static_cast<float>(drcRight - drcLeft);
      drc.height = static_cast<float>(drcTop - drcBottom);
   } else {
      drc.x = 0.0f;
      drc.y = 0.0f;
      drc.width = 0.0f;
      drc.height = 0.0f;
   }

   if (tvVisible) {
      tv.x = static_cast<float>(tvLeft);
      tv.y = static_cast<float>(tvBottom);
      tv.width = static_cast<float>(tvRight - tvLeft);
      tv.height = static_cast<float>(tvTop - tvBottom);
   } else {
      tv.x = 0.0f;
      tv.y = 0.0f;
      tv.width = 0.0f;
      tv.height = 0.0f;
   }

   decaf_check(drc.x >= 0);
   decaf_check(drc.y >= 0);
   decaf_check(drc.x + drc.width <= windowWidth);
   decaf_check(drc.y + drc.height <= windowHeight);
   decaf_check(tv.x >= 0);
   decaf_check(tv.y >= 0);
   decaf_check(tv.x + tv.width <= windowWidth);
   decaf_check(tv.y + tv.height <= windowHeight);
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
   SDL_SetWindowTitle(mGraphicsDriver->getWindow(), titleStr.c_str());

   // We have to be careful not to start rendering until the game is
   // fully loaded, or we will block window messaging.
   mGameLoaded = true;
}
