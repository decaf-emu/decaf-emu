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
Driver::_allocStagingBuffer(uint32_t size, StagingBufferType type)
{
   // Lets at least align our staging buffers to 1kb...
   size = align_up(size, 1024);

   vk::BufferUsageFlags bufferUsage;
   VmaMemoryUsage allocUsage;

   if (type == StagingBufferType::CpuToGpu) {
      allocUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
      bufferUsage =
         vk::BufferUsageFlagBits::eTransferSrc |
         vk::BufferUsageFlagBits::eTransferDst |
         vk::BufferUsageFlagBits::eIndexBuffer |
         vk::BufferUsageFlagBits::eUniformBuffer;
   } else if (type == StagingBufferType::GpuToCpu) {
      allocUsage = VMA_MEMORY_USAGE_GPU_TO_CPU;
      bufferUsage =
         vk::BufferUsageFlagBits::eTransferSrc |
         vk::BufferUsageFlagBits::eTransferDst;
   } else if (type == StagingBufferType::GpuToGpu) {
      allocUsage = VMA_MEMORY_USAGE_GPU_ONLY;
      bufferUsage =
         vk::BufferUsageFlagBits::eTransferSrc |
         vk::BufferUsageFlagBits::eTransferDst |
         vk::BufferUsageFlagBits::eStorageBuffer;
   }

   vk::BufferCreateInfo bufferDesc;
   bufferDesc.size = size;
   bufferDesc.usage = bufferUsage;
   bufferDesc.sharingMode = vk::SharingMode::eExclusive;
   bufferDesc.queueFamilyIndexCount = 0;
   bufferDesc.pQueueFamilyIndices = nullptr;

   VmaAllocationCreateInfo allocDesc = {};
   allocDesc.usage = allocUsage;

   VkBuffer buffer;
   VmaAllocation allocation;
   CHECK_VK_RESULT(
      vmaCreateBuffer(mAllocator,
                      reinterpret_cast<VkBufferCreateInfo*>(&bufferDesc),
                      &allocDesc,
                      &buffer,
                      &allocation,
                      nullptr));

   static uint64_t stagingBufferIdx = 0;
   setVkObjectName(buffer, fmt::format("stg_{}_{}_{}",
                                       stagingBufferIdx++,
                                       static_cast<uint32_t>(type),
                                       size).c_str());

   auto sbuffer = new StagingBuffer();
   sbuffer->type = type;
   sbuffer->maximumSize = size;
   sbuffer->size = 0;
   sbuffer->buffer = buffer;
   sbuffer->memory = allocation;
   sbuffer->mappedPtr = nullptr;

   if (type == StagingBufferType::GpuToCpu || type == StagingBufferType::CpuToGpu) {
      CHECK_VK_RESULT(vmaMapMemory(mAllocator, sbuffer->memory, &sbuffer->mappedPtr));
   }

   return sbuffer;
}

StagingBuffer *
Driver::getStagingBuffer(uint32_t size, StagingBufferType type)
{
   StagingBuffer *sbuffer = nullptr;

   if (!sbuffer) {
      for (auto i = mStagingBuffers.begin(); i != mStagingBuffers.end(); ++i) {
         auto searchBuffer = *i;
         if (searchBuffer->type == type && searchBuffer->maximumSize >= size) {
            mStagingBuffers.erase(i);
            sbuffer = searchBuffer;
            break;
         }
      }
   }

   if (!sbuffer) {
      sbuffer = _allocStagingBuffer(size, type);
   }

   sbuffer->size = size;

   mActiveSyncWaiter->stagingBuffers.push_back(sbuffer);

   return sbuffer;
}

void
Driver::retireStagingBuffer(StagingBuffer *sbuffer)
{
   mStagingBuffers.push_back(sbuffer);
}

void *
Driver::mapStagingBuffer(StagingBuffer *sbuffer)
{
   if (sbuffer->type == StagingBufferType::GpuToCpu) {
      vmaInvalidateAllocation(mAllocator, sbuffer->memory, 0, VK_WHOLE_SIZE);
   }

   return sbuffer->mappedPtr;
}

void
Driver::unmapStagingBuffer(StagingBuffer *sbuffer)
{
   if (sbuffer->type == StagingBufferType::CpuToGpu) {
      vmaFlushAllocation(mAllocator, sbuffer->memory, 0, VK_WHOLE_SIZE);
   }
}

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
