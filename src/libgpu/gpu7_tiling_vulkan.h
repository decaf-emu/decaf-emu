#pragma once
#ifdef DECAF_VULKAN
#include "gpu7_tiling.h"

#include <vulkan/vulkan.hpp>
#include <map>

namespace gpu7::tiling::vulkan
{

struct RetileInfo
{
   uint32_t firstSlice;
   uint32_t numSlices;
   bool isTiled;
   bool isMacroTiled;
   bool isDepth;
   uint32_t bitsPerElement;
   uint32_t macroTileWidth;
   uint32_t macroTileHeight;

   // Used for both micro and macro tiling
   uint32_t dstStride;
   uint32_t microTileBytes;
   uint32_t numTilesPerRow;
   uint32_t numTileRows;
   uint32_t sliceOffset;
   uint32_t sliceBytes;
   uint32_t microTileThickness;
   uint32_t bankSwapWidth;

   // Used only for micro tiling
   uint32_t sampleOffset;
   AddrTileMode tileMode;
   uint32_t macroTileBytes;
   uint32_t bankSwizzle;
   uint32_t pipeSwizzle;
};

// Samples are not currently supported
RetileInfo
calculateRetileInfo(const SurfaceDescription &surface,
                    uint32_t firstSlice,
                    uint32_t numSlices);

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
   tile(vk::CommandBuffer &commandBuffer,
        vk::Buffer dstBuffer, uint32_t dstOffset,
        vk::Buffer srcBuffer, uint32_t srcOffset,
        const RetileInfo& retileInfo)
   {
      return retile(false,
                    commandBuffer,
                    dstBuffer, dstOffset,
                    srcBuffer, srcOffset,
                    retileInfo);
   }

   RetileHandle
   untile(vk::CommandBuffer &commandBuffer,
          vk::Buffer dstBuffer, uint32_t dstOffset,
          vk::Buffer srcBuffer, uint32_t srcOffset,
          const RetileInfo& retileInfo)
   {
      return retile(true,
                    commandBuffer,
                    dstBuffer, dstOffset,
                    srcBuffer, srcOffset,
                    retileInfo);
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
          vk::CommandBuffer &commandBuffer,
          vk::Buffer dstBuffer, uint32_t dstOffset,
          vk::Buffer srcBuffer, uint32_t srcOffset,
          const RetileInfo& retileInfo);

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
