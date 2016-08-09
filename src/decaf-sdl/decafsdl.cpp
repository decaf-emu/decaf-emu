#include "clilog.h"
#include "config.h"
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
   if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) != 0) {
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

   // Set to OpenGL 4.5 core profile
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

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

   // Setup OpenGL graphics driver
   mGraphicsDriver = decaf::createGLDriver();
   decaf::setGraphicsDriver(mGraphicsDriver);

   // Set input provider
   decaf::setInputDriver(this);
   decaf::addEventListener(this);

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

   decaf::debugger::initialise();

   // Setup rendering
   SDL_GL_MakeCurrent(mWindow, mContext);
   SDL_GL_SetSwapInterval(1);
   initialiseContext();
   initialiseDraw();
   decaf::debugger::initialiseUiGL();

   // Start graphics thread
   if (!config::gpu::force_sync) {
      mGraphicsThread = std::thread{
         [this]() {
            SDL_GL_MakeCurrent(mWindow, mThreadContext);
            initialiseContext();
            mGraphicsDriver->run();
         } };
   } else {
      // Set the swap interval to 0 so that we don't slow
      //  down the GPU system when presenting...  The game should
      //  throttle our swapping automatically anyways.
      SDL_GL_SetSwapInterval(0);

      // Switch to the thread context, we automatically switch
      //  back when presenting a frame.
      SDL_GL_MakeCurrent(mWindow, mThreadContext);
   }

   // Start emulator
   decaf::start();

   while (!shouldQuit && !decaf::hasExited()) {
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

      if (!config::gpu::force_sync) {
         gl::GLuint tvBuffer = 0;
         gl::GLuint drcBuffer = 0;
         mGraphicsDriver->getSwapBuffers(&tvBuffer, &drcBuffer);
         drawScanBuffers(tvBuffer, drcBuffer);
      } else {
         mGraphicsDriver->syncPoll([&](unsigned int tvBuffer, unsigned int drcBuffer) {
            SDL_GL_MakeCurrent(mWindow, mContext);
            drawScanBuffers(tvBuffer, drcBuffer);
            SDL_GL_MakeCurrent(mWindow, mThreadContext);
         });
      }
   }

   // Shut down decaf
   decaf::shutdown();

   // Shut down the GPU
   if (!config::gpu::force_sync) {
      mGraphicsDriver->stop();
      mGraphicsThread.join();
   }

   return true;
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
   SDL_SetWindowTitle(mWindow, title.c_str());
}
