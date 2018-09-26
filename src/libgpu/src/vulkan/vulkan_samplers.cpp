#pragma optimize("", off)

#ifdef DECAF_VULKAN

#include "vulkan_driver.h"

namespace vulkan
{

static vk::BorderColor
getVkBorderColor(latte::SQ_TEX_BORDER_COLOR color)
{
   switch (color) {
   case latte::SQ_TEX_BORDER_COLOR::TRANS_BLACK:
      return vk::BorderColor::eFloatTransparentBlack;
   case latte::SQ_TEX_BORDER_COLOR::OPAQUE_BLACK:
      return vk::BorderColor::eFloatOpaqueBlack;
   case latte::SQ_TEX_BORDER_COLOR::OPAQUE_WHITE:
      return vk::BorderColor::eFloatOpaqueWhite;
   case latte::SQ_TEX_BORDER_COLOR::REGISTER:
      decaf_abort("Unsupported register-based texture border color");
   default:
      decaf_abort("Unexpected texture border color type");
   }
}

static vk::Filter
getVkXyTextureFilter(latte::SQ_TEX_XY_FILTER filter)
{
   switch (filter) {
   case latte::SQ_TEX_XY_FILTER::POINT:
      return vk::Filter::eNearest;
   case latte::SQ_TEX_XY_FILTER::BILINEAR:
      return vk::Filter::eLinear;
   case latte::SQ_TEX_XY_FILTER::BICUBIC:
      return vk::Filter::eCubicIMG;
   default:
      decaf_abort("Unexpected texture xy filter mode");
   }
}

static vk::SamplerMipmapMode
getVkZTextureFilter(latte::SQ_TEX_Z_FILTER filter)
{
   switch (filter) {
   case latte::SQ_TEX_Z_FILTER::NONE:
      return vk::SamplerMipmapMode::eNearest;
   case latte::SQ_TEX_Z_FILTER::POINT:
      return vk::SamplerMipmapMode::eNearest;
   case latte::SQ_TEX_Z_FILTER::LINEAR:
      return vk::SamplerMipmapMode::eLinear;
   default:
      decaf_abort("Unexpected texture xy filter mode");
   }
}

static vk::SamplerAddressMode
getVkTextureAddressMode(latte::SQ_TEX_CLAMP clamp)
{
   switch (clamp) {
   case latte::SQ_TEX_CLAMP::WRAP:
      return vk::SamplerAddressMode::eRepeat;
   case latte::SQ_TEX_CLAMP::MIRROR:
      return vk::SamplerAddressMode::eMirroredRepeat;
   case latte::SQ_TEX_CLAMP::CLAMP_LAST_TEXEL:
      return vk::SamplerAddressMode::eClampToEdge;
   case latte::SQ_TEX_CLAMP::MIRROR_ONCE_LAST_TEXEL:
      return vk::SamplerAddressMode::eMirrorClampToEdge;
   case latte::SQ_TEX_CLAMP::CLAMP_HALF_BORDER:
      return vk::SamplerAddressMode::eClampToBorder;
   case latte::SQ_TEX_CLAMP::MIRROR_ONCE_HALF_BORDER:
      return vk::SamplerAddressMode::eMirrorClampToEdge;
   case latte::SQ_TEX_CLAMP::CLAMP_BORDER:
      return vk::SamplerAddressMode::eClampToBorder;
   case latte::SQ_TEX_CLAMP::MIRROR_ONCE_BORDER:
      return vk::SamplerAddressMode::eMirrorClampToEdge;
   default:
      decaf_abort("Unexpected texture clamp mode");
   }
}

static bool
getVkAnisotropyEnabled(latte::SQ_TEX_ANISO aniso)
{
   return aniso != latte::SQ_TEX_ANISO::ANISO_1_TO_1;
}

static float
getVkMaxAnisotropy(latte::SQ_TEX_ANISO aniso)
{
   switch (aniso) {
   case latte::SQ_TEX_ANISO::ANISO_1_TO_1:
      return 1.0f;
   case latte::SQ_TEX_ANISO::ANISO_2_TO_1:
      return 2.0f;
   case latte::SQ_TEX_ANISO::ANISO_4_TO_1:
      return 4.0f;
   case latte::SQ_TEX_ANISO::ANISO_8_TO_1:
      return 8.0f;
   case latte::SQ_TEX_ANISO::ANISO_16_TO_1:
      return 16.0f;
   default:
      decaf_abort("Unexpected texture anisotropy mode");
   }
}

static bool
getVkComparisonEnabled(latte::REF_FUNC refFunc)
{
   return refFunc != latte::REF_FUNC::ALWAYS;
}

static vk::CompareOp
getVkComparisonFunc(latte::REF_FUNC refFunc)
{
   switch (refFunc) {
   case latte::REF_FUNC::NEVER:
      return vk::CompareOp::eNever;
   case latte::REF_FUNC::LESS:
      return vk::CompareOp::eLess;
   case latte::REF_FUNC::EQUAL:
      return vk::CompareOp::eEqual;
   case latte::REF_FUNC::LESS_EQUAL:
      return vk::CompareOp::eLessOrEqual;
   case latte::REF_FUNC::GREATER:
      return vk::CompareOp::eGreater;
   case latte::REF_FUNC::NOT_EQUAL:
      return vk::CompareOp::eNotEqual;
   case latte::REF_FUNC::GREATER_EQUAL:
      return vk::CompareOp::eGreaterOrEqual;
   case latte::REF_FUNC::ALWAYS:
      return vk::CompareOp::eAlways;
   default:
      decaf_abort("Unexpected texture comparison mode");
   }
}

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

   auto& foundSamp = mSamplers[currentDesc.hash()];
   if (foundSamp) {
      mCurrentSamplers[int(shaderStage)][samplerIdx] = foundSamp;
      return;
   }

   foundSamp = new SamplerObject();
   foundSamp->desc = currentDesc;

   
   /*
   currentDesc->texSamplerWord0;
      BITFIELD_ENTRY(15, 2, SQ_TEX_Z_FILTER, Z_FILTER)
      BITFIELD_ENTRY(24, 1, bool, POINT_SAMPLING_CLAMP)
      BITFIELD_ENTRY(25, 1, bool, TEX_ARRAY_OVERRIDE)
      BITFIELD_ENTRY(29, 2, SQ_TEX_CHROMA_KEY, CHROMA_KEY)
      BITFIELD_ENTRY(31, 1, bool, LOD_USES_MINOR_AXIS)

   currentDesc->texSamplerWord2;
      BITFIELD_ENTRY(0, 12, uint32_t, LOD_BIAS_SEC)
      BITFIELD_ENTRY(12, 1, bool, MC_COORD_TRUNCATE)
      BITFIELD_ENTRY(13, 1, bool, FORCE_DEGAMMA)
      BITFIELD_ENTRY(14, 1, bool, HIGH_PRECISION_FILTER)
      BITFIELD_ENTRY(15, 3, uint32_t, PERF_MIP)
      BITFIELD_ENTRY(18, 2, uint32_t, PERF_Z)
      BITFIELD_ENTRY(20, 6, ufixed_1_5_t, ANISO_BIAS)
      BITFIELD_ENTRY(26, 1, bool, FETCH_4)
      BITFIELD_ENTRY(27, 1, bool, SAMPLE_IS_PCF)
      BITFIELD_ENTRY(28, 1, SQ_TEX_ROUNDING_MODE, TRUNCATE_COORD)
      BITFIELD_ENTRY(29, 1, bool, DISABLE_CUBE_WRAP)
      BITFIELD_ENTRY(31, 1, bool, TYPE)
   */

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
   samplerDesc.compareEnable = getVkComparisonEnabled(currentDesc->texSamplerWord0.DEPTH_COMPARE_FUNCTION());
   samplerDesc.compareOp = getVkComparisonFunc(currentDesc->texSamplerWord0.DEPTH_COMPARE_FUNCTION());
   samplerDesc.minLod = currentDesc->texSamplerWord1.MIN_LOD();
   samplerDesc.maxLod = currentDesc->texSamplerWord1.MAX_LOD();
   samplerDesc.borderColor = getVkBorderColor(currentDesc->texSamplerWord0.BORDER_COLOR_TYPE());
   samplerDesc.unnormalizedCoordinates = VK_FALSE;

   auto sampler = mDevice.createSampler(samplerDesc);
   foundSamp->sampler = sampler;

   mCurrentSamplers[int(shaderStage)][samplerIdx] = foundSamp;
}

void
Driver::checkCurrentSamplers()
{
   for (auto shaderStage = 0; shaderStage < 3; ++shaderStage) {
      for (auto i = 0u; i < latte::MaxSamplers; ++i) {
         checkCurrentSampler(ShaderStage(shaderStage), i);
      }
   }
}

} // namespace vulkan

#endif // DECAF_VULKAN
