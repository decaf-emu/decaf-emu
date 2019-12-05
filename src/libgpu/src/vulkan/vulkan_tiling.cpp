#ifdef DECAF_VULKAN
#include "vulkan_driver.h"

namespace vulkan
{

void
Driver::dispatchGpuTile(const gpu7::tiling::RetileInfo& retileInfo,
                        vk::CommandBuffer &commandBuffer,
                        vk::Buffer dstBuffer, uint32_t dstOffset,
                        vk::Buffer srcBuffer, uint32_t srcOffset,
                        uint32_t firstSlice, uint32_t numSlices)
{
   auto handle = mGpuRetiler.tile(retileInfo,
                                  commandBuffer,
                                  dstBuffer, dstOffset,
                                  srcBuffer, srcOffset,
                                  firstSlice, numSlices);
   mActiveSyncWaiter->retileHandles.push_back(handle);
}

void
Driver::dispatchGpuUntile(const gpu7::tiling::RetileInfo& retileInfo,
                          vk::CommandBuffer &commandBuffer,
                          vk::Buffer dstBuffer, uint32_t dstOffset,
                          vk::Buffer srcBuffer, uint32_t srcOffset,
                          uint32_t firstSlice, uint32_t numSlices)
{
   auto handle = mGpuRetiler.untile(retileInfo,
                                    commandBuffer,
                                    dstBuffer, dstOffset,
                                    srcBuffer, srcOffset,
                                    firstSlice, numSlices);
   mActiveSyncWaiter->retileHandles.push_back(handle);
}

}

#endif // DECAF_VULKAN
