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
   initialise(int width, int height) override;

   void
   shutdown() override;

   void
   renderFrame(float tv[4], float drc[4]) override;

   decaf::GraphicsDriver *
   getDecafDriver() override;

protected:
   void drawScanBuffer(gl::GLuint object);
   void drawScanBuffers(float tvViewport[4], gl::GLuint tvBuffer, float drcViewport[4], gl::GLuint drcBuffer);
   void initialiseContext();
   void initialiseDraw();

   std::thread mGraphicsThread;
   decaf::OpenGLDriver *mDecafDriver = nullptr;

   SDL_GLContext mContext = nullptr;
   SDL_GLContext mThreadContext = nullptr;

   gl::GLuint mVertexProgram;
   gl::GLuint mPixelProgram;
   gl::GLuint mPipeline;
   gl::GLuint mVertArray;
   gl::GLuint mVertBuffer;
   gl::GLuint mSampler;

};

#endif // DECAF_NOGL
