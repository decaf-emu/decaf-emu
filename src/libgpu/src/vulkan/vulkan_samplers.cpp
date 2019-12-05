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
Driver::updateDrawSampler(ShaderStage shaderStage, uint32_t samplerIdx)
{
   uint32_t shaderStageIdx = static_cast<uint32_t>(shaderStage);
   auto &drawSampler = mCurrentDraw->samplers[shaderStageIdx][samplerIdx];

   HashedDesc<SamplerDesc> currentDesc = getSamplerDesc(shaderStage, samplerIdx);

   if (drawSampler && drawSampler->desc == currentDesc) {
      // We already have the correct sampler set
      return;
   }

   auto &foundSamp = mSamplers[currentDesc.hash()];
   if (foundSamp) {
      drawSampler = foundSamp;
      return;
   }

   vk::SamplerCreateInfo samplerDesc;
   samplerDesc.magFilter = getVkXyTextureFilter(currentDesc->texSamplerWord0.XY_MAG_FILTER());
   samplerDesc.minFilter = getVkXyTextureFilter(currentDesc->texSamplerWord0.XY_MIN_FILTER());
   samplerDesc.mipmapMode = getVkZTextureFilter(currentDesc->texSamplerWord0.MIP_FILTER());
   samplerDesc.addressModeU = getVkTextureAddressMode(currentDesc->texSamplerWord0.CLAMP_X());
   samplerDesc.addressModeV = getVkTextureAddressMode(currentDesc->texSamplerWord0.CLAMP_Y());
   samplerDesc.addressModeW = getVkTextureAddressMode(currentDesc->texSamplerWord0.CLAMP_Z());
   samplerDesc.mipLodBias = static_cast<float>(currentDesc->texSamplerWord1.LOD_BIAS());
   samplerDesc.anisotropyEnable = getVkAnisotropyEnabled(currentDesc->texSamplerWord0.MAX_ANISO_RATIO());
   samplerDesc.maxAnisotropy = getVkMaxAnisotropy(currentDesc->texSamplerWord0.MAX_ANISO_RATIO());
   samplerDesc.compareEnable = getVkCompareOpEnabled(currentDesc->texSamplerWord0.DEPTH_COMPARE_FUNCTION());
   samplerDesc.compareOp = getVkCompareOp(currentDesc->texSamplerWord0.DEPTH_COMPARE_FUNCTION());
   samplerDesc.minLod = static_cast<float>(currentDesc->texSamplerWord1.MIN_LOD());
   samplerDesc.maxLod = static_cast<float>(currentDesc->texSamplerWord1.MAX_LOD());
   samplerDesc.borderColor = getVkBorderColor(currentDesc->texSamplerWord0.BORDER_COLOR_TYPE());
   samplerDesc.unnormalizedCoordinates = VK_FALSE;
   auto sampler = mDevice.createSampler(samplerDesc);

   foundSamp = new SamplerObject();
   foundSamp->desc = currentDesc;
   foundSamp->sampler = sampler;

   drawSampler = foundSamp;
}

bool
Driver::checkCurrentSamplers()
{
   if (mCurrentDraw->vertexShader) {
      for (auto samplerIdx = 0u; samplerIdx < latte::MaxSamplers; ++samplerIdx) {
         if (mCurrentDraw->vertexShader->shader.meta.samplerUsed[samplerIdx]) {
            updateDrawSampler(ShaderStage::Vertex, samplerIdx);
         } else {
            mCurrentDraw->samplers[0][samplerIdx] = nullptr;
         }
      }
   } else {
      for (auto samplerIdx = 0; samplerIdx < latte::MaxSamplers; ++samplerIdx) {
         mCurrentDraw->samplers[0][samplerIdx] = nullptr;
      }
   }

   if (mCurrentDraw->geometryShader) {
      for (auto samplerIdx = 0u; samplerIdx < latte::MaxSamplers; ++samplerIdx) {
         if (mCurrentDraw->geometryShader->shader.meta.samplerUsed[samplerIdx]) {
            updateDrawSampler(ShaderStage::Geometry, samplerIdx);
         } else {
            mCurrentDraw->samplers[1][samplerIdx] = nullptr;
         }
      }
   } else {
      for (auto samplerIdx = 0; samplerIdx < latte::MaxSamplers; ++samplerIdx) {
         mCurrentDraw->samplers[1][samplerIdx] = nullptr;
      }
   }

   if (mCurrentDraw->pixelShader) {
      for (auto samplerIdx = 0u; samplerIdx < latte::MaxSamplers; ++samplerIdx) {
         if (mCurrentDraw->pixelShader->shader.meta.samplerUsed[samplerIdx]) {
            updateDrawSampler(ShaderStage::Pixel, samplerIdx);
         } else {
            mCurrentDraw->samplers[2][samplerIdx] = nullptr;
         }
      }
   } else {
      for (auto samplerIdx = 0; samplerIdx < latte::MaxSamplers; ++samplerIdx) {
         mCurrentDraw->samplers[2][samplerIdx] = nullptr;
      }
   }

   return true;
}

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
