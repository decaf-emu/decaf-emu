#ifdef DECAF_VULKAN
#include "vulkan_driver.h"
#include "vulkan_utils.h"

namespace vulkan
{

SamplerDesc
Driver::getSamplerDesc(ShaderStage shaderStage, uint32_t samplerIdx)
{
   uint32_t samplerBaseIdx;
   if (shaderStage == ShaderStage::Vertex) {
      samplerBaseIdx = 1;
   } else if (shaderStage == ShaderStage::Geometry) {
      samplerBaseIdx = 2;
   } else if (shaderStage == ShaderStage::Pixel) {
      samplerBaseIdx = 0;
   } else {
      decaf_abort("Unknown shader stage");
   }

   SamplerDesc desc;

   auto i = samplerBaseIdx + samplerIdx;
   desc.texSamplerWord0 = getRegister<latte::SQ_TEX_SAMPLER_WORD0_N>(latte::Register::SQ_TEX_SAMPLER_WORD0_0 + 4 * (i * 3));
   desc.texSamplerWord1 = getRegister<latte::SQ_TEX_SAMPLER_WORD1_N>(latte::Register::SQ_TEX_SAMPLER_WORD1_0 + 4 * (i * 3));
   desc.texSamplerWord2 = getRegister<latte::SQ_TEX_SAMPLER_WORD2_N>(latte::Register::SQ_TEX_SAMPLER_WORD2_0 + 4 * (i * 3));

   return desc;
}

void
Driver::checkCurrentSampler(ShaderStage shaderStage, uint32_t samplerIdx)
{
   mCurrentSamplers[int(shaderStage)][samplerIdx] = nullptr;

   if (shaderStage == ShaderStage::Vertex) {
      if (!mCurrentVertexShader || !mCurrentVertexShader->shader.samplerUsed[samplerIdx]) {
         return;
      }
   } else if (shaderStage == ShaderStage::Geometry) {
      if (!mCurrentGeometryShader || !mCurrentGeometryShader->shader.samplerUsed[samplerIdx]) {
         return;
      }
   } else if (shaderStage == ShaderStage::Pixel) {
      if (!mCurrentPixelShader || !mCurrentPixelShader->shader.samplerUsed[samplerIdx]) {
         return;
      }
   } else {
      decaf_abort("Unknown shader stage");
   }

   HashedDesc<SamplerDesc> currentDesc = getSamplerDesc(ShaderStage(shaderStage), samplerIdx);

   auto &foundSamp = mSamplers[currentDesc.hash()];
   if (foundSamp) {
      mCurrentSamplers[int(shaderStage)][samplerIdx] = foundSamp;
      return;
   }

   foundSamp = new SamplerObject();
   foundSamp->desc = currentDesc;

   vk::SamplerCreateInfo samplerDesc;
   samplerDesc.magFilter = getVkXyTextureFilter(currentDesc->texSamplerWord0.XY_MAG_FILTER());
   samplerDesc.minFilter = getVkXyTextureFilter(currentDesc->texSamplerWord0.XY_MIN_FILTER());
   samplerDesc.mipmapMode = getVkZTextureFilter(currentDesc->texSamplerWord0.MIP_FILTER());
   samplerDesc.addressModeU = getVkTextureAddressMode(currentDesc->texSamplerWord0.CLAMP_X());
   samplerDesc.addressModeV = getVkTextureAddressMode(currentDesc->texSamplerWord0.CLAMP_Y());
   samplerDesc.addressModeW = getVkTextureAddressMode(currentDesc->texSamplerWord0.CLAMP_Z());
   samplerDesc.mipLodBias = currentDesc->texSamplerWord1.LOD_BIAS();
   samplerDesc.anisotropyEnable = getVkAnisotropyEnabled(currentDesc->texSamplerWord0.MAX_ANISO_RATIO());
   samplerDesc.maxAnisotropy = getVkMaxAnisotropy(currentDesc->texSamplerWord0.MAX_ANISO_RATIO());
   samplerDesc.compareEnable = getVkCompareOpEnabled(currentDesc->texSamplerWord0.DEPTH_COMPARE_FUNCTION());
   samplerDesc.compareOp = getVkCompareOp(currentDesc->texSamplerWord0.DEPTH_COMPARE_FUNCTION());
   samplerDesc.minLod = currentDesc->texSamplerWord1.MIN_LOD();
   samplerDesc.maxLod = currentDesc->texSamplerWord1.MAX_LOD();
   samplerDesc.borderColor = getVkBorderColor(currentDesc->texSamplerWord0.BORDER_COLOR_TYPE());
   samplerDesc.unnormalizedCoordinates = VK_FALSE;

   auto sampler = mDevice.createSampler(samplerDesc);
   foundSamp->sampler = sampler;

   mCurrentSamplers[int(shaderStage)][samplerIdx] = foundSamp;
}

bool
Driver::checkCurrentSamplers()
{
   for (auto shaderStage = 0; shaderStage < 3; ++shaderStage) {
      for (auto i = 0u; i < latte::MaxSamplers; ++i) {
         checkCurrentSampler(ShaderStage(shaderStage), i);
      }
   }

   return true;
}

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
