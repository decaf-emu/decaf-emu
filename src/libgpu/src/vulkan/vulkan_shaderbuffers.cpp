#ifdef DECAF_VULKAN
#include "vulkan_driver.h"

namespace vulkan
{

void
Driver::checkCurrentGprBuffer(ShaderStage shaderStage)
{
   const void *registerVals = nullptr;

   if (shaderStage == ShaderStage::Vertex) {
      registerVals = &mRegisters[latte::Register::SQ_ALU_CONSTANT0_256 / 4];
   } else if (shaderStage == ShaderStage::Geometry) {
      // No registers are reserved for GS
      return;
   } else if (shaderStage == ShaderStage::Pixel) {
      registerVals = &mRegisters[latte::Register::SQ_ALU_CONSTANT0_0 / 4];
   } else {
      decaf_abort("Unknown shader stage");
   }

   auto gprsBuffer = getStagingBuffer(256 * 4 * 4);
   auto mappedData = mapStagingBuffer(gprsBuffer, false);
   memcpy(mappedData, registerVals, 256 * 4 * 4);
   unmapStagingBuffer(gprsBuffer, true);

   mCurrentGprBuffers[int(shaderStage)] = gprsBuffer;
}

void
Driver::checkCurrentUniformBuffer(ShaderStage shaderStage, uint32_t cbufferIdx)
{
   uint32_t cacheBase;
   uint32_t sizeBase;
   if (shaderStage == ShaderStage::Vertex) {
      cacheBase = latte::Register::SQ_ALU_CONST_CACHE_VS_0;
      sizeBase = latte::Register::SQ_ALU_CONST_BUFFER_SIZE_VS_0;
   } else if (shaderStage == ShaderStage::Geometry) {
      cacheBase = latte::Register::SQ_ALU_CONST_CACHE_GS_0;
      sizeBase = latte::Register::SQ_ALU_CONST_BUFFER_SIZE_GS_0;
   } else if (shaderStage == ShaderStage::Pixel) {
      cacheBase = latte::Register::SQ_ALU_CONST_CACHE_PS_0;
      sizeBase = latte::Register::SQ_ALU_CONST_BUFFER_SIZE_PS_0;
   } else {
      decaf_abort("Unexpected shader stage");
   }

   auto sq_alu_const_cache_vs = getRegister<uint32_t>(cacheBase + 4 * cbufferIdx);
   auto sq_alu_const_buffer_size_vs = getRegister<uint32_t>(sizeBase + 4 * cbufferIdx);

   auto bufferPtr = phys_addr(sq_alu_const_cache_vs << 8);
   auto bufferSize = sq_alu_const_buffer_size_vs << 8;

   if (!bufferPtr || bufferSize == 0) {
      mCurrentUniformBlocks[int(shaderStage)][cbufferIdx] = nullptr;
      return;
   }

   auto buffer = getDataBuffer(bufferPtr, bufferSize);
   mCurrentUniformBlocks[int(shaderStage)][cbufferIdx] = buffer;
}

void
Driver::checkCurrentShaderBuffers()
{
   auto sq_config = getRegister<latte::SQ_CONFIG>(latte::Register::SQ_CONFIG);
   bool isDx9Consts = sq_config.DX9_CONSTS();

   if (!isDx9Consts) {
      for (auto shaderStage = 0; shaderStage < 3; ++shaderStage) {
         mCurrentGprBuffers[shaderStage] = nullptr;

         for (auto i = 0u; i < latte::MaxUniformBlocks; ++i) {
            checkCurrentUniformBuffer(ShaderStage(shaderStage), i);
         }
      }
   } else {
      for (auto shaderStage = 0; shaderStage < 3; ++shaderStage) {
         for (auto i = 0u; i < latte::MaxUniformBlocks; ++i) {
            mCurrentUniformBlocks[shaderStage][i] = nullptr;
         }

         checkCurrentGprBuffer(ShaderStage(shaderStage));
      }
   }
}

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
