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
Driver::getStagingBuffer(uint32_t size)
{
   vk::BufferCreateInfo bufferDesc;
   bufferDesc.size = size;
   bufferDesc.usage = vk::BufferUsageFlagBits::eTransferSrc;
   bufferDesc.sharingMode = vk::SharingMode::eExclusive;
   bufferDesc.queueFamilyIndexCount = 0;
   bufferDesc.pQueueFamilyIndices = nullptr;
   auto buffer = mDevice.createBuffer(bufferDesc);

   auto bufferMemReqs = mDevice.getBufferMemoryRequirements(buffer);

   vk::MemoryAllocateInfo allocDesc;
   allocDesc.allocationSize = bufferMemReqs.size;
   allocDesc.memoryTypeIndex = findMemoryType(bufferMemReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible);
   auto bufferMem = mDevice.allocateMemory(allocDesc);

   mDevice.bindBufferMemory(buffer, bufferMem, 0);

   auto sbuffer = new StagingBuffer();
   sbuffer->size = size;
   sbuffer->buffer = buffer;
   sbuffer->memory = bufferMem;

   mActiveSyncWaiter->stagingBuffers.push_back(sbuffer);

   return sbuffer;
}

void
Driver::retireStagingBuffer(StagingBuffer *sbuffer)
{
   mDevice.freeMemory(sbuffer->memory);
   mDevice.destroyBuffer(sbuffer->buffer);
}

void *
Driver::mapStagingBuffer(StagingBuffer *sbuffer, bool flushGpu)
{
   auto data = mDevice.mapMemory(sbuffer->memory, 0, VK_WHOLE_SIZE, vk::MemoryMapFlags());

   if (flushGpu) {
      mDevice.invalidateMappedMemoryRanges({ vk::MappedMemoryRange(sbuffer->memory, 0, VK_WHOLE_SIZE) });
   }

   return data;
}

void
Driver::unmapStagingBuffer(StagingBuffer *sbuffer, bool flushCpu)
{
   if (flushCpu) {
      mDevice.flushMappedMemoryRanges({ vk::MappedMemoryRange(sbuffer->memory, 0, VK_WHOLE_SIZE) });
   }

   mDevice.unmapMemory(sbuffer->memory);
}

} // namespace vulkan

#endif // DECAF_VULKAN
