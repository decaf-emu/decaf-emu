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
   sbuffer->activeUsage = ResourceUsage::Undefined;
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

   auto alignedSizeBit = 0u;
   for (auto i = size >> 12; i > 0; i >>= 1, alignedSizeBit++);
   auto alignedSize = 1 << (alignedSizeBit + 12);

   if (!sbuffer) {
      auto typeIndex = static_cast<uint32_t>(type);
      auto &stagingBuffers = mStagingBuffers[typeIndex][alignedSizeBit];

      if (!stagingBuffers.empty()) {
         sbuffer = stagingBuffers.back();
         stagingBuffers.pop_back();

         // This is just to double-check that our algorithm is working
         // as it is intended to be working...
         decaf_check(sbuffer->maximumSize >= size);
      }
   }

   if (!sbuffer) {
      sbuffer = _allocStagingBuffer(alignedSize, type);
      sbuffer->poolIndex = alignedSizeBit;
   }

   sbuffer->size = size;

   mActiveSyncWaiter->stagingBuffers.push_back(sbuffer);

   return sbuffer;
}

void
Driver::retireStagingBuffer(StagingBuffer *sbuffer)
{
   auto typeIndex = static_cast<uint32_t>(sbuffer->type);
   auto poolIndex = sbuffer->poolIndex;
   mStagingBuffers[typeIndex][poolIndex].push_back(sbuffer);
}

void
Driver::transitionStagingBuffer(StagingBuffer *sbuffer, ResourceUsage usage)
{
   // If we are already set to the correct usage, no need to do any additional
   // work.  It is implied that a transition would need to happen for a change
   // to have occurred to the staging buffer.
   if (sbuffer->activeUsage == usage) {
      return;
   }

   auto srcMeta = getResourceUsageMeta(sbuffer->activeUsage);
   auto dstMeta = getResourceUsageMeta(usage);

   vk::BufferMemoryBarrier bufferBarrier;
   bufferBarrier.srcAccessMask = srcMeta.accessFlags;
   bufferBarrier.dstAccessMask = dstMeta.accessFlags;
   bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   bufferBarrier.buffer = sbuffer->buffer;
   bufferBarrier.offset = 0;
   bufferBarrier.size = VK_WHOLE_SIZE;

   mActiveCommandBuffer.pipelineBarrier(
      srcMeta.stageFlags,
      dstMeta.stageFlags,
      vk::DependencyFlags(),
      {},
      { bufferBarrier },
      {});

   sbuffer->activeUsage = usage;
}

void
Driver::copyToStagingBuffer(StagingBuffer *sbuffer, uint32_t offset, const void *data, uint32_t size)
{
   decaf_check(sbuffer->type == StagingBufferType::CpuToGpu);

   // Transition the buffer to allow safe writing
   transitionStagingBuffer(sbuffer, ResourceUsage::HostWrite);

   // Copy the data into the staging buffer.
   memcpy(static_cast<uint8_t*>(sbuffer->mappedPtr) + offset, data, size);

   // Flush the allocation to make the CPU write visible to the GPU.
   vmaFlushAllocation(mAllocator, sbuffer->memory, 0, VK_WHOLE_SIZE);
}

void
Driver::copyFromStagingBuffer(StagingBuffer *sbuffer, uint32_t offset, void *data, uint32_t size)
{
   decaf_check(sbuffer->type == StagingBufferType::GpuToCpu);

   // In the case of copying FROM the staging buffer, we actually only check that the
   // correct usage is configured.  This is because the read occurs later after the
   // transition, when we don't have a command buffer or a sync path to transition.
   decaf_check(sbuffer->activeUsage == ResourceUsage::HostRead);

   // Invalidate the allocation to make the GPU writes visible to the CPU
   vmaInvalidateAllocation(mAllocator, sbuffer->memory, 0, VK_WHOLE_SIZE);

   // Copy the data from the staging buffer
   memcpy(data, static_cast<uint8_t*>(sbuffer->mappedPtr) + offset, size);
}

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
