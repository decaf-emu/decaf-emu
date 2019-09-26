#pragma once
#ifdef DECAF_GL
#include "gpu_graphicsdriver.h"

namespace gpu
{

struct OpenGLDriverDebugInfo : public GraphicsDriverDebugInfo
{
   OpenGLDriverDebugInfo()
   {
      type = GraphicsDriverType::OpenGL;
   }

   uint64_t numFetchShaders = 0;
   uint64_t numVertexShaders = 0;
   uint64_t numPixelShaders = 0;
   uint64_t numShaderPipelines = 0;
   uint64_t numSurfaces = 0;
   uint64_t numDataBuffers = 0;
};

} // namespace gpu

#endif // ifdef DECAF_GL
