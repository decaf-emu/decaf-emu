#pragma once
#ifdef DECAF_GL
#include "decafsdl_graphics.h"

#include <glbinding/gl/gl.h>
#include <libdecaf/decaf.h>
#include <libgpu/gpu_opengldriver.h>
#include <libdebugui/debugui.h>
#include <SDL.h>
#include <spdlog/spdlog.h>

class DecafSDLOpenGL : public DecafSDLGraphics
{
public:
   DecafSDLOpenGL();
   ~DecafSDLOpenGL() override;

   bool
   initialise(int width,
              int height,
              bool renderDebugger = true) override;

   void
   shutdown() override;

   void
   windowResized() override;

   void
   renderFrame(Viewport &tv,
               Viewport &drc) override;

   gpu::GraphicsDriver *
   getDecafDriver() override;

   debugui::Renderer *
   getDebugUiRenderer() override;

   void
   getWindowSize(int *w, int *h) override;

protected:
   void
   drawScanBuffer(gl::GLuint object);

   void
   drawScanBuffers(Viewport &tvViewport,
                   gl::GLuint tvBuffer,
                   Viewport &drcViewport,
                   gl::GLuint drcBuffer);

   void
   initialiseContext();

   void
   initialiseDraw();

private:
   static void GL_APIENTRY
   debugMessageCallback(gl::GLenum source,
                        gl::GLenum type,
                        gl::GLuint id,
                        gl::GLenum severity,
                        gl::GLsizei length,
                        const gl::GLchar* message,
                        const void *userParam);

protected:
   std::shared_ptr<spdlog::logger> mLog;
   std::thread mGraphicsThread;
   gpu::OpenGLDriver *mDecafDriver = nullptr;
   debugui::OpenGLRenderer *mDebugUiRenderer = nullptr;

   SDL_GLContext mContext = nullptr;
   SDL_GLContext mThreadContext = nullptr;

   gl::GLuint mVertexProgram;
   gl::GLuint mPixelProgram;
   gl::GLuint mPipeline;
   gl::GLuint mVertArray;
   gl::GLuint mVertBuffer;
   gl::GLuint mSampler;

   gl::GLfloat mBackgroundColour[3];

   size_t mFramesPresented = 0;
};

#endif // ifdef DECAF_GL
