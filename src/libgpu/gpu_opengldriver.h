#pragma once
#ifndef DECAF_NOGL
#include "gpu_graphicsdriver.h"
#include <glbinding/gl/gl.h>

namespace gpu
{

class OpenGLDriver : public GraphicsDriver
{
public:
   using SwapFunction = std::function<void(gl::GLuint, gl::GLuint)>;

   virtual ~OpenGLDriver()
   {
   }

   virtual void
   getSwapBuffers(gl::GLuint *tv,
                  gl::GLuint *drc) = 0;

   virtual void
   syncPoll(const SwapFunction &swapFunc) = 0;
};

} // namespace gpu

#endif // DECAF_NOGL
