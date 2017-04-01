#ifndef DECAF_NOGL

#include "clilog.h"
#include <common/decaf_assert.h>
#include "config.h"
#include "decafsdl_opengl.h"
#include <string>
#include <glbinding/Binding.h>
#include <glbinding/Meta.h>

#include <iostream>

static std::string
getGlDebugSource(gl::GLenum source) {
   switch (source) {
   case gl::GL_DEBUG_SOURCE_API:
      return "API";
   case gl::GL_DEBUG_SOURCE_WINDOW_SYSTEM:
      return "WINSYS";
   case gl::GL_DEBUG_SOURCE_SHADER_COMPILER:
      return "COMPILER";
   case gl::GL_DEBUG_SOURCE_THIRD_PARTY:
      return "EXTERNAL";
   case gl::GL_DEBUG_SOURCE_APPLICATION:
      return "APP";
   case gl::GL_DEBUG_SOURCE_OTHER:
      return "OTHER";
   default:
      return glbinding::Meta::getString(source);
   }
}

static std::string
getGlDebugType(gl::GLenum severity) {
   switch (severity) {
   case gl::GL_DEBUG_TYPE_ERROR:
      return "ERROR";
   case gl::GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
      return "DEPRECATED_BEHAVIOR";
   case gl::GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
      return "UNDEFINED_BEHAVIOR";
   case gl::GL_DEBUG_TYPE_PORTABILITY:
      return "PORTABILITY";
   case gl::GL_DEBUG_TYPE_PERFORMANCE:
      return "PERFORMANCE";
   case gl::GL_DEBUG_TYPE_MARKER:
      return "MARKER";
   case gl::GL_DEBUG_TYPE_PUSH_GROUP:
      return "PUSH_GROUP";
   case gl::GL_DEBUG_TYPE_POP_GROUP:
      return "POP_GROUP";
   case gl::GL_DEBUG_TYPE_OTHER:
      return "OTHER";
   default:
      return glbinding::Meta::getString(severity);
   }
}

static std::string
getGlDebugSeverity(gl::GLenum severity) {
   switch (severity) {
   case gl::GL_DEBUG_SEVERITY_HIGH:
      return "HIGH";
   case gl::GL_DEBUG_SEVERITY_MEDIUM:
      return "MED";
   case gl::GL_DEBUG_SEVERITY_LOW:
      return "LOW";
   case gl::GL_DEBUG_SEVERITY_NOTIFICATION:
      return "NOTIF";
   default:
      return glbinding::Meta::getString(severity);
   }
}

static void GL_APIENTRY
debugMessageCallback(gl::GLenum source, gl::GLenum type, gl::GLuint id, gl::GLenum severity,
   gl::GLsizei length, const gl::GLchar* message, const void *userParam) {
   for (auto filterID : decaf::config::gpu::debug_filters) {
      if (filterID == id) {
         return;
      }
   }

   auto outputStr = fmt::format("GL Message ({}, {}, {}, {}) {}", id,
      getGlDebugSource(source),
      getGlDebugType(type),
      getGlDebugSeverity(severity),
      message);

   if (severity == gl::GL_DEBUG_SEVERITY_HIGH) {
      gCliLog->warn(outputStr);
   } else if (severity == gl::GL_DEBUG_SEVERITY_MEDIUM) {
      gCliLog->debug(outputStr);
   } else if (severity == gl::GL_DEBUG_SEVERITY_LOW) {
      gCliLog->trace(outputStr);
   } else if (severity == gl::GL_DEBUG_SEVERITY_NOTIFICATION) {
      gCliLog->info(outputStr);
   } else {
      gCliLog->info(outputStr);
   }
}

DecafSDLOpenGL::DecafSDLOpenGL() {
   using decaf::config::ui::background_colour;

   mBackgroundColour[0] = background_colour.r / 255.0;
   mBackgroundColour[1] = background_colour.g / 255.0;
   mBackgroundColour[2] = background_colour.b / 255.0;
}

DecafSDLOpenGL::~DecafSDLOpenGL() {
   if (mContext) {
      SDL_GL_DeleteContext(mContext);
      mContext = nullptr;
   }

   if (mThreadContext) {
      SDL_GL_DeleteContext(mThreadContext);
      mThreadContext = nullptr;
   }

   if (mContextDRC) {
      SDL_GL_DeleteContext(mContextDRC);
      mContext = nullptr;
   }

   if (mThreadContextDRC) {
      SDL_GL_DeleteContext(mThreadContextDRC);
      mContext = nullptr;
   }
}

void
DecafSDLOpenGL::initialiseContext() {
   glbinding::Binding::initialize();
   glbinding::setCallbackMaskExcept(glbinding::CallbackMask::After | glbinding::CallbackMask::ParametersAndReturnValue, { "glGetError" });
   glbinding::setAfterCallback([] (const glbinding::FunctionCall &call) {
      auto error = glbinding::Binding::GetError.directCall();

      if (error != gl::GL_NO_ERROR) {
         fmt::MemoryWriter writer;
         writer << call.function->name() << "(";

         for (unsigned i = 0; i < call.parameters.size(); ++i) {
            writer << call.parameters[i]->asString();
            if (i < call.parameters.size() - 1)
               writer << ", ";
         }

         writer << ")";

         if (call.returnValue) {
            writer << " -> " << call.returnValue->asString();
         }

         gCliLog->error("OpenGL error: {} with {}", glbinding::Meta::getString(error), writer.str());
      }
   });

   if (decaf::config::gpu::debug) {
      gl::glDebugMessageCallback(&debugMessageCallback, nullptr);
      gl::glEnable(gl::GL_DEBUG_OUTPUT);
      gl::glEnable(gl::GL_DEBUG_OUTPUT_SYNCHRONOUS);
   }
}

void
DecafSDLOpenGL::initialiseDraw() {
   static auto vertexCode = R"(
      #version 420 core
      in vec2 fs_position;
      in vec2 fs_texCoord;
      out vec2 vs_texCoord;

      out gl_PerVertex {
         vec4 gl_Position;
      };

      void main()
      {
         vs_texCoord = fs_texCoord;
         gl_Position = vec4(fs_position, 0.0, 1.0);
      })";

   static auto pixelCode = R"(
      #version 420 core
      in vec2 vs_texCoord;
      out vec4 ps_color;
      uniform sampler2D sampler_0;

      void main()
      {
         ps_color = texture(sampler_0, vs_texCoord);
      })";

   // Create vertex program
   mVertexProgram = gl::glCreateShaderProgramv(gl::GL_VERTEX_SHADER, 1, &vertexCode);

   // Create pixel program
   mPixelProgram = gl::glCreateShaderProgramv(gl::GL_FRAGMENT_SHADER, 1, &pixelCode);
   gl::glBindFragDataLocation(mPixelProgram, 0, "ps_color");

   // Create pipeline
   gl::glGenProgramPipelines(1, &mPipeline);
   gl::glUseProgramStages(mPipeline, gl::GL_VERTEX_SHADER_BIT, mVertexProgram);
   gl::glUseProgramStages(mPipeline, gl::GL_FRAGMENT_SHADER_BIT, mPixelProgram);

   // (TL, TR, BR)    (BR, BL, TL)
   // Create vertex buffer
   static const gl::GLfloat vertices[] = {
      -1.0f,  -1.0f,   0.0f, 1.0f,
      1.0f,  -1.0f,   1.0f, 1.0f,
      1.0f, 1.0f,   1.0f, 0.0f,

      1.0f, 1.0f,   1.0f, 0.0f,
      -1.0f, 1.0f,   0.0f, 0.0f,
      -1.0f,  -1.0f,   0.0f, 1.0f,
   };

   gl::glCreateBuffers(1, &mVertBuffer);
   gl::glNamedBufferData(mVertBuffer, sizeof(vertices), vertices, gl::GL_STATIC_DRAW);

   // Create vertex array
   gl::glCreateVertexArrays(1, &mVertArray);

   auto fs_position = gl::glGetAttribLocation(mVertexProgram, "fs_position");
   gl::glEnableVertexArrayAttrib(mVertArray, fs_position);
   gl::glVertexArrayAttribFormat(mVertArray, fs_position, 2, gl::GL_FLOAT, gl::GL_FALSE, 0);
   gl::glVertexArrayAttribBinding(mVertArray, fs_position, 0);

   auto fs_texCoord = gl::glGetAttribLocation(mVertexProgram, "fs_texCoord");
   gl::glEnableVertexArrayAttrib(mVertArray, fs_texCoord);
   gl::glVertexArrayAttribFormat(mVertArray, fs_texCoord, 2, gl::GL_FLOAT, gl::GL_FALSE, 2 * sizeof(gl::GLfloat));
   gl::glVertexArrayAttribBinding(mVertArray, fs_texCoord, 0);

   // Create texture sampler
   gl::glGenSamplers(1, &mSampler);

   gl::glSamplerParameteri(mSampler, gl::GL_TEXTURE_WRAP_S, static_cast<int>(gl::GL_CLAMP));
   gl::glSamplerParameteri(mSampler, gl::GL_TEXTURE_WRAP_S, static_cast<int>(gl::GL_CLAMP));
   gl::glSamplerParameteri(mSampler, gl::GL_TEXTURE_MIN_FILTER, static_cast<int>(gl::GL_LINEAR));
   gl::glSamplerParameteri(mSampler, gl::GL_TEXTURE_MAG_FILTER, static_cast<int>(gl::GL_LINEAR));
}

void
DecafSDLOpenGL::drawScanBuffer(gl::GLuint object) {
   // Setup screen draw shader
   gl::glBindVertexArray(mVertArray);
   gl::glBindVertexBuffer(0, mVertBuffer, 0, 4 * sizeof(gl::GLfloat));
   gl::glBindProgramPipeline(mPipeline);

   // Draw screen quad
   gl::glBindSampler(0, mSampler);
   gl::glBindTextureUnit(0, object);

   gl::glDrawArrays(gl::GL_TRIANGLES, 0, 6);
}

static const float ASPECT_RATIO = 16.0f / 9;

void
calculateAndExecuteViewportArray(int width, int height) {
   // Variables for the viewport array
   int vpWidth = 0, vpHeight = 0,
      xPadding = 0, yPadding = 0;

   // Check if the user has set the
   // stretch option, if so just
   // return the width and height
   if (config::display::stretch) {
      xPadding = 0;
      yPadding = 0;

      vpWidth  = width;
      vpHeight = height;
   } 
   
   // Apply jroweboy_'s aspect ratio fix
   else {
      float scale = std::min(static_cast<float>(width), height * ASPECT_RATIO);
      vpWidth     = static_cast<int>(std::round(scale));
      vpHeight    = static_cast<int>(std::round(scale / ASPECT_RATIO));
      xPadding    = (width - vpWidth) / 2;
      yPadding    = (height - vpHeight) / 2;
   }

   // Setup the viewport array
   float viewportArea[] = {
      xPadding, yPadding,
      vpWidth,  vpHeight
   };

   // Set the viewport
   gl::glViewportArrayv(0, 1, viewportArea);
}

void
DecafSDLOpenGL::drawScanBuffer_MW(gl::GLuint screenBuffer,
   gl::GLuint drcBuffer) {

   /* Screen */ {
      // Variables
      int width = 0, height = 0;

      // Setup the window and context
      SDL_GL_MakeCurrent(mWindow, mContext);

      // Clear screen
      gl::glClearColor(mBackgroundColour[0], mBackgroundColour[1], mBackgroundColour[2], 1.0f);
      gl::glClear(gl::GL_COLOR_BUFFER_BIT);

      // Get current dimensions
      SDL_GL_GetDrawableSize(mWindow, &width, &height);

      // Calculate viewport
      calculateAndExecuteViewportArray(width, height);

      // Render
      drawScanBuffer(screenBuffer);

      // Draw UI
      decaf::debugger::drawUiGL(width, height);

      // Swap the window
      SDL_GL_SwapWindow(mWindow);
   }

   /* DRC */  {
      // Variables
      int width = 0, height = 0;

      // Setup the window and context
      SDL_GL_MakeCurrent(mWindowDRC, mContextDRC);

      // Clear screen
      gl::glClearColor(mBackgroundColour[0], mBackgroundColour[1], mBackgroundColour[2], 1.0f);
      gl::glClear(gl::GL_COLOR_BUFFER_BIT);

      // Get current dimensions
      SDL_GL_GetDrawableSize(mWindowDRC, &width, &height);

      // Calculate viewport
      calculateAndExecuteViewportArray(width, height);

      // Render
      drawScanBuffer(drcBuffer);

      // Draw UI
      decaf::debugger::drawUiGL(width, height);

      // Swap the window
      SDL_GL_SwapWindow(mWindowDRC);
   }
}

void
DecafSDLOpenGL::drawScanBuffers(Viewport &tvViewport,
   gl::GLuint tvBuffer,
   Viewport &drcViewport,
   gl::GLuint drcBuffer) {

   // Clear screen
   gl::glClearColor(mBackgroundColour[0], mBackgroundColour[1], mBackgroundColour[2], 1.0f);
   gl::glClear(gl::GL_COLOR_BUFFER_BIT);

   // Draw displays
   auto drawTV = tvViewport.width > 0 && tvViewport.height > 0;
   auto drawDRC = drcViewport.width > 0 && drcViewport.height > 0;

   if (drawTV) {
      float viewportArray[] = {
          tvViewport.x, tvViewport.y,
          tvViewport.width, tvViewport.height
      };

      gl::glViewportArrayv(0, 1, viewportArray);
      drawScanBuffer(tvBuffer);
   }

   if (drawDRC) {
      float viewportArray[] = {
          drcViewport.x, drcViewport.y,
          drcViewport.width, drcViewport.height
      };

      gl::glViewportArrayv(0, 1, viewportArray);
      drawScanBuffer(drcBuffer);
   }

   // Draw UI
   int width, height;
   SDL_GetWindowSize(mWindow, &width, &height);
   decaf::debugger::drawUiGL(width, height);

   // Swap
   SDL_GL_SwapWindow(mWindow);
}

bool
DecafSDLOpenGL::initialise(int width, int height) {
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

   // Enable debug context
   if (decaf::config::gpu::debug) {
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
   }

   // Check if we are splitting the DRC and Screen into
   // seperate windows
   bool multi_window = config::display::mode == config::display::Popup;

   if (multi_window) {
      mWindowDRC = SDL_CreateWindow("Decaf DRC (Gamepad)",
         SDL_WINDOWPOS_UNDEFINED,
         SDL_WINDOWPOS_UNDEFINED,
         width / 2, height / 2,
         SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);

      if (!mWindowDRC) {
         gCliLog->error("Failed to create GL SDL window (DRC)");
         return false;
      }
   }

   // Create TV window
   mWindow = SDL_CreateWindow("Decaf",
      SDL_WINDOWPOS_UNDEFINED,
      SDL_WINDOWPOS_UNDEFINED,
      width, height,
      SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);

   if (!mWindow) {
      gCliLog->error("Failed to create GL SDL window");
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

   // Check if display is on popup mode
   if (config::display::mode == config::display::Popup) {
      // DRC
      mContextDRC = SDL_GL_CreateContext(mWindowDRC);

      if (!mContextDRC) {
         gCliLog->error("Failed to create Main OpenGL context, DRC: {}", SDL_GetError());
         return false;
      }

      mThreadContextDRC = SDL_GL_CreateContext(mWindowDRC);

      if (!mThreadContextDRC) {
         gCliLog->error("Failed to create GPU OpenGL context, DRC: {}", SDL_GetError());
         return false;
      }
   }

   SDL_GL_MakeCurrent(mWindow, mContext);

   // Setup decaf driver
   auto glDriver = decaf::createGLDriver();
   decaf_check(glDriver);
   mDecafDriver = reinterpret_cast<decaf::OpenGLDriver*>(glDriver);

   // Setup rendering
   if (config::display::mode == config::display::Popup) {
      SDL_GL_MakeCurrent(mWindowDRC, mContextDRC);

      initialiseContext();
      initialiseDraw();

      decaf::debugger::initialiseUiGL();

      SDL_GL_MakeCurrent(mWindowDRC, mContextDRC);

      SDL_GL_MakeCurrent(mWindow, mContext);
   }

   initialiseContext();
   initialiseDraw();

   decaf::debugger::initialiseUiGL();

   // Start graphics thread
   if (!config::gpu::force_sync) {
      SDL_GL_SetSwapInterval(1);

      mGraphicsThread = std::thread {
         [this]() {

          if (config::display::mode == config::display::Popup) {
              SDL_GL_MakeCurrent(mWindowDRC, mThreadContextDRC);
              initialiseContext();
          }

         SDL_GL_MakeCurrent(mWindow, mThreadContext);
         initialiseContext();

         mDecafDriver->run();
      } };
   } else {
      // Set the swap interval to 0 so that we don't slow
      //  down the GPU system when presenting...  The game should
      //  throttle our swapping automatically anyways.
      SDL_GL_SetSwapInterval(0);

      // Switch to the thread context, we automatically switch
      //  back when presenting a frame.
      SDL_GL_MakeCurrent(mWindow, mThreadContext);

      // Initialise the context
      initialiseContext();

      if (config::display::mode == config::display::Popup) {
         // Setup the drc context
         SDL_GL_MakeCurrent(mWindowDRC, mThreadContextDRC);

         // Init the context
         initialiseContext();
      }
   }

   return true;
}

void
DecafSDLOpenGL::shutdown() {
   // Shut down the GPU
   if (!config::gpu::force_sync) {
      mDecafDriver->stop();
      mGraphicsThread.join();
   }
}

void
DecafSDLOpenGL::renderFrame(Viewport &tv, Viewport &drc) {
   // May not work if force_sync is disabled
   if (!config::gpu::force_sync) {
      gl::GLuint tvBuffer = 0;
      gl::GLuint drcBuffer = 0;

      mDecafDriver->getSwapBuffers(&tvBuffer, &drcBuffer);

      if (config::display::mode == config::display::Popup) {
         drawScanBuffer_MW(tvBuffer, drcBuffer);
      } else {
         drawScanBuffers(tv, tvBuffer, drc, drcBuffer);
      }
   } else {
      mDecafDriver->syncPoll([&] (unsigned int tvBuffer, unsigned int drcBuffer) {
         if (config::display::mode == config::display::Popup) {
            drawScanBuffer_MW(tvBuffer, drcBuffer);

            SDL_GL_MakeCurrent(mWindow, mThreadContext);
         } else {
            SDL_GL_MakeCurrent(mWindow, mContext);
            drawScanBuffers(tv, tvBuffer, drc, drcBuffer);
            SDL_GL_MakeCurrent(mWindow, mThreadContext);
         }

      });
   }
}

decaf::GraphicsDriver *
DecafSDLOpenGL::getDecafDriver() {
   return mDecafDriver;
}

#endif // DECAF_NOGL
