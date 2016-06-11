#include "gl_common.h"
#include <glbinding/gl/gl.h>
#include <glbinding/Binding.h>
#include <glbinding/Meta.h>
#include "clilog.h"



void setupGlErrorHandling()
{
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

         gLog->error("OpenGL error: {} with {}", glbinding::Meta::getString(error), writer.str());
      }
   });
}

struct ScreenDrawData
{
   gl::GLuint vertexProgram;
   gl::GLuint pixelProgram;
   gl::GLuint pipeline;
   gl::GLuint vertArray;
   gl::GLuint vertBuffer;
};

ScreenDrawData mScreenDraw;

void cglInitialise()
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
   mScreenDraw.vertexProgram = gl::glCreateShaderProgramv(gl::GL_VERTEX_SHADER, 1, &vertexCode);

   // Create pixel program
   mScreenDraw.pixelProgram = gl::glCreateShaderProgramv(gl::GL_FRAGMENT_SHADER, 1, &pixelCode);
   gl::glBindFragDataLocation(mScreenDraw.pixelProgram, 0, "ps_color");

   // Create pipeline
   gl::glGenProgramPipelines(1, &mScreenDraw.pipeline);
   gl::glUseProgramStages(mScreenDraw.pipeline, gl::GL_VERTEX_SHADER_BIT, mScreenDraw.vertexProgram);
   gl::glUseProgramStages(mScreenDraw.pipeline, gl::GL_FRAGMENT_SHADER_BIT, mScreenDraw.pixelProgram);

   // (TL, TR, BR)    (BR, BL, TL)
   // Create vertex buffer
   static const gl::GLfloat vertices[] = {
      -1.0f,  1.0f,   0.0f, 1.0f,
      1.0f,  1.0f,   1.0f, 1.0f,
      1.0f, -1.0f,   1.0f, 0.0f,

      1.0f, -1.0f,   1.0f, 0.0f,
      -1.0f, -1.0f,   0.0f, 0.0f,
      -1.0f,  1.0f,   0.0f, 1.0f,
   };

   gl::glCreateBuffers(1, &mScreenDraw.vertBuffer);
   gl::glNamedBufferData(mScreenDraw.vertBuffer, sizeof(vertices), vertices, gl::GL_STATIC_DRAW);

   // Create vertex array
   gl::glCreateVertexArrays(1, &mScreenDraw.vertArray);

   auto fs_position = gl::glGetAttribLocation(mScreenDraw.vertexProgram, "fs_position");
   gl::glEnableVertexArrayAttrib(mScreenDraw.vertArray, fs_position);
   gl::glVertexArrayAttribFormat(mScreenDraw.vertArray, fs_position, 2, gl::GL_FLOAT, gl::GL_FALSE, 0);
   gl::glVertexArrayAttribBinding(mScreenDraw.vertArray, fs_position, 0);

   auto fs_texCoord = gl::glGetAttribLocation(mScreenDraw.vertexProgram, "fs_texCoord");
   gl::glEnableVertexArrayAttrib(mScreenDraw.vertArray, fs_texCoord);
   gl::glVertexArrayAttribFormat(mScreenDraw.vertArray, fs_texCoord, 2, gl::GL_FLOAT, gl::GL_FALSE, 2 * sizeof(gl::GLfloat));
   gl::glVertexArrayAttribBinding(mScreenDraw.vertArray, fs_texCoord, 0);
}

void cglContextInitialise()
{
   glbinding::Binding::initialize();
   setupGlErrorHandling();
}

void cglClear()
{
   gl::glClearColor(0.6f, 0.2f, 0.2f, 1.0f);
   gl::glClear(gl::GL_COLOR_BUFFER_BIT);
}

void cglDrawScanBuffer(gl::GLuint object)
{
   // Setup screen draw shader
   gl::glBindVertexArray(mScreenDraw.vertArray);
   gl::glBindVertexBuffer(0, mScreenDraw.vertBuffer, 0, 4 * sizeof(gl::GLfloat));
   gl::glBindProgramPipeline(mScreenDraw.pipeline);

   // Draw screen quad
   gl::glColorMaski(0, gl::GL_TRUE, gl::GL_TRUE, gl::GL_TRUE, gl::GL_TRUE);
   gl::glBindSampler(0, 0);
   gl::glDisablei(gl::GL_BLEND, 0);
   gl::glDisable(gl::GL_DEPTH_TEST);
   gl::glDisable(gl::GL_STENCIL_TEST);
   gl::glDisable(gl::GL_SCISSOR_TEST);
   gl::glDisable(gl::GL_CULL_FACE);
   gl::glDisable(gl::GL_ALPHA_TEST);
   gl::glBindTextureUnit(0, object);

   gl::glDrawArrays(gl::GL_TRIANGLES, 0, 6);
}