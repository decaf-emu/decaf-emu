#include "replayrenderwidget.h"

ReplayRenderWidget::ReplayRenderWidget(QWidget *parent) :
   QOpenGLWidget(parent)
{
   QSurfaceFormat format;
   format.setDepthBufferSize(24);
   format.setStencilBufferSize(8);
   format.setVersion(4, 5);
   format.setProfile(QSurfaceFormat::CompatibilityProfile);
   setFormat(format);
}

void ReplayRenderWidget::initializeGL()
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

   initializeOpenGLFunctions();

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

   glSamplerParameteri(mSampler, GL_TEXTURE_WRAP_S, static_cast<int>(GL_CLAMP_TO_EDGE));
   glSamplerParameteri(mSampler, GL_TEXTURE_WRAP_T, static_cast<int>(GL_CLAMP_TO_EDGE));
   glSamplerParameteri(mSampler, GL_TEXTURE_MIN_FILTER, static_cast<int>(GL_LINEAR));
   glSamplerParameteri(mSampler, GL_TEXTURE_MAG_FILTER, static_cast<int>(GL_LINEAR));
}

void ReplayRenderWidget::paintGL()
{
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

   // Setup screen draw shader
   glBindVertexArray(mVertArray);
   glBindVertexBuffer(0, mVertBuffer, 0, 4 * sizeof(GLfloat));
   glBindProgramPipeline(mPipeline);

   // Draw screen quad
   glBindSampler(0, mSampler);
   glBindTextureUnit(0, mTvBuffer);

   glDrawArrays(GL_TRIANGLES, 0, 6);
}

void ReplayRenderWidget::displayFrame(unsigned int tv, unsigned int drc)
{
   mTvBuffer = tv;
   mDrcBuffer = drc;
   update();
}
