#pragma once
#ifdef DECAF_VULKAN
#include "gpu7_tiling.h"

#include <common/vulkan_hpp.h>
#include <map>

namespace gpu7::tiling::vulkan
{

typedef void * RetileHandle;

class Retiler
{
public:
   Retiler()
   {
   }

   void
   initialise(vk::Device device);

   RetileHandle
   tile(const RetileInfo& retileInfo,
        vk::CommandBuffer &commandBuffer,
        vk::Buffer dstBuffer, uint32_t dstOffset,
        vk::Buffer srcBuffer, uint32_t srcOffset,
        uint32_t firstSlice, uint32_t numSlices)
   {
      return retile(false,
                    retileInfo,
                    commandBuffer,
                    dstBuffer, dstOffset,
                    srcBuffer, srcOffset,
                    firstSlice, numSlices);
   }

   RetileHandle
   untile(const RetileInfo& retileInfo,
          vk::CommandBuffer &commandBuffer,
          vk::Buffer dstBuffer, uint32_t dstOffset,
          vk::Buffer srcBuffer, uint32_t srcOffset,
          uint32_t firstSlice, uint32_t numSlices)
   {
      return retile(true,
                    retileInfo,
                    commandBuffer,
                    srcBuffer, srcOffset,
                    dstBuffer, dstOffset,
                    firstSlice, numSlices);
   }

   void
   releaseHandle(RetileHandle handle)
   {
      releaseHandle(reinterpret_cast<HandleImpl*>(handle));
   }

private:
   struct HandleImpl
   {
      vk::DescriptorPool descriptorPool;
      vk::DescriptorSet descriptorSet;
   };

   HandleImpl *
   allocateHandle();

   void
   releaseHandle(HandleImpl *handle);

   RetileHandle
   retile(bool wantsUntile,
          const RetileInfo& retileInfo,
          vk::CommandBuffer &commandBuffer,
          vk::Buffer tiledBuffer, uint32_t tiledOffset,
          vk::Buffer untiledBuffer, uint32_t untiledOffset,
          uint32_t firstSlice, uint32_t numSlices);

   void
   retile(bool wantsUntile,
          const RetileInfo& retileInfo,
          vk::DescriptorSet& descriptorSet,
          vk::CommandBuffer& commandBuffer,
          vk::Buffer tiledBuffer, uint32_t tiledOffset,
          vk::Buffer untiledBuffer, uint32_t untiledOffset,
          uint32_t firstSlice, uint32_t numSlices);

protected:
   vk::Device mDevice;
   vk::PipelineLayout mPipelineLayout;
   vk::ShaderModule mShader;
   std::map<uint32_t, vk::Pipeline> mPipelines;
   vk::DescriptorSetLayout mDescriptorSetLayout;
   std::vector<HandleImpl *> mHandlesPool;

};

}

#endif // DECAF_VULKAN
