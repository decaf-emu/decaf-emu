#ifdef DECAF_VULKAN
#include "vulkan_driver.h"

#include <common/log.h>

namespace vulkan
{

void
Driver::bindDescriptors()
{
   // Ensure the scratch buffers are large enough to guarentee we won't need
   // to perform any reallocations of the buffers.
   mScratchImageInfos.clear();
   mScratchBufferInfos.clear();
   auto &scratchImageInfos = mScratchImageInfos;
   auto &scratchBufferInfos = mScratchBufferInfos;

   // Ensure the descriptor scratch buffers are large enough to never realloc.
   mScratchDescriptorWrites.clear();
   auto &descWrites = mScratchDescriptorWrites;

   vk::DescriptorSet dSet;
   if (!mCurrentDraw->pipeline->pipelineLayout) {
      // If there is no custom pipeline layout configured, this means we have to use
      // standard descriptor sets rather than being able to take advantage of push.
      dSet = allocateGenericDescriptorSet();
   }

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

      auto bindingBase = 32 * shaderStage;

      // We have to ensure the sampler count and texture count match since we pack
      // multiple of these things into a single descriptor.
      static_assert(latte::MaxTextures == latte::MaxSamplers);

      for (auto i = 0u; i < latte::MaxSamplers; ++i) {
         if (shaderMeta->textureUsed[i]) {
            scratchImageInfos.push_back({});
            auto &imageInfo = scratchImageInfos.back();

            auto &sampler = mCurrentDraw->samplers[shaderStage][i];

            if (sampler) {
               imageInfo.sampler = sampler->sampler;
            } else {
               imageInfo.sampler = mBlankSampler;
            }

            auto &texture = mCurrentDraw->textures[shaderStage][i];
            if (texture) {
               imageInfo.imageView = texture->imageView;
            } else {
               imageInfo.imageView = mBlankImageView;
            }

            imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

            // Set up the descriptor
            descWrites.push_back({});
            vk::WriteDescriptorSet& writeDesc = descWrites.back();
            writeDesc.dstSet = dSet;
            writeDesc.dstBinding = bindingBase + i;
            writeDesc.dstArrayElement = 0;
            writeDesc.descriptorCount = 1;
            if (texture && sampler) {
               writeDesc.descriptorType = vk::DescriptorType::eCombinedImageSampler;
            } else if (sampler) {
               writeDesc.descriptorType = vk::DescriptorType::eSampler;
            } else if (texture) {
               writeDesc.descriptorType = vk::DescriptorType::eSampledImage;
            }
            writeDesc.pImageInfo = &imageInfo;
            writeDesc.pBufferInfo = nullptr;
            writeDesc.pTexelBufferView = nullptr;
         }
      }

      if (mCurrentDraw->gprBuffers[shaderStage]) {
         if (shaderMeta->cfileUsed) {
            scratchBufferInfos.push_back({});
            auto &bufferInfo = scratchBufferInfos.back();

            auto gprBuffer = mCurrentDraw->gprBuffers[shaderStage];
            if (gprBuffer) {
               bufferInfo.buffer = gprBuffer->buffer;
               bufferInfo.offset = 0;
               bufferInfo.range = gprBuffer->size;
            } else {
               bufferInfo.buffer = mBlankBuffer;
               bufferInfo.offset = 0;
               bufferInfo.range = 1024;
            }

            descWrites.push_back({});
            vk::WriteDescriptorSet& writeDesc = descWrites.back();
            writeDesc.dstSet = dSet;
            writeDesc.dstBinding = bindingBase + 16;
            writeDesc.dstArrayElement = 0;
            writeDesc.descriptorCount = 1;
            writeDesc.descriptorType = vk::DescriptorType::eUniformBuffer;
            writeDesc.pImageInfo = nullptr;
            writeDesc.pBufferInfo = &bufferInfo;
            writeDesc.pTexelBufferView = nullptr;
         }
      } else {
         for (auto i = 0u; i < latte::MaxUniformBlocks; ++i) {
            if (shaderMeta->cbufferUsed[i]) {
               scratchBufferInfos.push_back({});
               auto &bufferInfo = scratchBufferInfos.back();

               auto& uniformBuffer = mCurrentDraw->uniformBlocks[shaderStage][i];
               if (uniformBuffer) {
                  bufferInfo.buffer = uniformBuffer->buffer;
                  bufferInfo.offset = 0;
                  bufferInfo.range = uniformBuffer->size;
               } else {
                  bufferInfo.buffer = mBlankBuffer;
                  bufferInfo.offset = 0;
                  bufferInfo.range = 1024;
               }

               descWrites.push_back({});
               vk::WriteDescriptorSet& writeDesc = descWrites.back();
               writeDesc.dstSet = dSet;
               writeDesc.dstBinding = bindingBase + 16 + i;
               writeDesc.dstArrayElement = 0;
               writeDesc.descriptorCount = 1;
               writeDesc.descriptorType = vk::DescriptorType::eUniformBuffer;
               writeDesc.pImageInfo = nullptr;
               writeDesc.pBufferInfo = &bufferInfo;
               writeDesc.pTexelBufferView = nullptr;
            }
         }
      }
   }

   if (descWrites.empty()) {
      // If there are no descriptors for this draw, there is nothing to do.
      return;
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
      VsPushConstants vsConstData;
      vsConstData.posMulAdd.x = mCurrentDraw->shaderViewportData.xMul;
      vsConstData.posMulAdd.y = mCurrentDraw->shaderViewportData.yMul;
      vsConstData.posMulAdd.z = mCurrentDraw->shaderViewportData.xAdd;
      vsConstData.posMulAdd.w = mCurrentDraw->shaderViewportData.yAdd;
      vsConstData.zSpaceMul.x = mCurrentDraw->shaderViewportData.zAdd;
      vsConstData.zSpaceMul.y = mCurrentDraw->shaderViewportData.zMul;
      *reinterpret_cast<uint32_t*>(&vsConstData.zSpaceMul.z) = mCurrentDraw->baseVertex;
      *reinterpret_cast<uint32_t*>(&vsConstData.zSpaceMul.w) = mCurrentDraw->baseInstance;

      if (!mActiveVsConstantsSet || memcmp(&vsConstData, &mActiveVsConstants, sizeof(VsPushConstants)) != 0) {
         mActiveVsConstants = vsConstData;
         mActiveVsConstantsSet = true;

         mActiveCommandBuffer.pushConstants<VsPushConstants>(mPipelineLayout,
                                                             vk::ShaderStageFlagBits::eVertex |
                                                             vk::ShaderStageFlagBits::eGeometry,
                                                             0, { vsConstData });
      }
   }

   if (mCurrentDraw->pixelShader) {
      auto lopMode = mCurrentDraw->pipeline->shaderLopMode;
      auto alphaFunc = mCurrentDraw->pipeline->shaderAlphaFunc;
      auto alphaRef = mCurrentDraw->pipeline->shaderAlphaRef;

      PsPushConstants psConstData;
      psConstData.alphaData = (lopMode << 8) | static_cast<uint32_t>(alphaFunc);
      psConstData.alphaRef = alphaRef;
      psConstData.needPremultiply = 0;
      for (auto i = 0; i < latte::MaxRenderTargets; ++i) {
         if (mCurrentDraw->pipeline->needsPremultipliedTargets && !mCurrentDraw->pipeline->targetIsPremultiplied[i]) {
            psConstData.needPremultiply |= (1 << i);
         }
      }

      if (!mActivePsConstantsSet || memcmp(&psConstData, &mActivePsConstants, sizeof(PsPushConstants)) != 0) {
         mActivePsConstants = psConstData;
         mActivePsConstantsSet = true;

         mActiveCommandBuffer.pushConstants<PsPushConstants>(mPipelineLayout,
                                                             vk::ShaderStageFlagBits::eFragment,
                                                             32, { psConstData });
      }
   }
}

void
Driver::drawGenericIndexed(latte::VGT_DRAW_INITIATOR drawInit, uint32_t numIndices, void *indices)
{
   // First lets set up our draw description for everyone
   auto vgt_dma_index_type = getRegister<latte::VGT_DMA_INDEX_TYPE>(latte::Register::VGT_DMA_INDEX_TYPE);
   auto vgt_primitive_type = getRegister<latte::VGT_PRIMITIVE_TYPE>(latte::Register::VGT_PRIMITIVE_TYPE);
   auto vgt_dma_num_instances = getRegister<latte::VGT_DMA_NUM_INSTANCES>(latte::Register::VGT_DMA_NUM_INSTANCES);
   auto sq_vtx_base_vtx_loc = getRegister<latte::SQ_VTX_BASE_VTX_LOC>(latte::Register::SQ_VTX_BASE_VTX_LOC);
   auto sq_vtx_start_inst_loc = getRegister<latte::SQ_VTX_START_INST_LOC>(latte::Register::SQ_VTX_START_INST_LOC);
   auto vgt_strmout_en = getRegister<latte::VGT_STRMOUT_EN>(latte::Register::VGT_STRMOUT_EN);

   bool useStreamOut = vgt_strmout_en.STREAMOUT();
   bool useOpaque = drawInit.USE_OPAQUE();

   if (useOpaque) {
      decaf_check(numIndices == 0);
      decaf_check(!indices);
   }

   DrawDesc& drawDesc = mDrawCache;
   drawDesc.indices = indices;
   drawDesc.indexType = vgt_dma_index_type.INDEX_TYPE();
   drawDesc.indexSwapMode = vgt_dma_index_type.SWAP_MODE();
   drawDesc.primitiveType = vgt_primitive_type.PRIM_TYPE();
   drawDesc.isRectDraw = false;
   drawDesc.numIndices = numIndices;
   drawDesc.baseVertex = sq_vtx_base_vtx_loc.OFFSET();
   drawDesc.numInstances = vgt_dma_num_instances.NUM_INSTANCES();
   drawDesc.baseInstance = sq_vtx_start_inst_loc.OFFSET();
   drawDesc.streamOutEnabled = useStreamOut;
   drawDesc.streamOutContext = mStreamOutContext;
   mCurrentDraw = &drawDesc;

   if (drawDesc.primitiveType == latte::VGT_DI_PRIMITIVE_TYPE::RECTLIST) {
      drawDesc.isRectDraw = true;
      decaf_check(drawDesc.numIndices == 4);
   }

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
