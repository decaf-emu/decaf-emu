#ifdef DECAF_VULKAN
#include "vulkan_driver.h"

#include <common/log.h>

namespace vulkan
{

void
Driver::bindShaderDescriptors(ShaderStage shaderStage)
{
   auto shaderStageInt = static_cast<uint32_t>(shaderStage);

   const spirv::Shader *shader;
   if (shaderStage == ShaderStage::Vertex) {
      if (!mCurrentDraw->vertexShader) {
         return;
      }

      shader = &mCurrentDraw->vertexShader->shader;
   } else if (shaderStage == ShaderStage::Geometry) {
      if (!mCurrentDraw->geometryShader) {
         return;
      }

      shader = &mCurrentDraw->geometryShader->shader;
   } else if (shaderStage == ShaderStage::Pixel) {
      if (!mCurrentDraw->pixelShader) {
         return;
      }

      shader = &mCurrentDraw->pixelShader->shader;
   } else {
      decaf_abort("Unexpected shader stage during descriptor build");
   }

   bool dSetHasValues = false;

   std::array<vk::DescriptorImageInfo, latte::MaxSamplers> samplerInfos = { {} };
   for (auto i = 0u; i < latte::MaxSamplers; ++i) {
      if (shader->samplerUsed[i]) {
         auto &sampler = mCurrentDraw->samplers[shaderStageInt][i];
         if (sampler) {
            samplerInfos[i].sampler = sampler->sampler;
         } else {
            samplerInfos[i].sampler = mBlankSampler;
         }

         dSetHasValues = true;
      }
   }

   std::array<vk::DescriptorImageInfo, latte::MaxTextures> textureInfos = { {} };
   for (auto i = 0u; i < latte::MaxTextures; ++i) {
      if (shader->textureUsed[i]) {
         auto &texture = mCurrentDraw->textures[shaderStageInt][i];
         if (texture) {
            textureInfos[i].imageView = texture->imageView;
         } else {
            textureInfos[i].imageView = mBlankImageView;
         }

         textureInfos[i].imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

         dSetHasValues = true;
      }
   }

   std::array<vk::DescriptorBufferInfo, latte::MaxUniformBlocks> bufferInfos = { {} };
   if (mCurrentDraw->gprBuffers[shaderStageInt]) {
      auto gprBuffer = mCurrentDraw->gprBuffers[shaderStageInt];

      bufferInfos[0].buffer = gprBuffer->buffer;
      bufferInfos[0].offset = 0;
      bufferInfos[0].range = gprBuffer->size;

      dSetHasValues = true;
   } else {
      for (auto i = 0u; i < latte::MaxUniformBlocks; ++i) {
         if (shader->cbufferUsed[i]) {
            auto& uniformBuffer = mCurrentDraw->uniformBlocks[shaderStageInt][i];
            if (uniformBuffer) {
               bufferInfos[i].buffer = uniformBuffer->buffer;
               bufferInfos[i].offset = 0;
               bufferInfos[i].range = uniformBuffer->size;
            } else {
               bufferInfos[i].buffer = mBlankBuffer;
               bufferInfos[i].offset = 0;
               bufferInfos[i].range = 1024;
            }

            dSetHasValues = true;
         }
      }
   }

   // If this shader stage has nothing bound, there is no need to
   // actually generate our descriptor sets or anything.
   if (!dSetHasValues) {
      return;
   }

   vk::DescriptorSet dSet;
   if (shaderStage == ShaderStage::Vertex) {
      dSet = allocateVertexDescriptorSet();
   } else if (shaderStage == ShaderStage::Geometry) {
      dSet = allocateGeometryDescriptorSet();
   } else if (shaderStage == ShaderStage::Pixel) {
      dSet = allocatePixelDescriptorSet();
   } else {
      decaf_abort("Unexpected shader stage during descriptor build");
   }

   /*
   for (auto &samplerInfo : samplerInfos) {
      if (!samplerInfo.sampler) {
         samplerInfo.sampler = mBlankSampler;
      }
   }
   for (auto &textureInfo : textureInfos) {
      if (!textureInfo.imageView) {
         textureInfo.imageView = mBlankImageView;
         textureInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
      }
   }
   for (auto &bufferInfo : bufferInfos) {
      if (!bufferInfo.buffer) {
         bufferInfo.buffer = mBlankBuffer;
         bufferInfo.offset = 0;
         bufferInfo.range = 1;
      }
   }
   //*/

   std::array<vk::WriteDescriptorSet, 48> descWrites;
   uint32_t numDescWrites = 0;
   auto pushDescWrite = [&](vk::WriteDescriptorSet writeDesc) {
      decaf_check(numDescWrites < descWrites.size());
      descWrites[numDescWrites++] = writeDesc;
   };

   for (auto i = 0u; i < latte::MaxSamplers; ++i) {
      if (!samplerInfos[i].sampler) {
         continue;
      }

      vk::WriteDescriptorSet writeDesc;
      writeDesc.dstSet = dSet;
      writeDesc.dstBinding = i;
      writeDesc.dstArrayElement = 0;
      writeDesc.descriptorCount = 1;
      writeDesc.descriptorType = vk::DescriptorType::eSampler;
      writeDesc.pImageInfo = &samplerInfos[i];
      writeDesc.pBufferInfo = nullptr;
      writeDesc.pTexelBufferView = nullptr;
      pushDescWrite(writeDesc);
   }

   for (auto i = 0u; i < latte::MaxTextures; ++i) {
      auto &texture = mCurrentDraw->textures[shaderStageInt][i];
      if (!textureInfos[i].imageView) {
         continue;
      }

      vk::WriteDescriptorSet writeDesc;
      writeDesc.dstSet = dSet;
      writeDesc.dstBinding = latte::MaxSamplers + i;
      writeDesc.dstArrayElement = 0;
      writeDesc.descriptorCount = 1;
      writeDesc.descriptorType = vk::DescriptorType::eSampledImage;
      writeDesc.pImageInfo = &textureInfos[i];
      writeDesc.pBufferInfo = nullptr;
      writeDesc.pTexelBufferView = nullptr;
      pushDescWrite(writeDesc);
   }

   for (auto i = 0u; i < latte::MaxUniformBlocks; ++i) {
      if (!bufferInfos[i].buffer) {
         continue;
      }

      vk::WriteDescriptorSet writeDesc;
      writeDesc.dstSet = dSet;
      writeDesc.dstBinding = latte::MaxSamplers + latte::MaxTextures + i;
      writeDesc.dstArrayElement = 0;
      writeDesc.descriptorCount = 1;
      writeDesc.descriptorType = vk::DescriptorType::eUniformBuffer;
      writeDesc.pImageInfo = nullptr;
      writeDesc.pBufferInfo = &bufferInfos[i];
      writeDesc.pTexelBufferView = nullptr;
      pushDescWrite(writeDesc);
   }

   mDevice.updateDescriptorSets(numDescWrites, descWrites.data(), 0, nullptr);

   mActiveCommandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                           mPipelineLayout,
                                           shaderStageInt,
                                           { dSet }, {});
}

void
Driver::bindShaderResources()
{
   // Bind the descriptors for each of the shader stages.
   for (auto i = 0; i < 3; ++i) {
      bindShaderDescriptors(static_cast<ShaderStage>(i));
   }

   // This should probably be split to its own function
   if (mCurrentDraw->vertexShader) {
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

      vsConstData.posMulAdd.x = mCurrentDraw->shaderViewportData.xMul;
      vsConstData.posMulAdd.y = mCurrentDraw->shaderViewportData.yMul;
      vsConstData.posMulAdd.z = mCurrentDraw->shaderViewportData.xAdd;
      vsConstData.posMulAdd.w = mCurrentDraw->shaderViewportData.yAdd;
      vsConstData.zSpaceMul.x = mCurrentDraw->shaderViewportData.zAdd;
      vsConstData.zSpaceMul.y = mCurrentDraw->shaderViewportData.zMul;

      *reinterpret_cast<uint32_t*>(&vsConstData.zSpaceMul.z) = mCurrentDraw->baseVertex;
      *reinterpret_cast<uint32_t*>(&vsConstData.zSpaceMul.w) = mCurrentDraw->baseInstance;

      mActiveCommandBuffer.pushConstants<VsPushConstants>(mPipelineLayout,
                                                          vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eGeometry,
                                                          0, { vsConstData });
   }

   if (mCurrentDraw->pixelShader) {
      auto lopMode = mCurrentDraw->pipeline->shaderLopMode;
      auto alphaFunc = mCurrentDraw->pipeline->shaderAlphaFunc;
      auto alphaRef = mCurrentDraw->pipeline->shaderAlphaRef;

      struct PsPushConstants
      {
         uint32_t alphaData;
         float alphaRef;
         uint32_t needPremultiply;
      } psConstData;
      psConstData.alphaData = (lopMode << 8) | static_cast<uint32_t>(alphaFunc);
      psConstData.alphaRef = alphaRef;

      psConstData.needPremultiply = 0;
      for (auto i = 0; i < latte::MaxRenderTargets; ++i) {
         if (mCurrentDraw->pipeline->needsPremultipliedTargets && !mCurrentDraw->pipeline->targetIsPremultiplied[i]) {
            psConstData.needPremultiply |= (1 << i);
         }
      }

      mActiveCommandBuffer.pushConstants<PsPushConstants>(mPipelineLayout,
                                                          vk::ShaderStageFlagBits::eFragment,
                                                          32, { psConstData });
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
   bindShaderResources();
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
