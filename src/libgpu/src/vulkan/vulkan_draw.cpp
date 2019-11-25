#ifdef DECAF_VULKAN
#include "vulkan_driver.h"

#include <common/log.h>

namespace vulkan
{

void
Driver::bindDescriptors()
{
   bool dSetHasValues = false;

   std::array<std::array<vk::DescriptorImageInfo, latte::MaxTextures>, 3> texSampInfos;
   std::array<std::array<vk::DescriptorBufferInfo, latte::MaxUniformBlocks>, 3 > bufferInfos;

   for (auto shaderStage = 0u; shaderStage < 3u; ++shaderStage) {
      auto shaderStageTyped = static_cast<ShaderStage>(shaderStage);

      const spirv::ShaderMeta *shaderMeta;
      if (shaderStageTyped == ShaderStage::Vertex) {
         if (!mCurrentDraw->vertexShader) {
            continue;
         }

         shaderMeta = &mCurrentDraw->vertexShader->shader.meta;
      } else if (shaderStageTyped == ShaderStage::Geometry) {
         if (!mCurrentDraw->geometryShader) {
            continue;
         }

         shaderMeta = &mCurrentDraw->geometryShader->shader.meta;
      } else if (shaderStageTyped == ShaderStage::Pixel) {
         if (!mCurrentDraw->pixelShader) {
            continue;
         }

         shaderMeta = &mCurrentDraw->pixelShader->shader.meta;
      } else {
         decaf_abort("Unexpected shader stage during descriptor build");
      }

      for (auto i = 0u; i < latte::MaxSamplers; ++i) {
         if (shaderMeta->samplerUsed[i]) {
            auto &sampler = mCurrentDraw->samplers[shaderStage][i];
            if (sampler) {
               texSampInfos[shaderStage][i].sampler = sampler->sampler;
            } else {
               texSampInfos[shaderStage][i].sampler = mBlankSampler;
            }

            dSetHasValues = true;
         }
      }

      for (auto i = 0u; i < latte::MaxTextures; ++i) {
         if (shaderMeta->textureUsed[i]) {
            auto &texture = mCurrentDraw->textures[shaderStage][i];
            if (texture) {
               texSampInfos[shaderStage][i].imageView = texture->imageView;
            } else {
               texSampInfos[shaderStage][i].imageView = mBlankImageView;
            }

            texSampInfos[shaderStage][i].imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

            dSetHasValues = true;
         }
      }

      if (mCurrentDraw->gprBuffers[shaderStage]) {
         if (shaderMeta->cfileUsed) {
            auto gprBuffer = mCurrentDraw->gprBuffers[shaderStage];

            bufferInfos[shaderStage][0].buffer = gprBuffer->buffer;
            bufferInfos[shaderStage][0].offset = 0;
            bufferInfos[shaderStage][0].range = gprBuffer->size;

            dSetHasValues = true;
         }
      } else {
         for (auto i = 0u; i < latte::MaxUniformBlocks; ++i) {
            if (shaderMeta->cbufferUsed[i]) {
               auto& uniformBuffer = mCurrentDraw->uniformBlocks[shaderStage][i];
               if (uniformBuffer) {
                  bufferInfos[shaderStage][i].buffer = uniformBuffer->buffer;
                  bufferInfos[shaderStage][i].offset = 0;
                  bufferInfos[shaderStage][i].range = uniformBuffer->size;
               } else {
                  bufferInfos[shaderStage][i].buffer = mBlankBuffer;
                  bufferInfos[shaderStage][i].offset = 0;
                  bufferInfos[shaderStage][i].range = 1024;
               }

               dSetHasValues = true;
            }
         }
      }
   }

   // If this shader stage has nothing bound, there is no need to
   // actually generate our descriptor sets or anything.
   if (!dSetHasValues) {
      return;
   }

   /*
   for (auto shaderStage = 0u; shaderStage < 3u; ++shaderStage) {
      for (auto &samplerInfo : samplerInfos[shaderStage]) {
         if (!samplerInfo.sampler) {
            samplerInfo.sampler = mBlankSampler;
         }
      }
      for (auto &textureInfo : textureInfos[shaderStage]) {
         if (!textureInfo.imageView) {
            textureInfo.imageView = mBlankImageView;
            textureInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
         }
      }
      for (auto &bufferInfo : bufferInfos[shaderStage]) {
         if (!bufferInfo.buffer) {
            bufferInfo.buffer = mBlankBuffer;
            bufferInfo.offset = 0;
            bufferInfo.range = 1;
         }
      }
   }
   */

   vk::DescriptorSet dSet;
   if (!mCurrentDraw->pipeline->pipelineLayout) {
      // If there is no custom pipeline layout configured, this means we have to use
      // standard descriptor sets rather than being able to take advantage of push.

      dSet = allocateGenericDescriptorSet();
   }

   mScratchDescriptorWrites.clear();
   auto &descWrites = mScratchDescriptorWrites;

   for (auto shaderStage = 0u; shaderStage < 3u; ++shaderStage) {
      auto bindingBase = 32 * shaderStage;

      for (auto i = 0u; i < latte::MaxTextures; ++i) {
         if (!texSampInfos[shaderStage][i].sampler && !texSampInfos[shaderStage][i].imageView) {
            continue;
         }

         descWrites.push_back({});
         vk::WriteDescriptorSet& writeDesc = descWrites.back();
         writeDesc.dstSet = dSet;
         writeDesc.dstBinding = bindingBase + i;
         writeDesc.dstArrayElement = 0;
         writeDesc.descriptorCount = 1;
         if (texSampInfos[shaderStage][i].sampler && texSampInfos[shaderStage][i].imageView) {
            writeDesc.descriptorType = vk::DescriptorType::eCombinedImageSampler;
         } else if (texSampInfos[shaderStage][i].sampler) {
            writeDesc.descriptorType = vk::DescriptorType::eSampler;
         } else if (texSampInfos[shaderStage][i].imageView) {
            writeDesc.descriptorType = vk::DescriptorType::eSampledImage;
         }
         writeDesc.pImageInfo = &texSampInfos[shaderStage][i];
         writeDesc.pBufferInfo = nullptr;
         writeDesc.pTexelBufferView = nullptr;
      }

      bindingBase += latte::MaxTextures;

      for (auto i = 0u; i < latte::MaxUniformBlocks; ++i) {
         if (i >= 15) {
            // Refer to the descriptor layout creation code for information
            // on why this code is neccessary...
            break;
         }

         if (!bufferInfos[shaderStage][i].buffer) {
            continue;
         }

         descWrites.push_back({});
         vk::WriteDescriptorSet& writeDesc = descWrites.back();
         writeDesc.dstSet = dSet;
         writeDesc.dstBinding = bindingBase + i;
         writeDesc.dstArrayElement = 0;
         writeDesc.descriptorCount = 1;
         writeDesc.descriptorType = vk::DescriptorType::eStorageBuffer;
         writeDesc.pImageInfo = nullptr;
         writeDesc.pBufferInfo = &bufferInfos[shaderStage][i];
         writeDesc.pTexelBufferView = nullptr;
      }
   }

   if (!dSet) {
      mActiveCommandBuffer.pushDescriptorSetKHR(vk::PipelineBindPoint::eGraphics,
                                                mCurrentDraw->pipeline->pipelineLayout->pipelineLayout,
                                                0,
                                                descWrites,
                                                mVkDynLoader);
   } else {
      mDevice.updateDescriptorSets(descWrites, {});

      mActiveCommandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                              mPipelineLayout,
                                              0,
                                              { dSet }, {});
   }
}

void
Driver::bindShaderParams()
{

   // This should probably be split to its own function
   if (mCurrentDraw->vertexShader) {
      spirv::VertexPushConstants vsConstData;
      vsConstData.posMulAdd.x = mCurrentDraw->shaderViewportData.xMul;
      vsConstData.posMulAdd.y = mCurrentDraw->shaderViewportData.yMul;
      vsConstData.posMulAdd.z = mCurrentDraw->shaderViewportData.xAdd;
      vsConstData.posMulAdd.w = mCurrentDraw->shaderViewportData.yAdd;
      vsConstData.zSpaceMul.x = mCurrentDraw->shaderViewportData.zAdd;
      vsConstData.zSpaceMul.y = mCurrentDraw->shaderViewportData.zMul;
      *reinterpret_cast<uint32_t*>(&vsConstData.zSpaceMul.z) = mCurrentDraw->baseVertex;
      *reinterpret_cast<uint32_t*>(&vsConstData.zSpaceMul.w) = mCurrentDraw->baseInstance;
      vsConstData.pointSize = mCurrentDraw->pointSize / 8.0f;

      if (!mActiveVsConstantsSet ||
          memcmp(&vsConstData, &mActiveVsConstants, sizeof(vsConstData)) != 0) {
         mActiveVsConstants = vsConstData;
         mActiveVsConstantsSet = true;

         mActiveCommandBuffer.pushConstants<spirv::VertexPushConstants>(
            mPipelineLayout,
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eGeometry,
            spirv::VertexPushConstantsOffset, { vsConstData });
      }
   }

   if (mCurrentDraw->pixelShader) {
      auto lopMode = mCurrentDraw->pipeline->shaderLopMode;
      auto alphaFunc = mCurrentDraw->pipeline->shaderAlphaFunc;
      auto alphaRef = mCurrentDraw->pipeline->shaderAlphaRef;

      spirv::FragmentPushConstants psConstData;
      psConstData.alphaFunc = (lopMode << 8) | static_cast<uint32_t>(alphaFunc);
      psConstData.alphaRef = alphaRef;
      psConstData.needsPremultiply = 0;
      for (auto i = 0; i < latte::MaxRenderTargets; ++i) {
         if (mCurrentDraw->pipeline->needsPremultipliedTargets &&
             !mCurrentDraw->pipeline->targetIsPremultiplied[i]) {
            psConstData.needsPremultiply |= (1 << i);
         }
      }

      if (!mActivePsConstantsSet ||
          memcmp(&psConstData, &mActivePsConstants, sizeof(psConstData)) != 0) {
         mActivePsConstants = psConstData;
         mActivePsConstantsSet = true;

         mActiveCommandBuffer.pushConstants<spirv::FragmentPushConstants>(
            mPipelineLayout, vk::ShaderStageFlagBits::eFragment,
            spirv::FragmentPushConstantsOffset, { psConstData });
      }
   }
}

void
Driver::drawGenericIndexed(latte::VGT_DRAW_INITIATOR drawInit, uint32_t numIndices, void *indices)
{
   // First lets set up our draw description for everyone
   auto pa_su_point_size = getRegister<latte::PA_SU_POINT_SIZE>(latte::Register::PA_SU_POINT_SIZE);
   auto vgt_index_type = getRegister<latte::VGT_NODMA_INDEX_TYPE>(latte::Register::VGT_INDEX_TYPE);
   auto vgt_primitive_type = getRegister<latte::VGT_PRIMITIVE_TYPE>(latte::Register::VGT_PRIMITIVE_TYPE);
   auto sq_vtx_base_vtx_loc = getRegister<latte::SQ_VTX_BASE_VTX_LOC>(latte::Register::SQ_VTX_BASE_VTX_LOC);
   auto sq_vtx_start_inst_loc = getRegister<latte::SQ_VTX_START_INST_LOC>(latte::Register::SQ_VTX_START_INST_LOC);
   auto vgt_dma_num_instances = getRegister<latte::VGT_DMA_NUM_INSTANCES>(latte::Register::VGT_DMA_NUM_INSTANCES);
   auto vgt_dma_index_type = getRegister<latte::VGT_DMA_INDEX_TYPE>(latte::Register::VGT_DMA_INDEX_TYPE);
   auto vgt_strmout_en = getRegister<latte::VGT_STRMOUT_EN>(latte::Register::VGT_STRMOUT_EN);

   bool useStreamOut = vgt_strmout_en.STREAMOUT();
   bool useOpaque = drawInit.USE_OPAQUE();

   if (useOpaque) {
      decaf_check(numIndices == 0);
      decaf_check(!indices);
   }

   DrawDesc& drawDesc = mDrawCache;
   drawDesc.indices = indices;
   drawDesc.indexType = vgt_index_type.INDEX_TYPE();
   drawDesc.indexSwapMode = latte::VGT_DMA_SWAP::NONE;
   drawDesc.primitiveType = vgt_primitive_type.PRIM_TYPE();
   drawDesc.numIndices = numIndices;
   drawDesc.baseVertex = sq_vtx_base_vtx_loc.OFFSET();
   drawDesc.numInstances = 1;
   drawDesc.baseInstance = sq_vtx_start_inst_loc.OFFSET();
   drawDesc.streamOutEnabled = useStreamOut;
   drawDesc.streamOutContext = mStreamOutContext;
   drawDesc.pointSize = pa_su_point_size.WIDTH();

   if (drawInit.SOURCE_SELECT() == latte::VGT_DI_SRC_SEL::DMA) {
      drawDesc.indexType = vgt_dma_index_type.INDEX_TYPE();
      drawDesc.indexSwapMode = vgt_dma_index_type.SWAP_MODE();
      drawDesc.numInstances = vgt_dma_num_instances.NUM_INSTANCES();
   }

   mCurrentDraw = &drawDesc;

   // Set up all the required state, ordering here is very important
   if (!checkCurrentVertexShader()) {
      gLog->debug("Skipped draw due to a vertex shader error");
      return;
   }
   if (!checkCurrentGeometryShader()) {
      gLog->debug("Skipped draw due to a geometry shader error");
      return;
   }
   if (!checkCurrentPixelShader()) {
      gLog->debug("Skipped draw due to a pixel shader error");
      return;
   }
   if (!checkCurrentRectStubShader()) {
      gLog->debug("Skipped draw due to a rect stub shader error");
      return;
   }
   if (!checkCurrentRenderPass()) {
      gLog->debug("Skipped draw due to a render pass error");
      return;
   }
   if (!checkCurrentFramebuffer()) {
      gLog->debug("Skipped draw due to a framebuffer error");
      return;
   }
   if (!checkCurrentPipeline()) {
      gLog->debug("Skipped draw due to a pipeline error");
      return;
   }
   if (!checkCurrentSamplers()) {
      gLog->debug("Skipped draw due to a samplers error");
      return;
   }
   if (!checkCurrentTextures()) {
      gLog->debug("Skipped draw due to a textures error");
      return;
   }
   if (!checkCurrentAttribBuffers()) {
      gLog->debug("Skipped draw due to an attribute buffers error");
      return;
   }
   if (!checkCurrentShaderBuffers()) {
      gLog->debug("Skipped draw due to a shader buffers error");
      return;
   }
   if (!checkCurrentIndices()) {
      gLog->debug("Skipped draw due to an index buffer error");
      return;
   }
   if (!checkCurrentViewportAndScissor()) {
      gLog->debug("Skipped draw due to a viewport or scissor area error");
      return;
   }
   if (!checkCurrentStreamOut()) {
      gLog->debug("Skipped draw due to a stream out buffers error");
      return;
   }

   // These things need to happen up here before we enter the render pass, otherwise
   // the pipeline barrier triggered by the memory cache transition will fail.
   uint32_t opaqueStride = 0;
   MemCacheObject *opaqueCounter = nullptr;
   if (useOpaque) {
      auto vgt_strmout_draw_opaque_vertex_stride = getRegister<uint32_t>(latte::Register::VGT_STRMOUT_DRAW_OPAQUE_VERTEX_STRIDE);
      opaqueStride = vgt_strmout_draw_opaque_vertex_stride << 2;

      auto soBufferSizeAddr = getRegisterAddr(latte::Register::VGT_STRMOUT_DRAW_OPAQUE_BUFFER_FILLED_SIZE);
      opaqueCounter = getDataMemCache(soBufferSizeAddr, 4);

      transitionMemCache(opaqueCounter, ResourceUsage::StreamOutCounterRead);

      drawDesc.opaqueBuffer = opaqueCounter;
      drawDesc.opaqueStride = opaqueStride;
   } else {
      drawDesc.opaqueBuffer = nullptr;
   }

   // Finish this draw
   mCurrentDraw = nullptr;

   // Perform our pending draws if the renderpass has changed
   if (mActiveRenderPass != drawDesc.renderPass || mActiveFramebuffer != drawDesc.framebuffer) {
      flushPendingDraws();

      mActiveRenderPass = drawDesc.renderPass;
      mActiveFramebuffer = drawDesc.framebuffer;
   }

   // If this is a stream-out draw, we have to flush it as a singular draw call.
   if (useStreamOut) {
      flushPendingDraws();
   }

   // Now that we have potentially flushed our draws, lets prepare this draw.  We need to do it in
   // this order, since this draw might use the last renderpasses framebuffer as a texture, and we
   // need to barrier AFTER the previous draws are flushed.
   mCurrentDraw = &drawDesc;
   prepareCurrentTextures();
   prepareCurrentFramebuffer();
   mCurrentDraw = nullptr;

   // Record this pending draw
   mPendingDraws.push_back(drawDesc);

   // Finish with this draw call
   mCurrentDraw = nullptr;

   // If this is a stream-out draw, we have to flush it as a singular draw call.
   if (useStreamOut) {
      flushPendingDraws();
   }
}

void
Driver::flushPendingDraws()
{
   if (mPendingDraws.empty()) {
      return;
   }

   auto& fbRa = mActiveFramebuffer->renderArea;
   auto renderArea = vk::Rect2D { { 0, 0 }, fbRa };

   // Bind and set up everything, and then do our draw
   auto passBeginDesc = vk::RenderPassBeginInfo {};
   passBeginDesc.renderPass = mActiveRenderPass->renderPass;
   passBeginDesc.framebuffer = mActiveFramebuffer->framebuffer;
   passBeginDesc.renderArea = renderArea;
   passBeginDesc.clearValueCount = 0;
   passBeginDesc.pClearValues = nullptr;
   mActiveCommandBuffer.beginRenderPass(passBeginDesc, vk::SubpassContents::eInline);

   for (auto &drawDesc : mPendingDraws) {
      // Make sure that the draw descriptions match our expectations
      decaf_check(mActiveRenderPass == drawDesc.renderPass);
      decaf_check(mActiveFramebuffer == drawDesc.framebuffer);

      mCurrentDraw = &drawDesc;
      drawCurrentState();
      mCurrentDraw = nullptr;
   }
   mPendingDraws.clear();

   mActiveCommandBuffer.endRenderPass();
}

void
Driver::drawCurrentState()
{
   auto &drawDesc = *mCurrentDraw;

   if (mActivePipeline != drawDesc.pipeline) {
      mActiveCommandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, drawDesc.pipeline->pipeline);

      mActivePipeline = drawDesc.pipeline;
   }

   bindAttribBuffers();
   bindDescriptors();
   bindShaderParams();
   bindViewportAndScissor();
   bindIndexBuffer();
   bindStreamOutBuffers();

   if (drawDesc.streamOutEnabled) {
      beginStreamOut();
   }

   if (drawDesc.opaqueBuffer) {
      mActiveCommandBuffer.drawIndirectByteCountEXT(1, 0, drawDesc.opaqueBuffer->buffer, 0, 0, drawDesc.opaqueStride, mVkDynLoader);
   } else if (drawDesc.indexBuffer) {
      mActiveCommandBuffer.drawIndexed(drawDesc.numIndices, drawDesc.numInstances, 0, drawDesc.baseVertex, drawDesc.baseInstance);
   } else {
      mActiveCommandBuffer.draw(drawDesc.numIndices, drawDesc.numInstances, drawDesc.baseVertex, drawDesc.baseInstance);
   }

   if (drawDesc.streamOutEnabled) {
      endStreamOut();
   }
}

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
