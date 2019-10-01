#pragma optimize("", off)
#if DECAF_GL
#include "opengl_driver.h"

#include "gpu_config.h"
#include "gpu7_displaylayout.h"

#include <common/log.h>
#include <common/platform.h>
#include <fmt/format.h>
#include <memory>
#include <glad/glad.h>

namespace opengl
{

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
      return fmt::format("GL_DEBUG_SOURCE_UNKNOWN_{}", source);
   }
}

static std::string
getGlDebugType(GLenum type)
{
   switch (type) {
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
      return fmt::format("GL_DEBUG_TYPE_UNKNOWN_{}", type);
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
      return fmt::format("GL_DEBUG_SEVERITY_UNKNOWN_{}", severity);
   }
}

static void
debugMessageCallback(GLenum source,
                     GLenum type,
                     GLuint id,
                     GLenum severity,
                     GLsizei length,
                     const GLchar* message,
                     const void *userParam)
{
   for (auto filterID : gpu::config()->opengl.debug_message_filters) {
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

   if (severity == GL_DEBUG_SEVERITY_HIGH) {
      gLog->warn(outputStr);
   } else if (severity == GL_DEBUG_SEVERITY_MEDIUM) {
      gLog->debug(outputStr);
   } else if (severity == GL_DEBUG_SEVERITY_LOW) {
      gLog->trace(outputStr);
   } else if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
      gLog->info(outputStr);
   } else {
      gLog->info(outputStr);
   }
}

#ifdef GLAD_DEBUG
static void
gladPostCallback(const char* name, void* funcptr, int len_args, ...)
{
   if (glad_glGetError) {
      auto error = glad_glGetError();
      if (error != GL_NO_ERROR) {
         gLog->error("OpenGL error: {} failed with error {}", name, error);
      }
   }
}
#endif

static void
createDisplayPipeline(DisplayPipeline &displayPipeline)
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
   displayPipeline.vertexProgram = glCreateShaderProgramv(GL_VERTEX_SHADER, 1, &vertexCode);

   // Create pixel program
   displayPipeline.fragmentProgram = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &pixelCode);
   glBindFragDataLocation(displayPipeline.fragmentProgram, 0, "ps_color");

   // Create pipeline
   glGenProgramPipelines(1, &displayPipeline.programPipeline);
   glUseProgramStages(displayPipeline.programPipeline, GL_VERTEX_SHADER_BIT, displayPipeline.vertexProgram);
   glUseProgramStages(displayPipeline.programPipeline, GL_FRAGMENT_SHADER_BIT, displayPipeline.fragmentProgram);

   // (TL, TR, BR)    (BR, BL, TL)
   // Create vertex buffer
   static constexpr GLfloat vertices[] = {
      -1.0f,  -1.0f,   0.0f, 1.0f,
      1.0f,  -1.0f,   1.0f, 1.0f,
      1.0f, 1.0f,   1.0f, 0.0f,

      1.0f, 1.0f,   1.0f, 0.0f,
      -1.0f, 1.0f,   0.0f, 0.0f,
      -1.0f,  -1.0f,   0.0f, 1.0f,
   };

   glCreateBuffers(1, &displayPipeline.vertexBuffer);
   glNamedBufferData(displayPipeline.vertexBuffer, sizeof(vertices), vertices, GL_STATIC_DRAW);

   // Create vertex array
   glCreateVertexArrays(1, &displayPipeline.vertexArray);

   auto fs_position = glGetAttribLocation(displayPipeline.vertexProgram, "fs_position");
   glEnableVertexArrayAttrib(displayPipeline.vertexArray, fs_position);
   glVertexArrayAttribFormat(displayPipeline.vertexArray, fs_position, 2, GL_FLOAT, GL_FALSE, 0);
   glVertexArrayAttribBinding(displayPipeline.vertexArray, fs_position, 0);

   auto fs_texCoord = glGetAttribLocation(displayPipeline.vertexProgram, "fs_texCoord");
   glEnableVertexArrayAttrib(displayPipeline.vertexArray, fs_texCoord);
   glVertexArrayAttribFormat(displayPipeline.vertexArray, fs_texCoord, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat));
   glVertexArrayAttribBinding(displayPipeline.vertexArray, fs_texCoord, 0);

   // Create texture sampler
   glGenSamplers(1, &displayPipeline.sampler);

   glSamplerParameteri(displayPipeline.sampler, GL_TEXTURE_WRAP_S, static_cast<int>(GL_CLAMP_TO_EDGE));
   glSamplerParameteri(displayPipeline.sampler, GL_TEXTURE_WRAP_T, static_cast<int>(GL_CLAMP_TO_EDGE));
   glSamplerParameteri(displayPipeline.sampler, GL_TEXTURE_MIN_FILTER, static_cast<int>(GL_LINEAR));
   glSamplerParameteri(displayPipeline.sampler, GL_TEXTURE_MAG_FILTER, static_cast<int>(GL_LINEAR));
}

void
GLDriver::setWindowSystemInfo(const gpu::WindowSystemInfo &wsi)
{
#ifdef GLAD_DEBUG
   // Setup glad post callback
   glad_set_post_callback(&gladPostCallback);
#endif

   // Setup display context
   mDisplayContext = createContext(wsi);
   mDisplayContext->setSwapInterval(0);

   // Initialise glad
   gladLoadGL();
   if (gpu::config()->debug.debug_enabled) {
      glDebugMessageCallback(&debugMessageCallback, nullptr);
      glEnable(GL_DEBUG_OUTPUT);
      glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
   }

   // Create display pipeline
   std::tie(mDisplayPipeline.width, mDisplayPipeline.height) = mDisplayContext->getDimensions();
   createDisplayPipeline(mDisplayPipeline);

   // Setup render context
   mContext = mDisplayContext->createSharedContext();
   mContext->setSwapInterval(0);

   // Initialise driver
   initialise();

   // Clear context
   mContext->clearCurrent();
}

void
GLDriver::windowHandleChanged(void *handle)
{
}

void
GLDriver::windowSizeChanged(int width, int height)
{
   mDisplayPipeline.width = width;
   mDisplayPipeline.height = height;
}

void
GLDriver::renderDisplay()
{
   mDisplayContext->makeCurrent();

   // Set up some needed GL state
   glColorMaski(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
   glDisablei(GL_BLEND, 0);
   glDisable(GL_DEPTH_TEST);
   glDisable(GL_STENCIL_TEST);
   glDisable(GL_SCISSOR_TEST);
   glDisable(GL_CULL_FACE);

   // Clear screen
   auto config = gpu::config();
   glClearColor(config->display.backgroundColour[0] / 255.0f,
                    config->display.backgroundColour[1] / 255.0f,
                    config->display.backgroundColour[2] / 255.0f,
                    1.0f);
   glClear(GL_COLOR_BUFFER_BIT);

   // Setup display layout
   const auto displayWidth = mDisplayPipeline.width.load();
   const auto displayHeight = mDisplayPipeline.height.load();
   auto layout = gpu7::DisplayLayout { };
   gpu7::updateDisplayLayout(
      layout,
      static_cast<float>(displayWidth),
      static_cast<float>(displayHeight));
   layout.tv.y = displayHeight - (layout.tv.y + layout.tv.height);
   layout.drc.y = displayHeight - (layout.drc.y + layout.drc.height);

   // Draw displays
   auto drawScanBuffer =
      [this](GLuint object)
      {
         // Setup screen draw shader
         glBindVertexArray(mDisplayPipeline.vertexArray);
         glBindVertexBuffer(0, mDisplayPipeline.vertexBuffer, 0, 4 * sizeof(GLfloat));
         glBindProgramPipeline(mDisplayPipeline.programPipeline);

         // Draw screen quad
         glBindSampler(0, mDisplayPipeline.sampler);
         glBindTextureUnit(0, object);

         glDrawArrays(GL_TRIANGLES, 0, 6);
      };

   if (layout.tv.visible) {
      float viewportArray[] = {
         layout.tv.x, layout.tv.y,
         layout.tv.width, layout.tv.height
      };

      glViewportArrayv(0, 1, viewportArray);
      drawScanBuffer(mTvScanBuffers.object);
   }

   if (layout.drc.visible) {
      float viewportArray[] = {
         layout.drc.x, layout.drc.y,
         layout.drc.width, layout.drc.height
      };

      glViewportArrayv(0, 1, viewportArray);
      drawScanBuffer(mDrcScanBuffers.object);
   }

   mDisplayContext->swapBuffers();
   mContext->makeCurrent();
}

} // namespace opengl

#endif // if DECAF_GL
