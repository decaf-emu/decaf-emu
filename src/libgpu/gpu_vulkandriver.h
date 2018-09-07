#pragma once
#ifdef DECAF_VULKAN
#include "gpu_graphicsdriver.h"

namespace gpu
{

class VulkanDriver : public GraphicsDriver
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

   virtual ~VulkanDriver() = default;

   virtual DebuggerInfo *
   getDebuggerInfo() = 0;

};

} // namespace gpu

#endif // DECAF_VULKAN
