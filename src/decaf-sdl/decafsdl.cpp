#include "clilog.h"
#include "decafsdl.h"

DecafSDL::~DecafSDL()
{
   if (mContext) {
      SDL_GL_DeleteContext(mContext);
      mContext = nullptr;
   }

   if (mThreadContext) {
      SDL_GL_DeleteContext(mThreadContext);
      mThreadContext = nullptr;
   }

   if (mWindow) {
      SDL_DestroyWindow(mWindow);
      mWindow = nullptr;
   }
}

bool
DecafSDL::createWindow()
{
   if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) != 0) {
      gCliLog->error("Failed to initialize SDL: {}", SDL_GetError());
      return false;
   }

   if (SDL_GL_LoadLibrary(NULL) != 0) {
      gCliLog->error("Failed to load OpenGL library: {}", SDL_GetError());
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

   // Create TV window
   mWindow = SDL_CreateWindow("Decaf",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              WindowWidth, WindowHeight,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);

   if (!mWindow) {
      gCliLog->error("Failed to create TV window: {}", SDL_GetError());
      return false;
   }

   SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);

   // Create OpenGL context
   mContext = SDL_GL_CreateContext(mWindow);

   if (!mContext) {
      gCliLog->error("Failed to create Main OpenGL context: {}", SDL_GetError());
      return false;
   }

   mThreadContext = SDL_GL_CreateContext(mWindow);

   if (!mThreadContext) {
      gCliLog->error("Failed to create GPU OpenGL context: {}", SDL_GetError());
      return false;
   }

   SDL_GL_MakeCurrent(mWindow, mContext);

   return true;
}

bool
DecafSDL::run(const std::string &gamePath)
{
   auto shouldQuit = false;

   // Setup OpenGL graphics driver
   mGraphicsDriver = decaf::createGLDriver();
   decaf::setGraphicsDriver(mGraphicsDriver);

   // Set input provider
   decaf::setInputDriver(this);

   decaf::setClipboardTextCallbacks(
      []() -> const char * {
         return SDL_GetClipboardText();
      },
      [](auto text) {
         SDL_SetClipboardText(text);
      });

   // Initialise emulator
   if (!decaf::initialise(gamePath)) {
      return false;
   }

   decaf::debugger::initialise();

   // Start graphics thread
   mGraphicsThread = std::thread {
      [this]() {
         SDL_GL_MakeCurrent(mWindow, mThreadContext);
         initialiseContext();
         mGraphicsDriver->run();
      } };

   // Start emulator
   decaf::start();

   // Start rendering
   SDL_GL_MakeCurrent(mWindow, mContext);
   SDL_GL_SetSwapInterval(1);
   initialiseContext();
   initialiseDraw();
   decaf::debugger::initialiseUiGL();

   while (!shouldQuit) {
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

      gl::GLuint tvBuffer = 0;
      gl::GLuint drcBuffer = 0;
      mGraphicsDriver->getSwapBuffers(&tvBuffer, &drcBuffer);

      const auto drcRatio = 0.25f;
      const auto overallScale = 0.75f;
      const auto sepGap = 5.0f;

      static const auto DrcWidth = 854.0f;
      static const auto DrcHeight = 480.0f;
      static const auto TvWidth = 1280.0f;
      static const auto TvHeight = 720.0f;

      int windowWidth, windowHeight;
      SDL_GetWindowSize(mWindow, &windowWidth, &windowHeight);

      auto tvWidth = windowWidth * (1.0f - drcRatio) * overallScale;
      auto tvHeight = TvHeight * (tvWidth / TvWidth);
      auto drcWidth = windowWidth * (drcRatio)* overallScale;
      auto drcHeight = DrcHeight * (drcWidth / DrcWidth);
      auto totalWidth = std::max(tvWidth, drcWidth);
      auto totalHeight = tvHeight + drcHeight + sepGap;
      auto baseLeft = (windowWidth / 2) - (totalWidth / 2);
      auto baseBottom = -(windowHeight / 2) + (totalHeight / 2);

      auto tvLeft = 0.0f;
      auto tvBottom = windowHeight - tvHeight;
      auto drcLeft = (tvWidth / 2) - (drcWidth / 2);
      auto drcBottom = windowHeight - tvHeight - drcHeight - sepGap;

      float tvVp[4] = {
         baseLeft + tvLeft,
         baseBottom + tvBottom,
         tvWidth,
         tvHeight
      };

      float drcVp[4] = {
         baseLeft + drcLeft,
         baseBottom + drcBottom,
         drcWidth,
         drcHeight
      };

      // Clear screen
      gl::glClearColor(0.6f, 0.2f, 0.2f, 1.0f);
      gl::glClear(gl::GL_COLOR_BUFFER_BIT);

      // Draw TV display
      gl::glViewportArrayv(0, 1, tvVp);
      drawScanBuffer(tvBuffer);

      // Draw DRC display
      gl::glViewportArrayv(0, 1, drcVp);
      drawScanBuffer(drcBuffer);

      // Draw UI
      decaf::debugger::drawUiGL(windowWidth, windowHeight);

      // Swap
      SDL_GL_SwapWindow(mWindow);
   }

   // Shut down decaf
   decaf::shutdown();

   // Shut down the GPU
   mGraphicsDriver->stop();
   mGraphicsThread.join();
   return true;
}
