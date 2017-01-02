#include "sdl_window.h"
#include <common/gl.h>
#include <common/log.h>
#include <common/decaf_assert.h>

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

void
SDLWindow::initialiseContext()
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

         gLog->error("OpenGL error: {} with {}", glbinding::Meta::getString(error), writer.str());
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
SDLWindow::initialiseDraw()
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
SDLWindow::drawScanBuffer(GLuint object)
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
SDLWindow::calculateScreenViewports(float(&tv)[4],
                                    float(&drc)[4])
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

   SDL_GetWindowSize(mWindow, &windowWidth, &windowHeight);

   nativeHeight = DrcHeight + TvHeight + ScreenSeparation + 2 * OuterBorder;
   nativeWidth = std::max(DrcWidth, TvWidth) + 2 * OuterBorder;

   if (windowWidth * nativeHeight >= windowHeight * nativeWidth) {
      // Align to height
      int drcBorder = (windowWidth * nativeHeight - windowHeight * DrcWidth + nativeHeight) / nativeHeight / 2;
      int tvBorder = (windowWidth * nativeHeight - windowHeight * TvWidth + nativeHeight) / nativeHeight / 2;

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
SDLWindow::drawScanBuffers(GLuint tvBuffer, GLuint drcBuffer)
{
   float tvViewport[4], drcViewport[4];
   calculateScreenViewports(tvViewport, drcViewport);

   // Set up some needed GL state
   glColorMaski(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
   glDisablei(GL_BLEND, 0);
   glDisable(GL_DEPTH_TEST);
   glDisable(GL_STENCIL_TEST);
   glDisable(GL_SCISSOR_TEST);
   glDisable(GL_CULL_FACE);

   // Clear screen
   glClearColor(0.6f, 0.2f, 0.2f, 1.0f);
   glClear(GL_COLOR_BUFFER_BIT);

   // Draw displays
   glViewportArrayv(0, 1, tvViewport);
   drawScanBuffer(tvBuffer);

   glViewportArrayv(0, 1, drcViewport);
   drawScanBuffer(drcBuffer);

   // Swap
   SDL_GL_SwapWindow(mWindow);
}
