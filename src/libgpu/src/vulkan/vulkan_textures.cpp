#ifdef DECAF_VULKAN
#include "vulkan_driver.h"

namespace vulkan
{

TextureDesc
Driver::getTextureDesc(ShaderStage shaderStage, uint32_t textureIdx)
{
   TextureDesc desc;

   uint32_t samplerBaseIdx;
   if (shaderStage == ShaderStage::Vertex) {
      samplerBaseIdx = latte::SQ_RES_OFFSET::VS_TEX_RESOURCE_0;
   } else if (shaderStage == ShaderStage::Geometry) {
      samplerBaseIdx = latte::SQ_RES_OFFSET::GS_TEX_RESOURCE_0;
   } else if (shaderStage == ShaderStage::Pixel) {
      samplerBaseIdx = latte::SQ_RES_OFFSET::PS_TEX_RESOURCE_0;
   } else {
      decaf_abort("Unknown shader stage");
   }

   auto resourceOffset = (samplerBaseIdx + textureIdx) * 7;
   auto sq_tex_resource_word0 = getRegister<latte::SQ_TEX_RESOURCE_WORD0_N>(latte::Register::SQ_RESOURCE_WORD0_0 + 4 * resourceOffset);
   auto sq_tex_resource_word1 = getRegister<latte::SQ_TEX_RESOURCE_WORD1_N>(latte::Register::SQ_RESOURCE_WORD1_0 + 4 * resourceOffset);
   auto sq_tex_resource_word2 = getRegister<latte::SQ_TEX_RESOURCE_WORD2_N>(latte::Register::SQ_RESOURCE_WORD2_0 + 4 * resourceOffset);
   auto sq_tex_resource_word3 = getRegister<latte::SQ_TEX_RESOURCE_WORD3_N>(latte::Register::SQ_RESOURCE_WORD3_0 + 4 * resourceOffset);
   auto sq_tex_resource_word4 = getRegister<latte::SQ_TEX_RESOURCE_WORD4_N>(latte::Register::SQ_RESOURCE_WORD4_0 + 4 * resourceOffset);
   auto sq_tex_resource_word5 = getRegister<latte::SQ_TEX_RESOURCE_WORD5_N>(latte::Register::SQ_RESOURCE_WORD5_0 + 4 * resourceOffset);
   auto sq_tex_resource_word6 = getRegister<latte::SQ_TEX_RESOURCE_WORD6_N>(latte::Register::SQ_RESOURCE_WORD6_0 + 4 * resourceOffset);

   SurfaceDesc surfaceDataDesc;
   surfaceDataDesc.baseAddress = sq_tex_resource_word2.BASE_ADDRESS() << 8;
   surfaceDataDesc.pitch = (sq_tex_resource_word0.PITCH() + 1) * 8;
   surfaceDataDesc.width = sq_tex_resource_word0.TEX_WIDTH() + 1;
   surfaceDataDesc.height = sq_tex_resource_word1.TEX_HEIGHT() + 1;
   surfaceDataDesc.depth = sq_tex_resource_word1.TEX_DEPTH() + 1;
   surfaceDataDesc.samples = 1u;
   surfaceDataDesc.dim = sq_tex_resource_word0.DIM();
   surfaceDataDesc.format = latte::getSurfaceFormat(
      sq_tex_resource_word1.DATA_FORMAT(),
      sq_tex_resource_word4.NUM_FORMAT_ALL(),
      sq_tex_resource_word4.FORMAT_COMP_X(),
      sq_tex_resource_word4.FORCE_DEGAMMA());
   surfaceDataDesc.tileType = sq_tex_resource_word0.TILE_TYPE();
   surfaceDataDesc.tileMode = sq_tex_resource_word0.TILE_MODE();

   if (surfaceDataDesc.dim == latte::SQ_TEX_DIM::DIM_CUBEMAP) {
      surfaceDataDesc.depth *= 6;
   }

   SurfaceViewDesc surfaceDesc;
   surfaceDesc.surfaceDesc = surfaceDataDesc;
   surfaceDesc.sliceStart = sq_tex_resource_word5.BASE_ARRAY();
   surfaceDesc.sliceEnd = sq_tex_resource_word5.LAST_ARRAY() + 1;
   surfaceDesc.channels[0] = sq_tex_resource_word4.DST_SEL_X();
   surfaceDesc.channels[1] = sq_tex_resource_word4.DST_SEL_Y();
   surfaceDesc.channels[2] = sq_tex_resource_word4.DST_SEL_Z();
   surfaceDesc.channels[3] = sq_tex_resource_word4.DST_SEL_W();

   desc.surfaceDesc = surfaceDesc;

   return desc;
}

bool
Driver::checkCurrentTexture(ShaderStage shaderStage, uint32_t textureIdx)
{
   if (shaderStage == ShaderStage::Vertex) {
      if (!mCurrentVertexShader || !mCurrentVertexShader->shader.textureUsed[textureIdx]) {
         return true;
      }
   } else if (shaderStage == ShaderStage::Geometry) {
      if (!mCurrentGeometryShader || !mCurrentGeometryShader->shader.textureUsed[textureIdx]) {
         return true;
      }
   } else if (shaderStage == ShaderStage::Pixel) {
      if (!mCurrentPixelShader || !mCurrentPixelShader->shader.textureUsed[textureIdx]) {
         return true;
      }
   } else {
      decaf_abort("Unknown shader stage");
   }

   auto textureDesc = getTextureDesc(shaderStage, textureIdx);
   if (!textureDesc.surfaceDesc.surfaceDesc.baseAddress) {
      return false;
   }

   auto surface = getSurfaceView(textureDesc.surfaceDesc);
   mCurrentTextures[int(shaderStage)][textureIdx] = surface;
   return true;
}

bool
Driver::checkCurrentTextures()
{
   mCurrentTextures = { { nullptr } };

   for (auto shaderStage = 0; shaderStage < 3; ++shaderStage) {
      for (auto i = 0u; i < latte::MaxTextures; ++i) {
         if (!checkCurrentTexture(ShaderStage(shaderStage), i)) {
            return false;
         }
      }
   }

   return true;
}

void
Driver::prepareCurrentTextures()
{
   for (auto shaderStage = 0u; shaderStage < 3u; ++shaderStage) {
      for (auto i = 0u; i < latte::MaxTextures; ++i) {
         auto& surface = mCurrentTextures[shaderStage][i];
         if (surface) {
            transitionSurfaceView(surface, ResourceUsage::Texture, vk::ImageLayout::eShaderReadOnlyOptimal);
         }
      }
   }
}

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
