#include "clilog.h"
#include "common/decaf_assert.h"
#include "config.h"
#include "decafsdl.h"
#include "decafsdl_opengl.h"
#include "decafsdl_dx12.h"

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
#ifndef DECAF_NOGL
   mGraphicsDriver = new DecafSDLOpenGL();

   if (!mGraphicsDriver->initialise(WindowWidth, WindowHeight)) {
      gCliLog->error("Failed to create GL graphics window");
      return false;
   }

   return true;
#else
   decaf_abort("GL support was excluded from this build");
#endif
}

bool
DecafSDL::initDx12Graphics()
{
#ifdef DECAF_DX12
   mGraphicsDriver = new DecafSDLDX12();

   if (!mGraphicsDriver->initialise(WindowWidth, WindowHeight)) {
      gCliLog->error("Failed to create DX12 graphics window");
      return false;
}

   return true;
#else
   decaf_abort("DX12 support was not included in this build");
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

   // Setup OpenGL graphics driver
   auto graphicsDriver = mGraphicsDriver->getDecafDriver();
   decaf::setGraphicsDriver(graphicsDriver);

   // Set input provider
   decaf::setInputDriver(this);
   decaf::addEventListener(this);
   openInputDevices();

   // Set sound driver
   decaf::setSoundDriver(mSoundDriver);

   decaf::setClipboardTextCallbacks(
      []() -> const char * {
         return SDL_GetClipboardText();
      },
      [](const char *text) {
         SDL_SetClipboardText(text);
      });

   // Initialise emulator
   if (!decaf::initialise(gamePath)) {
      return false;
   }

   // Initialise the emulators debugger
   decaf::debugger::initialise();

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

      float tvViewport[4], drcViewport[4];
      calculateScreenViewports(tvViewport, drcViewport);
      mGraphicsDriver->renderFrame(tvViewport, drcViewport);
   }

   // Shut down decaf
   decaf::shutdown();

   // Shut down graphics
   mGraphicsDriver->shutdown();

   return true;
}

void
DecafSDL::calculateScreenViewports(float(&tv)[4], float(&drc)[4])
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

   SDL_GetWindowSize(mGraphicsDriver->getWindow(), &windowWidth, &windowHeight);

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
      // Copy TV size to DRC size
      drcLeft = tvLeft;
      drcRight = tvRight;
      drcTop = tvTop;
      drcBottom = tvBottom;
   }

   tv[0] = static_cast<float>(tvLeft);
   tv[1] = static_cast<float>(tvBottom);
   tv[2] = static_cast<float>(tvRight - tvLeft);
   tv[3] = static_cast<float>(tvTop - tvBottom);

   drc[0] = static_cast<float>(drcLeft);
   drc[1] = static_cast<float>(drcBottom);
   drc[2] = static_cast<float>(drcRight - drcLeft);
   drc[3] = static_cast<float>(drcTop - drcBottom);

   decaf_check(tv[0] >= 0);
   decaf_check(tv[1] >= 0);
   decaf_check(tv[0] + tv[2] <= windowWidth);
   decaf_check(tv[1] + tv[3] <= windowHeight);
   decaf_check(drc[0] >= 0);
   decaf_check(drc[1] >= 0);
   decaf_check(drc[0] + drc[2] <= windowWidth);
   decaf_check(drc[1] + drc[3] <= windowHeight);
}

void
DecafSDL::onGameLoaded(const decaf::GameInfo &info)
{
   auto name = info.meta.shortnames[decaf::Language::English];

   // If we do not have English short name, pick first non-empty short name
   if (name.empty()) {
      for (auto itr : info.meta.shortnames) {
         if (itr.empty()) {
            name = itr;
            break;
         }
      }
   }

   // Try product code
   if (name.empty()) {
      name = info.meta.product_code;
   }

   // Finally use RPX name
   if (name.empty()) {
      name = info.cos.argstr;
   }

   // Update window title
   auto title = fmt::format("Decaf - {}", name);
   SDL_SetWindowTitle(mGraphicsDriver->getWindow(), title.c_str());
}
