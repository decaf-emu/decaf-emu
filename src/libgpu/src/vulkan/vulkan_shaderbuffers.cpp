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

   auto gprsBuffer = getStagingBuffer(256 * 4 * 4, vk::BufferUsageFlagBits::eUniformBuffer);
   auto mappedData = mapStagingBuffer(gprsBuffer, false);
   memcpy(mappedData, registerVals, 256 * 4 * 4);
   unmapStagingBuffer(gprsBuffer, true);

   mCurrentUniformBlocks[int(shaderStage)][0] = gprsBuffer;
}

void
Driver::checkCurrentUniformBuffer(ShaderStage shaderStage, uint32_t cbufferIdx)
{
   decaf_abort("We do not yet support cbuffer-based shaders");
}

void
Driver::checkCurrentShaderBuffers()
{
   auto sq_config = getRegister<latte::SQ_CONFIG>(latte::Register::SQ_CONFIG);
   bool isDx9Consts = sq_config.DX9_CONSTS();

   for (auto shaderStage = 0; shaderStage < 3; ++shaderStage) {
      if (!isDx9Consts) {
         for (auto i = 0u; i < latte::MaxUniformBlocks; ++i) {
            checkCurrentUniformBuffer(ShaderStage(shaderStage), i);
         }
      } else {
         for (auto i = 1u; i < latte::MaxUniformBlocks; ++i) {
            mCurrentUniformBlocks[shaderStage][i] = nullptr;
         }
         checkCurrentGprBuffer(ShaderStage(shaderStage));
      }
   }
}

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
