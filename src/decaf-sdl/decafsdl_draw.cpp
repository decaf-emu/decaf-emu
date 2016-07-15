#include "clilog.h"
#include "common/decaf_assert.h"
#include "decafsdl.h"
#include <glbinding/Binding.h>
#include <glbinding/Meta.h>

void DecafSDL::initialiseContext()
{
   glbinding::Binding::initialize();
   glbinding::setCallbackMaskExcept(glbinding::CallbackMask::After | glbinding::CallbackMask::ParametersAndReturnValue, { "glGetError" });
   glbinding::setAfterCallback([](const glbinding::FunctionCall &call) {
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
}

void DecafSDL::initialiseDraw()
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

   gl::glSamplerParameteri(mSampler, gl::GL_TEXTURE_WRAP_S, static_cast<int>(gl::GL_CLAMP));
   gl::glSamplerParameteri(mSampler, gl::GL_TEXTURE_WRAP_S, static_cast<int>(gl::GL_CLAMP));
   gl::glSamplerParameteri(mSampler, gl::GL_TEXTURE_MIN_FILTER, static_cast<int>(gl::GL_LINEAR));
   gl::glSamplerParameteri(mSampler, gl::GL_TEXTURE_MAG_FILTER, static_cast<int>(gl::GL_LINEAR));
}

void DecafSDL::drawScanBuffer(gl::GLuint object)
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

void DecafSDL::calculateScreenViewports(float (&tv)[4], float (&drc)[4])
{
   static const auto DrcWidth = 854.0f;
   static const auto DrcHeight = 480.0f;
   static const auto TvWidth = 1280.0f;
   static const auto TvHeight = 720.0f;

   static const auto DrcRatio = DrcHeight / (TvHeight + DrcHeight);
   static const auto OverallScale = 1.0f;
   static const auto ScreenSeparation = 5.0f;

   int windowWidth, windowHeight;
   SDL_GetWindowSize(mWindow, &windowWidth, &windowHeight);

   auto tvHeight = (windowHeight - ScreenSeparation) * (1.0f - DrcRatio) * OverallScale;
   auto tvWidth = TvWidth * (tvHeight / TvHeight);
   auto drcHeight = (windowHeight - ScreenSeparation) * DrcRatio * OverallScale;
   auto drcWidth = DrcWidth * (drcHeight / DrcHeight);
   auto totalWidth = std::max(tvWidth, drcWidth);
   auto totalHeight = tvHeight + drcHeight + ScreenSeparation;
   auto baseLeft = floor((windowWidth / 2) - (totalWidth / 2));
   auto baseBottom = floor(-(windowHeight / 2) + (totalHeight / 2));

   auto tvLeft = 0.0f;
   auto tvBottom = windowHeight - tvHeight;
   auto drcLeft = (tvWidth / 2) - (drcWidth / 2);
   auto drcBottom = windowHeight - tvHeight - drcHeight - ScreenSeparation;

   tv[0] = baseLeft + tvLeft;
   tv[1] = baseBottom + tvBottom;
   tv[2] = tvWidth;
   tv[3] = tvHeight;

   drc[0] = baseLeft + drcLeft;
   drc[1] = baseBottom + drcBottom;
   drc[2] = drcWidth;
   drc[3] = drcHeight;

   if (OverallScale <= 1.0f) {
      decaf_check(roundf(tv[0]) >= 0);
      decaf_check(roundf(tv[1]) >= 0);
      decaf_check(roundf(tv[0] + tv[2]) <= windowWidth);
      decaf_check(roundf(tv[1] + tv[3]) <= windowHeight);
      decaf_check(roundf(drc[0]) >= 0);
      decaf_check(roundf(drc[1]) >= 0);
      decaf_check(roundf(drc[0] + drc[2]) <= windowWidth);
      decaf_check(roundf(drc[1] + drc[3]) <= windowHeight);
   }
}

void DecafSDL::drawScanBuffers(gl::GLuint tvBuffer, gl::GLuint drcBuffer)
{
   float tvViewport[4], drcViewport[4];
   calculateScreenViewports(tvViewport, drcViewport);

   // Set up some needed GL state
   gl::glColorMaski(0, gl::GL_TRUE, gl::GL_TRUE, gl::GL_TRUE, gl::GL_TRUE);
   gl::glDisablei(gl::GL_BLEND, 0);
   gl::glDisable(gl::GL_DEPTH_TEST);
   gl::glDisable(gl::GL_STENCIL_TEST);
   gl::glDisable(gl::GL_SCISSOR_TEST);
   gl::glDisable(gl::GL_CULL_FACE);

   // Clear screen
   gl::glClearColor(0.6f, 0.2f, 0.2f, 1.0f);
   gl::glClear(gl::GL_COLOR_BUFFER_BIT);

   // Draw TV display
   if (tvBuffer != 0) {
      gl::glViewportArrayv(0, 1, tvViewport);
      drawScanBuffer(tvBuffer);
   }

   // Draw DRC display
   if (drcBuffer != 0) {
      gl::glViewportArrayv(0, 1, drcViewport);
      drawScanBuffer(drcBuffer);
   }

   // Draw UI
   int width, height;
   SDL_GetWindowSize(mWindow, &width, &height);
   decaf::debugger::drawUiGL(width, height);

   // Swap
   SDL_GL_SwapWindow(mWindow);
}
