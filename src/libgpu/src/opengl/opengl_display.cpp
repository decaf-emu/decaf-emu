#pragma optimize("", off)
#if DECAF_GL
#include "opengl_driver.h"

#include "gpu_config.h"
#include "gpu7_displaylayout.h"

#include <common/log.h>
#include <common/platform.h>
#include <memory>
#include <glbinding/Binding.h>
#include <glbinding/Meta.h>

namespace opengl
{

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

static void GL_APIENTRY
debugMessageCallback(gl::GLenum source,
                     gl::GLenum type,
                     gl::GLuint id,
                     gl::GLenum severity,
                     gl::GLsizei length,
                     const gl::GLchar* message,
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

   if (severity == gl::GL_DEBUG_SEVERITY_HIGH) {
      gLog->warn(outputStr);
   } else if (severity == gl::GL_DEBUG_SEVERITY_MEDIUM) {
      gLog->debug(outputStr);
   } else if (severity == gl::GL_DEBUG_SEVERITY_LOW) {
      gLog->trace(outputStr);
   } else if (severity == gl::GL_DEBUG_SEVERITY_NOTIFICATION) {
      gLog->info(outputStr);
   } else {
      gLog->info(outputStr);
   }
}

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
   displayPipeline.vertexProgram = gl::glCreateShaderProgramv(gl::GL_VERTEX_SHADER, 1, &vertexCode);

   // Create pixel program
   displayPipeline.fragmentProgram = gl::glCreateShaderProgramv(gl::GL_FRAGMENT_SHADER, 1, &pixelCode);
   gl::glBindFragDataLocation(displayPipeline.fragmentProgram, 0, "ps_color");

   // Create pipeline
   gl::glGenProgramPipelines(1, &displayPipeline.programPipeline);
   gl::glUseProgramStages(displayPipeline.programPipeline, gl::GL_VERTEX_SHADER_BIT, displayPipeline.vertexProgram);
   gl::glUseProgramStages(displayPipeline.programPipeline, gl::GL_FRAGMENT_SHADER_BIT, displayPipeline.fragmentProgram);

   // (TL, TR, BR)    (BR, BL, TL)
   // Create vertex buffer
   static constexpr gl::GLfloat vertices[] = {
      -1.0f,  -1.0f,   0.0f, 1.0f,
      1.0f,  -1.0f,   1.0f, 1.0f,
      1.0f, 1.0f,   1.0f, 0.0f,

      1.0f, 1.0f,   1.0f, 0.0f,
      -1.0f, 1.0f,   0.0f, 0.0f,
      -1.0f,  -1.0f,   0.0f, 1.0f,
   };

   gl::glCreateBuffers(1, &displayPipeline.vertexBuffer);
   gl::glNamedBufferData(displayPipeline.vertexBuffer, sizeof(vertices), vertices, gl::GL_STATIC_DRAW);

   // Create vertex array
   gl::glCreateVertexArrays(1, &displayPipeline.vertexArray);

   auto fs_position = gl::glGetAttribLocation(displayPipeline.vertexProgram, "fs_position");
   gl::glEnableVertexArrayAttrib(displayPipeline.vertexArray, fs_position);
   gl::glVertexArrayAttribFormat(displayPipeline.vertexArray, fs_position, 2, gl::GL_FLOAT, gl::GL_FALSE, 0);
   gl::glVertexArrayAttribBinding(displayPipeline.vertexArray, fs_position, 0);

   auto fs_texCoord = gl::glGetAttribLocation(displayPipeline.vertexProgram, "fs_texCoord");
   gl::glEnableVertexArrayAttrib(displayPipeline.vertexArray, fs_texCoord);
   gl::glVertexArrayAttribFormat(displayPipeline.vertexArray, fs_texCoord, 2, gl::GL_FLOAT, gl::GL_FALSE, 2 * sizeof(gl::GLfloat));
   gl::glVertexArrayAttribBinding(displayPipeline.vertexArray, fs_texCoord, 0);

   // Create texture sampler
   gl::glGenSamplers(1, &displayPipeline.sampler);

   gl::glSamplerParameteri(displayPipeline.sampler, gl::GL_TEXTURE_WRAP_S, static_cast<int>(gl::GL_CLAMP_TO_EDGE));
   gl::glSamplerParameteri(displayPipeline.sampler, gl::GL_TEXTURE_WRAP_T, static_cast<int>(gl::GL_CLAMP_TO_EDGE));
   gl::glSamplerParameteri(displayPipeline.sampler, gl::GL_TEXTURE_MIN_FILTER, static_cast<int>(gl::GL_LINEAR));
   gl::glSamplerParameteri(displayPipeline.sampler, gl::GL_TEXTURE_MAG_FILTER, static_cast<int>(gl::GL_LINEAR));
}

static void
initialiseContext()
{
   glbinding::Binding::initialize();
   if (gpu::config()->debug.debug_enabled) {
      glbinding::setCallbackMaskExcept(glbinding::CallbackMask::After | glbinding::CallbackMask::ParametersAndReturnValue, { "glGetError" });
      glbinding::setAfterCallback([&](const glbinding::FunctionCall &call) {
         auto error = gl::glGetError();
         if (error != gl::GL_NO_ERROR) {
            auto out = fmt::memory_buffer { };
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

            gLog->error("OpenGL error: {} with {}", glbinding::Meta::getString(error), out.data());
         }
      });

      gl::glDebugMessageCallback(&debugMessageCallback, nullptr);
      gl::glEnable(gl::GL_DEBUG_OUTPUT);
      gl::glEnable(gl::GL_DEBUG_OUTPUT_SYNCHRONOUS);
   }
}

void
GLDriver::setWindowSystemInfo(const gpu::WindowSystemInfo &wsi)
{
   // Setup display context
   mDisplayContext = createContext(wsi);
   mDisplayContext->setSwapInterval(0);
   initialiseContext();
   std::tie(mDisplayPipeline.width, mDisplayPipeline.height) = mDisplayContext->getDimensions();
   createDisplayPipeline(mDisplayPipeline);

   // Setup render context
   mContext = mDisplayContext->createSharedContext();
   mContext->setSwapInterval(0);
   initialiseContext();

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
   gl::glColorMaski(0, gl::GL_TRUE, gl::GL_TRUE, gl::GL_TRUE, gl::GL_TRUE);
   gl::glDisablei(gl::GL_BLEND, 0);
   gl::glDisable(gl::GL_DEPTH_TEST);
   gl::glDisable(gl::GL_STENCIL_TEST);
   gl::glDisable(gl::GL_SCISSOR_TEST);
   gl::glDisable(gl::GL_CULL_FACE);

   // Clear screen
   auto config = gpu::config();
   gl::glClearColor(config->display.backgroundColour[0] / 255.0f,
                    config->display.backgroundColour[1] / 255.0f,
                    config->display.backgroundColour[2] / 255.0f,
                    1.0f);
   gl::glClear(gl::GL_COLOR_BUFFER_BIT);

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
      [this](gl::GLuint object)
      {
         // Setup screen draw shader
         gl::glBindVertexArray(mDisplayPipeline.vertexArray);
         gl::glBindVertexBuffer(0, mDisplayPipeline.vertexBuffer, 0, 4 * sizeof(gl::GLfloat));
         gl::glBindProgramPipeline(mDisplayPipeline.programPipeline);

         // Draw screen quad
         gl::glBindSampler(0, mDisplayPipeline.sampler);
         gl::glBindTextureUnit(0, object);

         gl::glDrawArrays(gl::GL_TRIANGLES, 0, 6);
      };

   if (layout.tv.visible) {
      float viewportArray[] = {
         layout.tv.x, layout.tv.y,
         layout.tv.width, layout.tv.height
      };

      gl::glViewportArrayv(0, 1, viewportArray);
      drawScanBuffer(mTvScanBuffers.object);
   }

   if (layout.drc.visible) {
      float viewportArray[] = {
         layout.drc.x, layout.drc.y,
         layout.drc.width, layout.drc.height
      };

      gl::glViewportArrayv(0, 1, viewportArray);
      drawScanBuffer(mDrcScanBuffers.object);
   }

   mDisplayContext->swapBuffers();
   mContext->makeCurrent();
}

} // namespace opengl

#endif // if DECAF_GL
