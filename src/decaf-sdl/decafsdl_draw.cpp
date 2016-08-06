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
   static const int DrcWidth = 854;
   static const int DrcHeight = 480;
   static const int TvWidth = 1280;
   static const int TvHeight = 720;

   static const int ScreenSeparation = 5;
   static const int OuterBorder = 0;

   int windowWidth, windowHeight;
   SDL_GetWindowSize(mWindow, &windowWidth, &windowHeight);

   int nativeHeight = DrcHeight + TvHeight + ScreenSeparation + 2 * OuterBorder;
   int nativeWidth = std::max(DrcWidth, TvWidth) + 2 * OuterBorder;

   int tvLeft, tvBottom, tvTop, tvRight;
   int drcLeft, drcBottom, drcTop, drcRight;

   if (windowWidth * nativeHeight >= windowHeight * nativeWidth)
   {
      // align to height
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
   }
   else
   {
      // align to width
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

   tv[0] = tvLeft;
   tv[1] = tvBottom;
   tv[2] = tvRight - tvLeft;
   tv[3] = tvTop - tvBottom;

   drc[0] = drcLeft;
   drc[1] = drcBottom;
   drc[2] = drcRight - drcLeft;
   drc[3] = drcTop - drcBottom;

   decaf_check(tv[0] >= 0);
   decaf_check(tv[1] >= 0);
   decaf_check(tv[0] + tv[2] <= windowWidth);
   decaf_check(tv[1] + tv[3] <= windowHeight);
   decaf_check(drc[0] >= 0);
   decaf_check(drc[1] >= 0);
   decaf_check(drc[0] + drc[2] <= windowWidth);
   decaf_check(drc[1] + drc[3] <= windowHeight);
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
