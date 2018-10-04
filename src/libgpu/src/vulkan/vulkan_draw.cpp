#ifdef DECAF_VULKAN
#include "vulkan_driver.h"

#include <common/log.h>

namespace vulkan
{

bool
Driver::bindShaderResources()
{
   auto buildDescriptorSet = [&](vk::DescriptorSet dSet, int shaderStage)
   {
      bool dSetHasValues = false;

      for (auto i = 0u; i < latte::MaxSamplers; ++i) {
         auto &sampler = mCurrentSamplers[shaderStage][i];
         if (!sampler) {
            continue;
         }

         vk::DescriptorImageInfo imageDesc;
         imageDesc.sampler = sampler->sampler;

         vk::WriteDescriptorSet writeDesc;
         writeDesc.dstSet = dSet;
         writeDesc.dstBinding = i;
         writeDesc.dstArrayElement = 0;
         writeDesc.descriptorCount = 1;
         writeDesc.descriptorType = vk::DescriptorType::eSampler;
         writeDesc.pImageInfo = &imageDesc;
         writeDesc.pBufferInfo = nullptr;
         writeDesc.pTexelBufferView = nullptr;
         mDevice.updateDescriptorSets({ writeDesc }, {});
         dSetHasValues = true;
      }

      for (auto i = 0u; i < latte::MaxTextures; ++i) {
         auto &texture = mCurrentTextures[shaderStage][i];
         if (!texture) {
            continue;
         }

         vk::DescriptorImageInfo imageDesc;
         imageDesc.imageView = texture->imageView;
         imageDesc.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

         vk::WriteDescriptorSet writeDesc;
         writeDesc.dstSet = dSet;
         writeDesc.dstBinding = latte::MaxSamplers + i;
         writeDesc.dstArrayElement = 0;
         writeDesc.descriptorCount = 1;
         writeDesc.descriptorType = vk::DescriptorType::eSampledImage;
         writeDesc.pImageInfo = &imageDesc;
         writeDesc.pBufferInfo = nullptr;
         writeDesc.pTexelBufferView = nullptr;
         mDevice.updateDescriptorSets({ writeDesc }, {});
         dSetHasValues = true;
      }

      if (mCurrentGprBuffers[shaderStage]) {
         auto gprBuffer = mCurrentGprBuffers[shaderStage];

         vk::DescriptorBufferInfo bufferDesc;
         bufferDesc.buffer = gprBuffer->buffer;
         bufferDesc.offset = 0;
         bufferDesc.range = gprBuffer->size;

         vk::WriteDescriptorSet writeDesc;
         writeDesc.dstSet = dSet;
         writeDesc.dstBinding = latte::MaxSamplers + latte::MaxTextures + 0;
         writeDesc.dstArrayElement = 0;
         writeDesc.descriptorCount = 1;
         writeDesc.descriptorType = vk::DescriptorType::eUniformBuffer;
         writeDesc.pImageInfo = nullptr;
         writeDesc.pBufferInfo = &bufferDesc;
         writeDesc.pTexelBufferView = nullptr;
         mDevice.updateDescriptorSets({ writeDesc }, {});
         dSetHasValues = true;
      } else {
         for (auto i = 0u; i < latte::MaxUniformBlocks; ++i) {
            auto& uniformBuffer = mCurrentUniformBlocks[shaderStage][i];
            if (!uniformBuffer) {
               continue;
            }

            vk::DescriptorBufferInfo bufferDesc;
            bufferDesc.buffer = uniformBuffer->buffer;
            bufferDesc.offset = 0;
            bufferDesc.range = uniformBuffer->size;

            vk::WriteDescriptorSet writeDesc;
            writeDesc.dstSet = dSet;
            writeDesc.dstBinding = latte::MaxSamplers + latte::MaxTextures + i;
            writeDesc.dstArrayElement = 0;
            writeDesc.descriptorCount = 1;
            writeDesc.descriptorType = vk::DescriptorType::eUniformBuffer;
            writeDesc.pImageInfo = nullptr;
            writeDesc.pBufferInfo = &bufferDesc;
            writeDesc.pTexelBufferView = nullptr;
            mDevice.updateDescriptorSets({ writeDesc }, {});
            dSetHasValues = true;
         }
      }

      return dSetHasValues;
   };

   std::array<vk::DescriptorSet, 3> descBinds;

   // TODO: We should avoid allocating when there is nothing to upload

   if (mCurrentVertexShader) {
      auto vsDSet = allocateVertexDescriptorSet();
      if (buildDescriptorSet(vsDSet, 0)) {
         descBinds[0] = vsDSet;
      }
   }

   if (mCurrentGeometryShader) {
      auto gsDSet = allocateGeometryDescriptorSet();
      if (buildDescriptorSet(gsDSet, 1)) {
         descBinds[1] = gsDSet;
      }
   }

   if (mCurrentPixelShader) {
      auto psDSet = allocatePixelDescriptorSet();
      if (buildDescriptorSet(psDSet, 2)) {
         descBinds[2] = psDSet;
      }
   }

   for (auto i = 0; i < 3; ++i) {
      if (!descBinds[i]) {
         continue;
      }

      mActiveCommandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, mPipelineLayout, i, { descBinds[i] }, {});
   }

   // This should probably be split to its own function
   if (mCurrentVertexShader) {
      auto pa_cl_clip_cntl = getRegister<latte::PA_CL_CLIP_CNTL>(latte::Register::PA_CL_CLIP_CNTL);

      struct VsPushConstants
      {
         struct Vec4
         {
            float x;
            float y;
            float z;
            float w;
         };

         Vec4 posMulAdd;
         Vec4 zSpaceMul;
      } vsConstData;

      if (!mCurrentDrawDesc.isScreenSpace) {
         vsConstData.posMulAdd.x = 1.0f;
         vsConstData.posMulAdd.y = 1.0f;
         vsConstData.posMulAdd.z = 0.0f;
         vsConstData.posMulAdd.w = 0.0f;
      } else {
         auto screenSizeX = mCurrentViewport.width - mCurrentViewport.x;
         auto screenSizeY = mCurrentViewport.height - mCurrentViewport.y;
         vsConstData.posMulAdd.x = 2.0f / screenSizeX;
         vsConstData.posMulAdd.y = 2.0f / screenSizeY;
         vsConstData.posMulAdd.z = -1.0f;
         vsConstData.posMulAdd.w = -1.0f;
      }

      if (!pa_cl_clip_cntl.DX_CLIP_SPACE_DEF()) {
         // map gl(-1 to 1) onto vk(0 to 1)
         vsConstData.zSpaceMul.x = 1.0f; // Add W
         vsConstData.zSpaceMul.y = 0.5f; // * 0.5
      } else {
         // maintain 0 to 1
         vsConstData.zSpaceMul.x = 0.0f; // Add 0
         vsConstData.zSpaceMul.y = 1.0f; // * 1.0
      }

      mActiveCommandBuffer.pushConstants<VsPushConstants>(mPipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, { vsConstData });
   }

   if (mCurrentPixelShader) {
      // TODO: We should probably move this into a getXXXXDesc-like function
      // so that our register reads are slightly more consistent.
      auto sx_alpha_test_control = getRegister<latte::SX_ALPHA_TEST_CONTROL>(latte::Register::SX_ALPHA_TEST_CONTROL);
      auto sx_alpha_ref = getRegister<latte::SX_ALPHA_REF>(latte::Register::SX_ALPHA_REF);

      auto alphaFunc = sx_alpha_test_control.ALPHA_FUNC();
      if (!sx_alpha_test_control.ALPHA_TEST_ENABLE()) {
         alphaFunc = latte::REF_FUNC::ALWAYS;
      }
      if (sx_alpha_test_control.ALPHA_TEST_BYPASS()) {
         alphaFunc = latte::REF_FUNC::ALWAYS;
      }
      auto alphaRef = sx_alpha_ref.ALPHA_REF();

      struct PsPushConstants
      {
         uint32_t alphaOp;
         float alphaRef;
      } psConstData;
      psConstData.alphaOp = alphaFunc;
      psConstData.alphaRef = alphaRef;
      mActiveCommandBuffer.pushConstants<PsPushConstants>(mPipelineLayout, vk::ShaderStageFlagBits::eFragment, 32, { psConstData });
   }

   return true;
}

void
Driver::drawGenericIndexed(uint32_t numIndices, void *indices)
{
   // First lets set up our draw description for everyone
   auto vgt_dma_index_type = getRegister<latte::VGT_DMA_INDEX_TYPE>(latte::Register::VGT_DMA_INDEX_TYPE);
   auto vgt_primitive_type = getRegister<latte::VGT_PRIMITIVE_TYPE>(latte::Register::VGT_PRIMITIVE_TYPE);
   auto vgt_dma_num_instances = getRegister<latte::VGT_DMA_NUM_INSTANCES>(latte::Register::VGT_DMA_NUM_INSTANCES);
   auto sq_vtx_base_vtx_loc = getRegister<latte::SQ_VTX_BASE_VTX_LOC>(latte::Register::SQ_VTX_BASE_VTX_LOC);
   auto sq_vtx_start_inst_loc = getRegister<latte::SQ_VTX_START_INST_LOC>(latte::Register::SQ_VTX_START_INST_LOC);

   auto& drawDesc = mCurrentDrawDesc;
   drawDesc.indices = indices;
   drawDesc.indexType = vgt_dma_index_type.INDEX_TYPE();
   drawDesc.indexSwapMode = vgt_dma_index_type.SWAP_MODE();
   drawDesc.primitiveType = vgt_primitive_type.PRIM_TYPE();
   drawDesc.isScreenSpace = false;
   drawDesc.numIndices = numIndices;
   drawDesc.baseVertex = sq_vtx_base_vtx_loc.OFFSET();
   drawDesc.numInstances = vgt_dma_num_instances.NUM_INSTANCES();
   drawDesc.baseInstance = sq_vtx_start_inst_loc.OFFSET();

   if (drawDesc.primitiveType == latte::VGT_DI_PRIMITIVE_TYPE::RECTLIST) {
      drawDesc.isScreenSpace = true;
   }

   // Set up all the required state, ordering here is very important
   checkCurrentShaders();
   checkCurrentRenderPass();
   checkCurrentFramebuffer();
   checkCurrentPipeline();
   checkCurrentSamplers();
   checkCurrentTextures();
   checkCurrentAttribBuffers();
   checkCurrentShaderBuffers();
   checkCurrentIndices();
   checkCurrentViewportAndScissor();

   // TODO: We should probably move our code that does surface transitions to
   // some common location.  We cannot do it in the checkCurrent's, since its
   // possible someone transitions the surfaces after they are active, but
   // before the draw, and we cannot do it in bind as we already started our
   // render pass there...
   for (auto &surface : mCurrentFramebuffer->colorSurfaces) {
      transitionSurface(surface, vk::ImageLayout::eColorAttachmentOptimal);
   }
   if (mCurrentFramebuffer->depthSurface) {
      auto &surface = mCurrentFramebuffer->depthSurface;
      transitionSurface(surface, vk::ImageLayout::eDepthStencilAttachmentOptimal);
   }
   for (auto shaderStage = 0u; shaderStage < 3u; ++shaderStage) {
      for (auto i = 0u; i < latte::MaxTextures; ++i) {
         auto& surface = mCurrentTextures[shaderStage][i];
         if (surface) {
            transitionSurface(surface, vk::ImageLayout::eShaderReadOnlyOptimal);
         }
      }
   }

   auto& fbRa = mCurrentFramebuffer->renderArea;
   auto renderArea = vk::Rect2D { { 0, 0 }, fbRa };

   // Bind and set up everything, and then do our draw
   auto passBeginDesc = vk::RenderPassBeginInfo {};
   passBeginDesc.renderPass = mCurrentRenderPass->renderPass;
   passBeginDesc.framebuffer = mCurrentFramebuffer->framebuffer;
   passBeginDesc.renderArea = renderArea;
   passBeginDesc.clearValueCount = 0;
   passBeginDesc.pClearValues = nullptr;
   mActiveCommandBuffer.beginRenderPass(passBeginDesc, vk::SubpassContents::eInline);

   mActiveCommandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, mCurrentPipeline->pipeline);

   bool drawIsLegal = true;

   drawIsLegal &= bindAttribBuffers();
   drawIsLegal &= bindShaderResources();
   drawIsLegal &= bindViewportAndScissor();

   if (drawIsLegal) {
      if (mCurrentIndexBuffer) {
         bindIndexBuffer();
         mActiveCommandBuffer.drawIndexed(drawDesc.numIndices, drawDesc.numInstances, 0, drawDesc.baseVertex, drawDesc.baseInstance);
      } else {
         mActiveCommandBuffer.draw(drawDesc.numIndices, drawDesc.numInstances, drawDesc.baseVertex, drawDesc.baseInstance);
      }
   } else {
      gLog->debug("Skipped draw due to invalid parameters");
   }

   mActiveCommandBuffer.endRenderPass();
}

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
