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

bool
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
         // If the vertex shader takes this as an input, but there is no
         // actual buffer specified, we should fail our draw entirely.
         return false;
      }

      auto memCache = getDataMemCache(desc.baseAddress, desc.size);

      transitionMemCache(memCache, ResourceUsage::AttributeBuffer);

      mCurrentAttribBuffers[i] = memCache;
   }

   return true;
}

void
Driver::bindAttribBuffers()
{
   for (auto i = 0u; i < latte::MaxAttribBuffers; ++i) {
      auto buffer = mCurrentAttribBuffers[i];
      if (buffer) {
         mActiveCommandBuffer.bindVertexBuffers(i, { buffer->buffer }, { 0 });
      }
   }
}

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
