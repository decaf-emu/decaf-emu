#pragma once
#ifdef DECAF_GL
#include "gpu_graphicsdriver.h"
#include <glbinding/gl/gl.h>

namespace gpu
{

class OpenGLDriver : public GraphicsDriver
{
public:
   struct DebuggerInfo
   {
      uint64_t numFetchShaders = 0;
      uint64_t numVertexShaders = 0;
      uint64_t numPixelShaders = 0;
      uint64_t numShaderPipelines = 0;
      uint64_t numSurfaces = 0;
      uint64_t numDataBuffers = 0;
   };

   virtual ~OpenGLDriver() = default;

   virtual void
   initialise() = 0;

   virtual void
   shutdown() = 0;

   virtual void
   getSwapBuffers(gl::GLuint *tv,
                  gl::GLuint *drc) = 0;

   virtual DebuggerInfo *
   getDebuggerInfo() = 0;
};

} // namespace gpu

#endif // ifdef DECAF_GL
