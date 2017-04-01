#pragma once
#ifndef DECAF_NOGL
#include "decafsdl_graphics.h"
#include "libdecaf/decaf.h"
#include "libdecaf/decaf_opengl.h"
#include <SDL.h>
#include <glbinding/gl/gl.h>

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

   decaf::GraphicsDriver *
   getDecafDriver() override;

protected:
   void
   drawScanBuffer(gl::GLuint object);

   void
   drawScanBuffers(Viewport &tvViewport,
                   gl::GLuint tvBuffer,
                   Viewport &drcViewport,
                   gl::GLuint drcBuffer);

   void
   drawScanBuffer_MW(gl::GLuint tvBuffer, gl::GLuint drcBuffer);

   void
   initialiseContext();

   void
   initialiseDraw();

protected:
   std::thread mGraphicsThread;

   decaf::OpenGLDriver *mDecafDriver		= nullptr;
   decaf::OpenGLDriver *mDecafDriverDRC		= nullptr;

   SDL_GLContext mContext					= nullptr;
   SDL_GLContext mThreadContext				= nullptr;

   SDL_GLContext mContextDRC				= nullptr;
   SDL_GLContext mThreadContextDRC			= nullptr;

   gl::GLuint mVertexProgram;
   gl::GLuint mPixelProgram;
   gl::GLuint mPipeline;
   gl::GLuint mVertArray;
   gl::GLuint mVertBuffer;
   gl::GLuint mSampler;
   
   gl::GLfloat mBackgroundColour[3];
};

#endif // DECAF_NOGL
