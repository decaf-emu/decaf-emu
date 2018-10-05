#ifdef DECAF_VULKAN
#include "vulkan_driver.h"

namespace vulkan
{

VertexBufferDesc
Driver::getAttribBufferDesc(uint32_t bufferIndex)
{
   VertexBufferDesc desc;

   auto resourceOffset = (latte::SQ_RES_OFFSET::VS_ATTRIB_RESOURCE_0 + bufferIndex) * 7;
   auto sq_vtx_constant_word0 = getRegister<latte::SQ_VTX_CONSTANT_WORD0_N>(latte::Register::SQ_RESOURCE_WORD0_0 + 4 * resourceOffset);
   auto sq_vtx_constant_word1 = getRegister<latte::SQ_VTX_CONSTANT_WORD1_N>(latte::Register::SQ_RESOURCE_WORD1_0 + 4 * resourceOffset);
   auto sq_vtx_constant_word2 = getRegister<latte::SQ_VTX_CONSTANT_WORD2_N>(latte::Register::SQ_RESOURCE_WORD2_0 + 4 * resourceOffset);
   auto sq_vtx_constant_word6 = getRegister<latte::SQ_VTX_CONSTANT_WORD6_N>(latte::Register::SQ_RESOURCE_WORD6_0 + 4 * resourceOffset);

   decaf_check(sq_vtx_constant_word2.BASE_ADDRESS_HI() == 0);

   if (sq_vtx_constant_word6.TYPE() != latte::SQ_TEX_VTX_TYPE::VALID_BUFFER) {
      desc.baseAddress = 0;
      desc.size = 0;
      desc.stride = 0;
      return desc;
   }

   desc.baseAddress = sq_vtx_constant_word0.BASE_ADDRESS();
   desc.size = sq_vtx_constant_word1.SIZE();
   desc.stride = sq_vtx_constant_word2.STRIDE();

   if (!desc.size) {
      desc.baseAddress = 0;
   }

   return desc;
}

void
Driver::bindAttribBuffers()
{
   // Must have a vertex shader to describe what to upload...
   decaf_check(mCurrentVertexShader);

   for (auto i = 0u; i < latte::MaxAttribBuffers; ++i) {
      if (!mCurrentVertexShader->shader.inputBuffers[i].isUsed) {
         continue;
      }

      auto desc = getAttribBufferDesc(i);

      if (!desc.baseAddress) {
         continue;
      }

      auto bufferData = phys_cast<void*>(phys_addr(desc.baseAddress)).getRawPointer();
      auto bufferSize = desc.size + 1;

      auto buffer = getStagingBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer);
      auto mappedData = mapStagingBuffer(buffer, false);
      memcpy(mappedData, bufferData, bufferSize);
      unmapStagingBuffer(buffer, true);

      mActiveCommandBuffer.bindVertexBuffers(i, { buffer->buffer }, { 0 });
   }
}

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
