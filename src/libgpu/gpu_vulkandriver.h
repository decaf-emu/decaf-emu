#pragma once
#ifdef DECAF_VULKAN
#include "gpu_graphicsdriver.h"
#include <vulkan/vulkan.hpp>

namespace gpu
{

struct VulkanDriverDebugInfo : GraphicsDriverDebugInfo
{
   VulkanDriverDebugInfo()
   {
      type = GraphicsDriverType::Vulkan;
   }

   uint64_t numVertexShaders = 0;
   uint64_t numGeometryShaders = 0;
   uint64_t numPixelShaders = 0;
   uint64_t numRenderPasses = 0;
   uint64_t numPipelines = 0;
   uint64_t numSamplers = 0;
   uint64_t numSurfaces = 0;
   uint64_t numDataBuffers = 0;
};

} // namespace gpu

#endif // DECAF_VULKAN
