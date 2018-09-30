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
Driver::allocTempBuffer(uint32_t size)
{
   vk::BufferCreateInfo bufferDesc;
   bufferDesc.size = size;
   bufferDesc.usage =
      vk::BufferUsageFlagBits::eTransferSrc |
      vk::BufferUsageFlagBits::eIndexBuffer |
      vk::BufferUsageFlagBits::eUniformBuffer;
   bufferDesc.sharingMode = vk::SharingMode::eExclusive;
   bufferDesc.queueFamilyIndexCount = 0;
   bufferDesc.pQueueFamilyIndices = nullptr;

   VmaAllocationCreateInfo allocDesc = {};
   allocDesc.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

   VkBuffer buffer;
   VmaAllocation allocation;
   vmaCreateBuffer(mAllocator,
                   &static_cast<VkBufferCreateInfo>(bufferDesc),
                   &allocDesc,
                   &buffer,
                   &allocation,
                   nullptr);

   auto sbuffer = new StagingBuffer();
   sbuffer->maximumSize = size;
   sbuffer->size = 0;
   sbuffer->buffer = buffer;
   sbuffer->memory = allocation;

   return sbuffer;
}

StagingBuffer *
Driver::getStagingBuffer(uint32_t size)
{
   StagingBuffer *sbuffer = nullptr;

   if (!sbuffer) {
      for (auto& i = mStagingBuffers.begin(); i != mStagingBuffers.end(); ++i) {
         auto& searchBuffer = *i;
         if (searchBuffer->maximumSize >= size) {
            mStagingBuffers.erase(i);
            sbuffer = searchBuffer;
            break;
         }
      }
   }

   if (!sbuffer) {
      sbuffer = allocTempBuffer(size);
   }

   sbuffer->size = size;

   mActiveSyncWaiter->stagingBuffers.push_back(sbuffer);

   return sbuffer;
}

void
Driver::retireStagingBuffer(StagingBuffer *sbuffer)
{
   if (sbuffer->maximumSize >= 1 * 1024 * 1024) {
      // We don't keep around buffers over 1kb in size
      vmaDestroyBuffer(mAllocator, sbuffer->buffer, sbuffer->memory);
      delete sbuffer;
      return;
   }

   mStagingBuffers.push_back(sbuffer);
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
