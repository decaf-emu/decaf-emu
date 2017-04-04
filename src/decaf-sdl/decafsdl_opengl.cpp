#ifndef DECAF_NOGL

#include "clilog.h"
#include <common/decaf_assert.h>
#include <common/gl.h>
#include <common/platform_opengl.h>
#include "config.h"
#include "decafsdl_opengl.h"
#include <string>

#ifdef DECAF_GLBINDING
#include <glbinding/Binding.h>
#include <glbinding/Meta.h>
#endif

static std::string
getGlDebugSource(GLenum source)
{
   switch (source) {
   case GL_DEBUG_SOURCE_API:
      return "API";
   case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
      return "WINSYS";
   case GL_DEBUG_SOURCE_SHADER_COMPILER:
      return "COMPILER";
   case GL_DEBUG_SOURCE_THIRD_PARTY:
      return "EXTERNAL";
   case GL_DEBUG_SOURCE_APPLICATION:
      return "APP";
   case GL_DEBUG_SOURCE_OTHER:
      return "OTHER";
   default:
#ifdef DECAF_GLBINDING
      return glbinding::Meta::getString(source);
#else
      return "???";
#endif
   }
}

static std::string
getGlDebugType(GLenum severity)
{
   switch (severity) {
   case GL_DEBUG_TYPE_ERROR:
      return "ERROR";
   case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
      return "DEPRECATED_BEHAVIOR";
   case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
      return "UNDEFINED_BEHAVIOR";
   case GL_DEBUG_TYPE_PORTABILITY:
      return "PORTABILITY";
   case GL_DEBUG_TYPE_PERFORMANCE:
      return "PERFORMANCE";
   case GL_DEBUG_TYPE_MARKER:
      return "MARKER";
   case GL_DEBUG_TYPE_PUSH_GROUP:
      return "PUSH_GROUP";
   case GL_DEBUG_TYPE_POP_GROUP:
      return "POP_GROUP";
   case GL_DEBUG_TYPE_OTHER:
      return "OTHER";
   default:
#ifdef DECAF_GLBINDING
      return glbinding::Meta::getString(severity);
#else
      return "???";
#endif
   }
}

static std::string
getGlDebugSeverity(GLenum severity)
{
   switch (severity) {
   case GL_DEBUG_SEVERITY_HIGH:
      return "HIGH";
   case GL_DEBUG_SEVERITY_MEDIUM:
      return "MED";
   case GL_DEBUG_SEVERITY_LOW:
      return "LOW";
   case GL_DEBUG_SEVERITY_NOTIFICATION:
      return "NOTIF";
   default:
#ifdef DECAF_GLBINDING
      return glbinding::Meta::getString(severity);
#else
      return "???";
#endif
   }
}

static void APIENTRY
debugMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
   GLsizei length, const GLchar* message, const void *userParam)
{
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

   if (severity == GL_DEBUG_SEVERITY_HIGH) {
      gCliLog->warn(outputStr);
   } else if (severity == GL_DEBUG_SEVERITY_MEDIUM) {
      gCliLog->debug(outputStr);
   } else if (severity == GL_DEBUG_SEVERITY_LOW) {
      gCliLog->trace(outputStr);
   } else if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
      gCliLog->info(outputStr);
   } else {
      gCliLog->info(outputStr);
   }
}

DecafSDLOpenGL::DecafSDLOpenGL()
{
    using decaf::config::ui::background_colour;
    
    mBackgroundColour[0] = background_colour.r / 255.0;
    mBackgroundColour[1] = background_colour.g / 255.0;
    mBackgroundColour[2] = background_colour.b / 255.0;
}

DecafSDLOpenGL::~DecafSDLOpenGL()
{
   if (mContext) {
      SDL_GL_DeleteContext(mContext);
      mContext = nullptr;
   }

   if (mThreadContext) {
      SDL_GL_DeleteContext(mThreadContext);
      mThreadContext = nullptr;
   }
}

void
DecafSDLOpenGL::initialiseContext()
{
#ifdef DECAF_GLBINDING
   glbinding::Binding::initialize();
   glbinding::setCallbackMaskExcept(glbinding::CallbackMask::After | glbinding::CallbackMask::ParametersAndReturnValue, { "glGetError" });
   glbinding::setAfterCallback([](const glbinding::FunctionCall &call) {
      auto error = glbinding::Binding::GetError.directCall();

      if (error != GL_NO_ERROR) {
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
#endif

   if (decaf::config::gpu::debug) {
      glDebugMessageCallback(&debugMessageCallback, nullptr);
      glEnable(GL_DEBUG_OUTPUT);
      glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
   }
}

void
DecafSDLOpenGL::initialiseDraw()
{
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
   mVertexProgram = glCreateShaderProgramv(GL_VERTEX_SHADER, 1, &vertexCode);

   // Create pixel program
   mPixelProgram = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &pixelCode);
   glBindFragDataLocation(mPixelProgram, 0, "ps_color");

   // Create pipeline
   glGenProgramPipelines(1, &mPipeline);
   glUseProgramStages(mPipeline, GL_VERTEX_SHADER_BIT, mVertexProgram);
   glUseProgramStages(mPipeline, GL_FRAGMENT_SHADER_BIT, mPixelProgram);

   // (TL, TR, BR)    (BR, BL, TL)
   // Create vertex buffer
   static const GLfloat vertices[] = {
      -1.0f,  -1.0f,   0.0f, 1.0f,
      1.0f,  -1.0f,   1.0f, 1.0f,
      1.0f, 1.0f,   1.0f, 0.0f,

      1.0f, 1.0f,   1.0f, 0.0f,
      -1.0f, 1.0f,   0.0f, 0.0f,
      -1.0f,  -1.0f,   0.0f, 1.0f,
   };

   glCreateBuffers(1, &mVertBuffer);
   glNamedBufferData(mVertBuffer, sizeof(vertices), vertices, GL_STATIC_DRAW);

   // Create vertex array
   glCreateVertexArrays(1, &mVertArray);

   auto fs_position = glGetAttribLocation(mVertexProgram, "fs_position");
   glEnableVertexArrayAttrib(mVertArray, fs_position);
   glVertexArrayAttribFormat(mVertArray, fs_position, 2, GL_FLOAT, GL_FALSE, 0);
   glVertexArrayAttribBinding(mVertArray, fs_position, 0);

   auto fs_texCoord = glGetAttribLocation(mVertexProgram, "fs_texCoord");
   glEnableVertexArrayAttrib(mVertArray, fs_texCoord);
   glVertexArrayAttribFormat(mVertArray, fs_texCoord, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat));
   glVertexArrayAttribBinding(mVertArray, fs_texCoord, 0);

   // Create texture sampler
   glGenSamplers(1, &mSampler);

   glSamplerParameteri(mSampler, GL_TEXTURE_WRAP_S, static_cast<int>(GL_CLAMP));
   glSamplerParameteri(mSampler, GL_TEXTURE_WRAP_S, static_cast<int>(GL_CLAMP));
   glSamplerParameteri(mSampler, GL_TEXTURE_MIN_FILTER, static_cast<int>(GL_LINEAR));
   glSamplerParameteri(mSampler, GL_TEXTURE_MAG_FILTER, static_cast<int>(GL_LINEAR));
}

void
DecafSDLOpenGL::drawScanBuffer(GLuint object)
{
   // Setup screen draw shader
   glBindVertexArray(mVertArray);
   glBindVertexBuffer(0, mVertBuffer, 0, 4 * sizeof(GLfloat));
   glBindProgramPipeline(mPipeline);

   // Draw screen quad
   glBindSampler(0, mSampler);
   glBindTextureUnit(0, object);

   glDrawArrays(GL_TRIANGLES, 0, 6);
}

void
DecafSDLOpenGL::drawScanBuffers(Viewport &tvViewport,
                                GLuint tvBuffer,
                                Viewport &drcViewport,
                                GLuint drcBuffer)
{
   // Set up some needed GL state
   glColorMaski(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
   glDisablei(GL_BLEND, 0);
   glDisable(GL_DEPTH_TEST);
   glDisable(GL_STENCIL_TEST);
   glDisable(GL_SCISSOR_TEST);
   glDisable(GL_CULL_FACE);

   // Clear screen
   glClearColor(mBackgroundColour[0], mBackgroundColour[1], mBackgroundColour[2], 1.0f);
   glClear(GL_COLOR_BUFFER_BIT);

   // Draw displays
   auto drawTV = tvViewport.width > 0 && tvViewport.height > 0;
   auto drawDRC = drcViewport.width > 0 && drcViewport.height > 0;

   if (drawTV) {
      float viewportArray[] = {
         tvViewport.x, tvViewport.y,
         tvViewport.width, tvViewport.height
      };

      glViewportArrayv(0, 1, viewportArray);
      drawScanBuffer(tvBuffer);
   }

   if (drawDRC) {
      float viewportArray[] = {
         drcViewport.x, drcViewport.y,
         drcViewport.width, drcViewport.height
      };

      glViewportArrayv(0, 1, viewportArray);
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
DecafSDLOpenGL::initialise(int width, int height)
{
   if (SDL_GL_LoadLibrary(NULL) != 0) {
      gCliLog->error("Failed to load OpenGL library: {}", SDL_GetError());
      return false;
   }

#ifndef DECAF_GLBINDING
   platform::setGLLookupFunction(SDL_GL_GetProcAddress);
#endif

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

   SDL_GL_MakeCurrent(mWindow, mContext);

   // Setup decaf driver
   auto glDriver = decaf::createGLDriver();
   decaf_check(glDriver);
   mDecafDriver = reinterpret_cast<decaf::OpenGLDriver*>(glDriver);

   // Setup rendering
   initialiseContext();
   initialiseDraw();
   decaf::debugger::initialiseUiGL();

   // Start graphics thread
   if (!config::gpu::force_sync) {
      SDL_GL_SetSwapInterval(1);

      mGraphicsThread = std::thread{
         [this]() {
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
   }

   return true;
}

void
DecafSDLOpenGL::shutdown()
{
   // Shut down the GPU
   if (!config::gpu::force_sync) {
      mDecafDriver->stop();
      mGraphicsThread.join();
   }
}

void
DecafSDLOpenGL::renderFrame(Viewport &tv, Viewport &drc)
{
   if (!config::gpu::force_sync) {
      GLuint tvBuffer = 0;
      GLuint drcBuffer = 0;
      mDecafDriver->getSwapBuffers(&tvBuffer, &drcBuffer);
      drawScanBuffers(tv, tvBuffer, drc, drcBuffer);
   } else {
      mDecafDriver->syncPoll([&](unsigned int tvBuffer, unsigned int drcBuffer) {
         SDL_GL_MakeCurrent(mWindow, mContext);
         drawScanBuffers(tv, tvBuffer, drc, drcBuffer);
         SDL_GL_MakeCurrent(mWindow, mThreadContext);
      });
   }
}

decaf::GraphicsDriver *
DecafSDLOpenGL::getDecafDriver()
{
   return mDecafDriver;
}

#endif // DECAF_NOGL
