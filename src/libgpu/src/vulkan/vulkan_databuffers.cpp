#ifdef DECAF_VULKAN
#include "vulkan_driver.h"

namespace vulkan
{

DataBufferObject *
Driver::allocateDataBuffer(phys_addr baseAddress, uint32_t size)
{
   vk::BufferCreateInfo bufferDesc;
   bufferDesc.size = size;
   bufferDesc.usage =
      vk::BufferUsageFlagBits::eVertexBuffer |
      vk::BufferUsageFlagBits::eUniformBuffer |
      vk::BufferUsageFlagBits::eTransferDst;
   bufferDesc.sharingMode = vk::SharingMode::eExclusive;
   bufferDesc.queueFamilyIndexCount = 0;
   bufferDesc.pQueueFamilyIndices = nullptr;

   VmaAllocationCreateInfo allocInfo = {};
   allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

   VkBuffer buffer;
   VmaAllocation allocation;
   vmaCreateBuffer(mAllocator,
                   &static_cast<VkBufferCreateInfo>(bufferDesc),
                   &allocInfo,
                   &buffer,
                   &allocation,
                   nullptr);

   DataBufferObject *dataBuffer = new DataBufferObject();
   dataBuffer->baseAddress = baseAddress;
   dataBuffer->size = size;
   dataBuffer->lastHashedIndex = 0;
   dataBuffer->buffer = buffer;
   dataBuffer->memory = allocation;

   return dataBuffer;
}

void
Driver::checkDataBuffer(DataBufferObject *dataBuffer)
{
   if (mActivePm4BufferIndex <= dataBuffer->lastHashedIndex) {
      // We already hashed and uploaded for this PM4 buffer
      return;
   }

   auto dataPtr = phys_cast<void*>(dataBuffer->baseAddress).getRawPointer();
   auto dataSize = dataBuffer->size;
   auto dataHash = DataHash {}.write(dataPtr, dataBuffer->size);
   if (dataBuffer->hash == dataHash) {
      // The buffer is still up to date, no need to upload
      dataBuffer->lastHashedIndex = mActivePm4BufferIndex;
      return;
   }

   dataBuffer->hash = dataHash;
   dataBuffer->lastHashedIndex = mActivePm4BufferIndex;

   auto stagingBuffer = getStagingBuffer(dataSize);
   auto mappedData = mapStagingBuffer(stagingBuffer, false);
   memcpy(mappedData, dataPtr, dataSize);
   unmapStagingBuffer(stagingBuffer, true);

   vk::BufferCopy region;
   region.srcOffset = 0;
   region.dstOffset = 0;
   region.size = dataSize;
   mActiveCommandBuffer.copyBuffer(stagingBuffer->buffer, dataBuffer->buffer, { region });
}

DataBufferObject *
Driver::getDataBuffer(phys_addr baseAddress, uint32_t size, bool discardData)
{
   auto bufferKey = std::make_pair(baseAddress, size);
   auto& buffer = mDataBuffers[bufferKey];

   // Check if we need to create the buffer
   if (!buffer) {
      buffer = allocateDataBuffer(baseAddress, size);
   }

   // Check if we need to hash and upload the buffer
   checkDataBuffer(buffer);

   return buffer;
}

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
