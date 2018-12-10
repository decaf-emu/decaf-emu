#ifdef DECAF_VULKAN
#include "vulkan_driver.h"

namespace vulkan
{

void
Driver::dispatchGpuTile(vk::CommandBuffer &commandBuffer,
                        vk::Buffer dstBuffer, uint32_t dstOffset,
                        vk::Buffer srcBuffer, uint32_t srcOffset,
                        const gpu7::tiling::vulkan::RetileInfo& retileInfo)
{
   auto handle = mGpuRetiler.tile(commandBuffer, dstBuffer, dstOffset, srcBuffer, srcOffset, retileInfo);
   mActiveSyncWaiter->retileHandles.push_back(handle);
}

void
Driver::dispatchGpuUntile(vk::CommandBuffer &commandBuffer,
                          vk::Buffer dstBuffer, uint32_t dstOffset,
                          vk::Buffer srcBuffer, uint32_t srcOffset,
                          const gpu7::tiling::vulkan::RetileInfo& retileInfo)
{
   auto handle = mGpuRetiler.untile(commandBuffer, dstBuffer, dstOffset, srcBuffer, srcOffset, retileInfo);
   mActiveSyncWaiter->retileHandles.push_back(handle);
}

}

#endif // DECAF_VULKAN
