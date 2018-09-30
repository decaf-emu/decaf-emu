#ifdef DECAF_VULKAN
#include "vulkan_driver.h"

namespace vulkan
{

/*
Staging buffers are used for performing uploads/downloads from the host GPU.
These buffers will only last as long as a single host command buffer does, and
thus all uploading must be done in the context where the buffer is created, or
within a retire task of that particular command buffer.
*/

StagingBuffer *
Driver::getStagingBuffer(uint32_t size, vk::BufferUsageFlags usage)
{
   vk::BufferCreateInfo bufferDesc;
   bufferDesc.size = size;
   bufferDesc.usage = usage;
   bufferDesc.sharingMode = vk::SharingMode::eExclusive;
   bufferDesc.queueFamilyIndexCount = 0;
   bufferDesc.pQueueFamilyIndices = nullptr;

   VmaAllocationCreateInfo allocInfo = {};
   allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

   VkBuffer buffer;
   VmaAllocation allocation;
   vmaCreateBuffer(mAllocator,
                   &static_cast<VkBufferCreateInfo>(bufferDesc),
                   &allocInfo,
                   &buffer,
                   &allocation,
                   nullptr);

   auto sbuffer = new StagingBuffer();
   sbuffer->size = size;
   sbuffer->buffer = buffer;
   sbuffer->memory = allocation;

   mActiveSyncWaiter->stagingBuffers.push_back(sbuffer);

   return sbuffer;
}

void
Driver::retireStagingBuffer(StagingBuffer *sbuffer)
{
   vmaFreeMemory(mAllocator, sbuffer->memory);
   mDevice.destroyBuffer(sbuffer->buffer);
}

void *
Driver::mapStagingBuffer(StagingBuffer *sbuffer, bool flushGpu)
{
   void * data;
   vmaMapMemory(mAllocator, sbuffer->memory, &data);

   if (flushGpu) {
      vmaInvalidateAllocation(mAllocator, sbuffer->memory, 0, VK_WHOLE_SIZE);
   }

   return data;
}

void
Driver::unmapStagingBuffer(StagingBuffer *sbuffer, bool flushCpu)
{
   if (flushCpu) {
      vmaFlushAllocation(mAllocator, sbuffer->memory, 0, VK_WHOLE_SIZE);
   }

   vmaUnmapMemory(mAllocator, sbuffer->memory);
}

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
