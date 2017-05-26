#pragma once
#ifdef DECAF_GL
#include "decafsdl_graphics.h"

#include <SDL.h>
#include <glbinding/gl/gl.h>
#include <libdecaf/decaf.h>
#include <libgpu/gpu_opengldriver.h>

class DecafSDLOpenGL : public DecafSDLGraphics
{
public:
   DecafSDLOpenGL();
   ~DecafSDLOpenGL() override;

   bool
   initialise(int width,
              int height) override;

   void
   shutdown() override;

   void
   renderFrame(Viewport &tv,
               Viewport &drc) override;

   gpu::GraphicsDriver *
   getDecafDriver() override;

   decaf::DebugUiRenderer *
   getDecafDebugUiRenderer() override;

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

protected:
   std::thread mGraphicsThread;
   gpu::OpenGLDriver *mDecafDriver = nullptr;
   decaf::DebugUiRenderer *mDebugUiRenderer = nullptr;

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
