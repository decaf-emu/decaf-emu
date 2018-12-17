#ifdef DECAF_VULKAN
#include "vulkan_driver.h"

namespace vulkan
{

SurfaceViewDesc
Driver::getTextureDesc(ShaderStage shaderStage, uint32_t textureIdx)
{
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

   return surfaceDesc;
}

void
Driver::updateDrawTexture(ShaderStage shaderStage, uint32_t textureIdx)
{
   uint32_t shaderStageIdx = static_cast<uint32_t>(shaderStage);
   auto &drawTexture = mCurrentDraw->textures[shaderStageIdx][textureIdx];

   HashedDesc<SurfaceViewDesc> currentDesc = getTextureDesc(shaderStage, textureIdx);

   if (!currentDesc->surfaceDesc.baseAddress) {
      drawTexture = nullptr;
      return;
   }

   if (drawTexture && drawTexture->desc == currentDesc) {
      // We already have the correct texture set
      return;
   }

   drawTexture = getSurfaceView(*currentDesc);
   mCurrentDraw->textureDirty[shaderStageIdx][textureIdx] = true;
}

bool
Driver::checkCurrentTextures()
{
   if (mCurrentDraw->vertexShader) {
      for (auto textureIdx = 0u; textureIdx < latte::MaxTextures; ++textureIdx) {
         if (mCurrentDraw->vertexShader->shader.meta.textureUsed[textureIdx]) {
            updateDrawTexture(ShaderStage::Vertex, textureIdx);
         } else {
            mCurrentDraw->textures[0][textureIdx] = nullptr;
         }
      }
   } else {
      for (auto textureIdx = 0; textureIdx < latte::MaxTextures; ++textureIdx) {
         mCurrentDraw->textures[0][textureIdx] = nullptr;
      }
   }

   if (mCurrentDraw->geometryShader) {
      for (auto textureIdx = 0u; textureIdx < latte::MaxTextures; ++textureIdx) {
         if (mCurrentDraw->geometryShader->shader.meta.textureUsed[textureIdx]) {
            updateDrawTexture(ShaderStage::Geometry, textureIdx);
         } else {
            mCurrentDraw->textures[1][textureIdx] = nullptr;
         }
      }
   } else {
      for (auto textureIdx = 0; textureIdx < latte::MaxTextures; ++textureIdx) {
         mCurrentDraw->textures[1][textureIdx] = nullptr;
      }
   }

   if (mCurrentDraw->pixelShader) {
      for (auto textureIdx = 0u; textureIdx < latte::MaxTextures; ++textureIdx) {
         if (mCurrentDraw->pixelShader->shader.meta.textureUsed[textureIdx]) {
            updateDrawTexture(ShaderStage::Pixel, textureIdx);
         } else {
            mCurrentDraw->textures[2][textureIdx] = nullptr;
         }
      }
   } else {
      for (auto textureIdx = 0; textureIdx < latte::MaxTextures; ++textureIdx) {
         mCurrentDraw->textures[2][textureIdx] = nullptr;
      }
   }

   return true;
}

void
Driver::prepareCurrentTextures()
{
   for (auto i = 0u; i < latte::MaxTextures; ++i) {
      auto& vsSurface = mCurrentDraw->textures[0][i];
      if (vsSurface) {
         if (!mCurrentDraw->textureDirty[0][i]) {
            transitionSurfaceView(vsSurface, ResourceUsage::VertexTexture, vk::ImageLayout::eShaderReadOnlyOptimal, true);
         } else {
            transitionSurfaceView(vsSurface, ResourceUsage::VertexTexture, vk::ImageLayout::eShaderReadOnlyOptimal);
            mCurrentDraw->textureDirty[0][i] = false;
         }
      }

      auto& gsSurface = mCurrentDraw->textures[1][i];
      if (gsSurface) {
         if (!mCurrentDraw->textureDirty[1][i]) {
            transitionSurfaceView(gsSurface, ResourceUsage::GeometryTexture, vk::ImageLayout::eShaderReadOnlyOptimal, true);
         } else {
            transitionSurfaceView(gsSurface, ResourceUsage::GeometryTexture, vk::ImageLayout::eShaderReadOnlyOptimal);
            mCurrentDraw->textureDirty[1][i] = false;
         }
      }

      auto& psSurface = mCurrentDraw->textures[2][i];
      if (psSurface) {
         if (!mCurrentDraw->textureDirty[2][i]) {
            transitionSurfaceView(psSurface, ResourceUsage::PixelTexture, vk::ImageLayout::eShaderReadOnlyOptimal, true);
         } else {
            transitionSurfaceView(psSurface, ResourceUsage::PixelTexture, vk::ImageLayout::eShaderReadOnlyOptimal);
            mCurrentDraw->textureDirty[2][i] = false;
         }
      }
   }
}

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
