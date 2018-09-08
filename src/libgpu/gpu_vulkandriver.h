#pragma once
#ifdef DECAF_VULKAN
#include "gpu_graphicsdriver.h"
#include <vulkan/vulkan.hpp>

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

   virtual void
   initialise(vk::PhysicalDevice physDevice,
              vk::Device device,
              vk::Queue queue,
              uint32_t queueFamilyIndex) = 0;

   virtual void
   shutdown() = 0;

   virtual void
   getSwapBuffers(vk::Image &tvImage,
                  vk::ImageView &tvView,
                  vk::Image &drcImage,
                  vk::ImageView &drcView) = 0;

   virtual DebuggerInfo *
   getDebuggerInfo() = 0;

};

} // namespace gpu

#endif // DECAF_VULKAN
