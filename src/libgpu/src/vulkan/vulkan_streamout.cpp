#ifdef DECAF_VULKAN
#include "vulkan_driver.h"

namespace vulkan
{

// TODO: We should consolidate the management of all 3 types of
// buffers that we create (memory cache, staging and stream out
// counters).  This will allow us to share the logic for the
// management of those buffers.

static inline void
_barrierStreamContextBuffer(vk::CommandBuffer cmdBuffer, vk::Buffer buffer,
                            vk::PipelineStageFlags srcStage, vk::AccessFlags srcMask,
                            vk::PipelineStageFlags dstStage, vk::AccessFlags dstMask)
{
   vk::BufferMemoryBarrier bufferBarrier;
   bufferBarrier.srcAccessMask = srcMask;
   bufferBarrier.dstAccessMask = dstMask;
   bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   bufferBarrier.buffer = buffer;
   bufferBarrier.offset = 0;
   bufferBarrier.size = VK_WHOLE_SIZE;

   cmdBuffer.pipelineBarrier( srcStage, dstStage, vk::DependencyFlags(), { }, { bufferBarrier }, { });
}

StreamContextObject *
Driver::allocateStreamContext(uint32_t initialOffset)
{
   StreamContextObject *context = nullptr;

   if (!mStreamOutContextPool.empty()) {
      context = mStreamOutContextPool.back();
      mStreamOutContextPool.pop_back();
   }

   if (!context) {
      vk::BufferCreateInfo bufferDesc;
      bufferDesc.size = 4;
      bufferDesc.usage =
         vk::BufferUsageFlagBits::eTransformFeedbackCounterBufferEXT |
         vk::BufferUsageFlagBits::eTransferSrc |
         vk::BufferUsageFlagBits::eTransferDst;
      bufferDesc.sharingMode = vk::SharingMode::eExclusive;
      bufferDesc.queueFamilyIndexCount = 0;
      bufferDesc.pQueueFamilyIndices = nullptr;

      VmaAllocationCreateInfo allocInfo = {};
      allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

      VkBuffer buffer;
      VmaAllocation allocation;
      vmaCreateBuffer(mAllocator,
                      reinterpret_cast<VkBufferCreateInfo*>(&bufferDesc),
                      &allocInfo,
                      &buffer,
                      &allocation,
                      nullptr);

      static uint64_t streamOutCounterIdx = 0;
      setVkObjectName(buffer, fmt::format("soctr_{}", streamOutCounterIdx++).c_str());

      context = new StreamContextObject();
      context->allocation = allocation;
      context->buffer = buffer;
   }

   // Transition this buffer to being filled.
   _barrierStreamContextBuffer(mActiveCommandBuffer,
                               context->buffer,
                               vk::PipelineStageFlagBits::eTransformFeedbackEXT,
                               vk::AccessFlagBits::eTransformFeedbackCounterReadEXT,
                               vk::PipelineStageFlagBits::eTransfer,
                               vk::AccessFlagBits::eTransferWrite);

   // Fill the buffer with the initial value
   mActiveCommandBuffer.fillBuffer(context->buffer, 0, 4, initialOffset);

   // Transition the stream out context buffer to the correct state for having counter
   // data written into it.  It is expected that all contexts will always be in a state
   // ready to receive transform feedback, and readers will switch it in and then back
   // out of this state.
   _barrierStreamContextBuffer(mActiveCommandBuffer,
                               context->buffer,
                               vk::PipelineStageFlagBits::eTransfer,
                               vk::AccessFlagBits::eTransferWrite,
                               vk::PipelineStageFlagBits::eTransformFeedbackEXT,
                               vk::AccessFlagBits::eTransformFeedbackCounterReadEXT);

   return context;
}

void
Driver::releaseStreamContext(StreamContextObject* stream)
{
   mStreamOutContextPool.push_back(stream);
}

void
Driver::readbackStreamContext(StreamContextObject *stream, phys_addr writeAddr)
{
   // Transition the stream-out context to being read for a read by the transfer sytem.
   _barrierStreamContextBuffer(mActiveCommandBuffer,
                               stream->buffer,
                               vk::PipelineStageFlagBits::eTransformFeedbackEXT,
                               vk::AccessFlagBits::eTransformFeedbackCounterWriteEXT,
                               vk::PipelineStageFlagBits::eTransfer,
                               vk::AccessFlagBits::eTransferRead);

   // Grab the memory cache object for the destination
   auto memCache = getDataMemCache(writeAddr, 4);

   // Transition the cache to being a write target, this will cause it to write-back
   // to the GPU whenever this Pm4 Context is completed (or if its needed).
   transitionMemCache(memCache, ResourceUsage::StreamOutCounterWrite);

   // Copy the pointer from our local stream-out buffers into the user supplied region
   vk::BufferCopy copyRegion(0, 0, 4);
   mActiveCommandBuffer.copyBuffer(stream->buffer, memCache->buffer, { copyRegion });

   // Return the stream out context to its normal feedback counter write state to
   // enable it to continue to be used by future stream-out operations.
   _barrierStreamContextBuffer(mActiveCommandBuffer,
                               stream->buffer,
                               vk::PipelineStageFlagBits::eTransfer,
                               vk::AccessFlagBits::eTransferRead,
                               vk::PipelineStageFlagBits::eTransformFeedbackEXT,
                               vk::AccessFlagBits::eTransformFeedbackCounterWriteEXT);
}

StreamOutBufferDesc
Driver::getStreamOutBufferDesc(uint32_t bufferIndex)
{
   // If streamout is disabled, then there is no buffer here.
   auto vgt_strmout_en = getRegister<latte::VGT_STRMOUT_EN>(latte::Register::VGT_STRMOUT_EN);
   if (!vgt_strmout_en.STREAMOUT()) {
      return StreamOutBufferDesc();
   }

   StreamOutBufferDesc desc;

   auto vgt_strmout_buffer_base = getRegister<uint32_t>(latte::Register::VGT_STRMOUT_BUFFER_BASE_0 + 16 * bufferIndex);
   auto vgt_strmout_buffer_offset = getRegister<uint32_t>(latte::Register::VGT_STRMOUT_BUFFER_OFFSET_0 + 16 * bufferIndex);
   auto vgt_strmout_buffer_size = getRegister<uint32_t>(latte::Register::VGT_STRMOUT_BUFFER_SIZE_0 + 16 * bufferIndex);
   auto vgt_strmout_vtx_stride = getRegister<uint32_t>(latte::Register::VGT_STRMOUT_VTX_STRIDE_0 + 16 * bufferIndex);

   decaf_check(vgt_strmout_buffer_offset == 0);

   desc.baseAddress = phys_addr(vgt_strmout_buffer_base << 8);
   desc.size = vgt_strmout_buffer_size << 2;
   desc.stride = vgt_strmout_vtx_stride << 2;

   return desc;
}

bool
Driver::checkCurrentStreamOut()
{
   if (!mCurrentDraw->streamOutEnabled) {
      return true;
   }

   for (auto i = 0; i < latte::MaxStreamOutBuffers; ++i) {
      auto currentDesc = getStreamOutBufferDesc(i);

      if (!currentDesc.baseAddress) {
         mCurrentDraw->streamOutBuffers[i] = nullptr;
         continue;
      }

      // Fetch the memory cache for this buffer
      auto memCache = getDataMemCache(currentDesc.baseAddress, currentDesc.size);

      // Transition the buffer to being a stream out buffer.  This will cause it to be
      // automatically invalidated later on.
      transitionMemCache(memCache, ResourceUsage::StreamOutBuffer);

      mCurrentDraw->streamOutBuffers[i] = memCache;
   }

   return true;
}

void
Driver::bindStreamOutBuffers()
{
   for (auto i = 0; i < latte::MaxStreamOutBuffers; ++i) {
      auto& buffer = mCurrentDraw->streamOutBuffers[i];
      if (!buffer) {
         continue;
      }

      mActiveCommandBuffer.bindTransformFeedbackBuffersEXT(i, { buffer->buffer }, { 0 }, { buffer->size }, mVkDynLoader);
   }
}

void
Driver::beginStreamOut()
{
   std::array<vk::Buffer, latte::MaxStreamOutBuffers> buffers = { vk::Buffer{} };
   std::array<vk::DeviceSize, latte::MaxStreamOutBuffers> offsets = { 0 };
   for (auto i = 0; i < latte::MaxStreamOutBuffers; ++i) {
      auto ctxData = mCurrentDraw->streamOutContext[i];
      if (ctxData) {
         buffers[i] = ctxData->buffer;
      }
   }

   mActiveCommandBuffer.beginTransformFeedbackEXT(0, buffers, offsets, mVkDynLoader);
}

void
Driver::endStreamOut()
{
   std::array<vk::Buffer, latte::MaxStreamOutBuffers> buffers = { vk::Buffer{} };
   std::array<vk::DeviceSize, latte::MaxStreamOutBuffers> offsets = { 0 };
   for (auto i = 0; i < latte::MaxStreamOutBuffers; ++i) {
      auto ctxData = mCurrentDraw->streamOutContext[i];
      if (ctxData) {
         buffers[i] = ctxData->buffer;
      }
   }

   mActiveCommandBuffer.endTransformFeedbackEXT(0, buffers, offsets, mVkDynLoader);
}

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
