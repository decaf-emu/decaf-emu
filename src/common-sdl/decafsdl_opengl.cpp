#ifdef DECAF_GL
#include "decafsdl_config.h"
#include "decafsdl_opengl.h"

#include <common/decaf_assert.h>
#include <common/platform_dir.h>
#include <fmt/format.h>
#include <glbinding/Binding.h>
#include <glbinding/Meta.h>
#include <libdecaf/decaf_log.h>
#include <libgpu/gpu_config.h>
#include <libgpu/gpu_opengldriver.h>
#include <string>

static std::string
getGlDebugSource(gl::GLenum source)
{
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
getGlDebugType(gl::GLenum severity)
{
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
getGlDebugSeverity(gl::GLenum severity)
{
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

DecafSDLOpenGL::DecafSDLOpenGL()
{
   // Initialise logger
   mLog = decaf::makeLogger("sdl-gl");

   // Setup background colour
   using config::display::background_colour;
   mBackgroundColour[0] = background_colour.r / 255.0f;
   mBackgroundColour[1] = background_colour.g / 255.0f;
   mBackgroundColour[2] = background_colour.b / 255.0f;
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

void GL_APIENTRY
DecafSDLOpenGL::debugMessageCallback(gl::GLenum source,
                                     gl::GLenum type,
                                     gl::GLuint id,
                                     gl::GLenum severity,
                                     gl::GLsizei length,
                                     const gl::GLchar* message,
                                     const void *userParam)
{
   auto self = reinterpret_cast<DecafSDLOpenGL *>(const_cast<void *>(userParam));
   for (auto filterID : gpu::config::debug_filters) {
      if (filterID == id) {
         return;
      }
   }

   auto outputStr =
      fmt::format(
         "GL Message ({}, {}, {}, {}) {}",
         id,
         getGlDebugSource(source),
         getGlDebugType(type),
         getGlDebugSeverity(severity),
         message);

   if (severity == gl::GL_DEBUG_SEVERITY_HIGH) {
      self->mLog->warn(outputStr);
   } else if (severity == gl::GL_DEBUG_SEVERITY_MEDIUM) {
      self->mLog->debug(outputStr);
   } else if (severity == gl::GL_DEBUG_SEVERITY_LOW) {
      self->mLog->trace(outputStr);
   } else if (severity == gl::GL_DEBUG_SEVERITY_NOTIFICATION) {
      self->mLog->info(outputStr);
   } else {
      self->mLog->info(outputStr);
   }
}

void
DecafSDLOpenGL::initialiseContext()
{
   glbinding::Binding::initialize();

   if (gpu::config::debug) {
      glbinding::setCallbackMaskExcept(glbinding::CallbackMask::After | glbinding::CallbackMask::ParametersAndReturnValue, { "glGetError" });
      glbinding::setAfterCallback([&](const glbinding::FunctionCall &call) {
         auto error = glbinding::Binding::GetError.directCall();

         if (error != gl::GL_NO_ERROR) {
            fmt::memory_buffer out;
            fmt::format_to(out, "{}(", call.function->name());

            for (unsigned i = 0; i < call.parameters.size(); ++i) {
               fmt::format_to(out, "{}", call.parameters[i]->asString());
               if (i < call.parameters.size() - 1) {
                  fmt::format_to(out, ", ");
               }
            }

            fmt::format_to(out, ")");

            if (call.returnValue) {
               fmt::format_to(out, " -> {}", call.returnValue->asString());
            }

            mLog->error("OpenGL error: {} with {}", glbinding::Meta::getString(error), out.data());
         }
      });

      gl::glDebugMessageCallback(&debugMessageCallback, this);
      gl::glEnable(gl::GL_DEBUG_OUTPUT);
      gl::glEnable(gl::GL_DEBUG_OUTPUT_SYNCHRONOUS);
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

   gl::glSamplerParameteri(mSampler, gl::GL_TEXTURE_WRAP_S, static_cast<int>(gl::GL_CLAMP_TO_EDGE));
   gl::glSamplerParameteri(mSampler, gl::GL_TEXTURE_WRAP_T, static_cast<int>(gl::GL_CLAMP_TO_EDGE));
   gl::glSamplerParameteri(mSampler, gl::GL_TEXTURE_MIN_FILTER, static_cast<int>(gl::GL_LINEAR));
   gl::glSamplerParameteri(mSampler, gl::GL_TEXTURE_MAG_FILTER, static_cast<int>(gl::GL_LINEAR));
}

void
DecafSDLOpenGL::drawScanBuffer(gl::GLuint object)
{
   // Setup screen draw shader
   gl::glBindVertexArray(mVertArray);
   gl::glBindVertexBuffer(0, mVertBuffer, 0, 4 * sizeof(gl::GLfloat));
   gl::glBindProgramPipeline(mPipeline);

   // Draw screen quad
   gl::glBindSampler(0, mSampler);
   gl::glBindTextureUnit(0, object);

   gl::glDrawArrays(gl::GL_TRIANGLES, 0, 6);
}

void
DecafSDLOpenGL::drawScanBuffers(Viewport &tvViewport,
                                gl::GLuint tvBuffer,
                                Viewport &drcViewport,
                                gl::GLuint drcBuffer)
{
   // Set up some needed GL state
   gl::glColorMaski(0, gl::GL_TRUE, gl::GL_TRUE, gl::GL_TRUE, gl::GL_TRUE);
   gl::glDisablei(gl::GL_BLEND, 0);
   gl::glDisable(gl::GL_DEPTH_TEST);
   gl::glDisable(gl::GL_STENCIL_TEST);
   gl::glDisable(gl::GL_SCISSOR_TEST);
   gl::glDisable(gl::GL_CULL_FACE);

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
   if (mDebugUiRenderer) {
      int width, height;
      SDL_GetWindowSize(mWindow, &width, &height);
      mDebugUiRenderer->draw(width, height);
   }

   // Swap
   SDL_GL_SwapWindow(mWindow);
   mFramesPresented++;

   if (config::test::timeout_frames != -1 && mFramesPresented >= config::test::timeout_frames) {
      decaf::shutdown();
   }
}

bool
DecafSDLOpenGL::initialise(int width,
                           int height,
                           bool renderDebugger)
{
   if (SDL_GL_LoadLibrary(NULL) != 0) {
      mLog->error("Failed to load OpenGL library: {}", SDL_GetError());
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
   if (gpu::config::debug) {
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
   }

   // Create TV window
   mWindow = SDL_CreateWindow("Decaf",
      SDL_WINDOWPOS_UNDEFINED,
      SDL_WINDOWPOS_UNDEFINED,
      width, height,
      SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);

   if (!mWindow) {
      mLog->error("Failed to create GL SDL window");
      return false;
   }

   SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);

   // Create OpenGL context
   mContext = SDL_GL_CreateContext(mWindow);
   if (!mContext) {
      mLog->error("Failed to create Main OpenGL context: {}", SDL_GetError());
      return false;
   }

   mThreadContext = SDL_GL_CreateContext(mWindow);
   if (!mThreadContext) {
      mLog->error("Failed to create GPU OpenGL context: {}", SDL_GetError());
      return false;
   }

   SDL_GL_MakeCurrent(mWindow, mContext);

   // Setup rendering
   initialiseContext();
   initialiseDraw();

   // Setup decaf driver
   mDecafDriver = reinterpret_cast<gpu::OpenGLDriver*>(gpu::createGLDriver());

   if (config::test::dump_drc_frames || config::test::dump_tv_frames) {
      platform::createDirectory(config::test::dump_frames_dir);
      mDecafDriver->startFrameCapture(config::test::dump_frames_dir + "/dump",
                                      config::test::dump_tv_frames,
                                      config::test::dump_drc_frames);
   }

   // Initialise debugger renderer if needed
   if (renderDebugger) {
      mDebugUiRenderer = reinterpret_cast<debugui::OpenGLRenderer *>(
         debugui::createOpenGLRenderer(decaf::makeConfigPath("imgui.ini")));
   }

   // Start graphics thread
   if (!config::display::force_sync) {
      SDL_GL_SetSwapInterval(1);

      mGraphicsThread = std::thread {
         [this]() {
            SDL_GL_MakeCurrent(mWindow, mThreadContext);
            initialiseContext();
            mDecafDriver->initialise();
            mDecafDriver->run();
         } };
   } else {
      // Set the swap interval to 0 so that we don't slow
      //  down the GPU system when presenting...  The game should
      //  throttle our swapping automatically anyways.
      SDL_GL_SetSwapInterval(0);

      // Switch to the thread context for initialisation
      SDL_GL_MakeCurrent(mWindow, mThreadContext);

      // Initialise the context
      initialiseContext();

      // Setup the driver
      mDecafDriver->initialise();

      // Switch back to the window context
      SDL_GL_MakeCurrent(mWindow, mContext);
   }

   return true;
}

void
DecafSDLOpenGL::shutdown()
{
   // Shut down the debugger ui driver
   if (mDebugUiRenderer) {
      delete mDebugUiRenderer;
      mDebugUiRenderer = nullptr;
   }

   // Stop the GPU
   if (!config::display::force_sync) {
      mDecafDriver->stop();
      mGraphicsThread.join();
   }

   // Shut down the GPU
   mDecafDriver->shutdown();
}

void
DecafSDLOpenGL::windowResized()
{
}

void
DecafSDLOpenGL::renderFrame(Viewport &tv, Viewport &drc)
{
   if (config::display::force_sync) {
      SDL_GL_MakeCurrent(mWindow, mThreadContext);
      mDecafDriver->runUntilFlip();
      SDL_GL_MakeCurrent(mWindow, mContext);
   }

   gl::GLuint tvBuffer = 0;
   gl::GLuint drcBuffer = 0;
   mDecafDriver->getSwapBuffers(&tvBuffer, &drcBuffer);
   drawScanBuffers(tv, tvBuffer, drc, drcBuffer);
}

gpu::GraphicsDriver *
DecafSDLOpenGL::getDecafDriver()
{
   return mDecafDriver;
}

debugui::Renderer *
DecafSDLOpenGL::getDebugUiRenderer()
{
   return mDebugUiRenderer;
}

void
DecafSDLOpenGL::getWindowSize(int *w, int *h)
{
   SDL_GL_GetDrawableSize(mWindow, w, h);
}

#endif // ifdef DECAF_GL
