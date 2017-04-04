#pragma once
#ifndef DECAF_NOGL
#include "common/gl.h"
#include "decafsdl_graphics.h"
#include "libdecaf/decaf.h"
#include "libdecaf/decaf_opengl.h"
#include <SDL.h>

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
   drawScanBuffer(GLuint object);

   void
   drawScanBuffers(Viewport &tvViewport,
                   GLuint tvBuffer,
                   Viewport &drcViewport,
                   GLuint drcBuffer);

   void
   initialiseContext();

   void
   initialiseDraw();

protected:
   std::thread mGraphicsThread;
   decaf::OpenGLDriver *mDecafDriver = nullptr;

   SDL_GLContext mContext = nullptr;
   SDL_GLContext mThreadContext = nullptr;

   GLuint mVertexProgram;
   GLuint mPixelProgram;
   GLuint mPipeline;
   GLuint mVertArray;
   GLuint mVertBuffer;
   GLuint mSampler;
   
   GLfloat mBackgroundColour[3];
};

#endif // DECAF_NOGL
