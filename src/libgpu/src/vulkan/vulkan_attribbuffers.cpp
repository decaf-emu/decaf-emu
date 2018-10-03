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
      desc.baseAddress = phys_addr(0);
      desc.size = 0;
      desc.stride = 0;
      return desc;
   }

   desc.baseAddress = phys_addr(sq_vtx_constant_word0.BASE_ADDRESS());
   desc.size = sq_vtx_constant_word1.SIZE() + 1;
   desc.stride = sq_vtx_constant_word2.STRIDE();

   return desc;
}

void
Driver::checkCurrentAttribBuffers()
{
   // Must have a vertex shader to describe what to upload...
   decaf_check(mCurrentVertexShader);

   for (auto i = 0u; i < latte::MaxAttribBuffers; ++i) {
      if (!mCurrentVertexShader->shader.inputBuffers[i].isUsed) {
         mCurrentAttribBuffers[i] = nullptr;
         continue;
      }

      auto desc = getAttribBufferDesc(i);

      if (!desc.baseAddress) {
         mCurrentAttribBuffers[i] = nullptr;
         continue;
      }

      mCurrentAttribBuffers[i] = getDataBuffer(desc.baseAddress, desc.size, false);
   }
}

bool
Driver::bindAttribBuffers()
{
   for (auto i = 0u; i < latte::MaxAttribBuffers; ++i) {
      auto buffer = mCurrentAttribBuffers[i];

      if (mCurrentVertexShader->shader.inputBuffers[i].isUsed) {
         if (!buffer) {
            // If the buffer is used, but we don't have one, we should cancel the draw.
            return false;
         }
      }

      if (buffer) {
         mActiveCommandBuffer.bindVertexBuffers(i, { buffer->buffer }, { 0 });
      }
   }

   return true;
}

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
