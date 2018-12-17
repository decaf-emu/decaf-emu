#ifdef DECAF_VULKAN
#include "vulkan_driver.h"

namespace vulkan
{

void
Driver::updateDrawGprBuffer(ShaderStage shaderStage)
{
   auto shaderStageInt = static_cast<uint32_t>(shaderStage);
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

   auto gprsBuffer = getStagingBuffer(256 * 4 * 4, StagingBufferType::CpuToGpu);
   copyToStagingBuffer(gprsBuffer, 0, registerVals, 256 * 4 * 4);

   if (shaderStage == ShaderStage::Vertex) {
      transitionStagingBuffer(gprsBuffer, ResourceUsage::VertexUniforms);
   } else if (shaderStage == ShaderStage::Geometry) {
      transitionStagingBuffer(gprsBuffer, ResourceUsage::GeometryUniforms);
   } else if (shaderStage == ShaderStage::Pixel) {
      transitionStagingBuffer(gprsBuffer, ResourceUsage::PixelUniforms);
   } else {
      decaf_abort("Unexpected shader stage in GPR buffer setup");
   }

   mCurrentDraw->gprBuffers[shaderStageInt] = gprsBuffer;
}

void
Driver::updateDrawUniformBuffer(ShaderStage shaderStage, uint32_t cbufferIdx)
{
   auto shaderStageInt = static_cast<uint32_t>(shaderStage);

   uint32_t cacheBase;
   uint32_t sizeBase;
   if (shaderStage == ShaderStage::Vertex) {
      cacheBase = latte::Register::SQ_ALU_CONST_CACHE_VS_0;
      sizeBase = latte::Register::SQ_ALU_CONST_BUFFER_SIZE_VS_0;
   } else if (shaderStage == ShaderStage::Geometry) {
      cacheBase = latte::Register::SQ_ALU_CONST_CACHE_GS_0;
      sizeBase = latte::Register::SQ_ALU_CONST_BUFFER_SIZE_GS_0;
   } else if (shaderStage == ShaderStage::Pixel) {
      cacheBase = latte::Register::SQ_ALU_CONST_CACHE_PS_0;
      sizeBase = latte::Register::SQ_ALU_CONST_BUFFER_SIZE_PS_0;
   } else {
      decaf_abort("Unexpected shader stage");
   }

   auto sq_alu_const_cache_vs = getRegister<uint32_t>(cacheBase + 4 * cbufferIdx);
   auto sq_alu_const_buffer_size_vs = getRegister<uint32_t>(sizeBase + 4 * cbufferIdx);

   auto bufferPtr = phys_addr(sq_alu_const_cache_vs << 8);
   auto bufferSize = sq_alu_const_buffer_size_vs << 8;

   if (!bufferPtr || bufferSize == 0) {
      mCurrentDraw->uniformBlocks[shaderStageInt][cbufferIdx] = nullptr;
      return;
   }

   auto &currentUniformBlock = mCurrentDraw->uniformBlocks[shaderStageInt][cbufferIdx];
   if (currentUniformBlock &&
       currentUniformBlock->address == bufferPtr &&
       currentUniformBlock->size == bufferSize) {
      // We already have the correct buffer, no need to look it up again.
      return;
   }

   auto memCache = getDataMemCache(bufferPtr, bufferSize);

   if (shaderStage == ShaderStage::Vertex) {
      transitionMemCache(memCache, ResourceUsage::VertexUniforms);
   } else if (shaderStage == ShaderStage::Geometry) {
      transitionMemCache(memCache, ResourceUsage::GeometryUniforms);
   } else if (shaderStage == ShaderStage::Pixel) {
      transitionMemCache(memCache, ResourceUsage::PixelUniforms);
   } else {
      decaf_abort("Unexpected shader stage");
   }

   currentUniformBlock = memCache;
}

bool
Driver::checkCurrentShaderBuffers()
{
   auto sq_config = getRegister<latte::SQ_CONFIG>(latte::Register::SQ_CONFIG);
   bool isDx9Consts = sq_config.DX9_CONSTS();

   if (!isDx9Consts) {
      for (auto shaderStage = 0; shaderStage < 3; ++shaderStage) {
         mCurrentDraw->gprBuffers[shaderStage] = nullptr;
      }

      if (mCurrentDraw->vertexShader) {
         for (auto blockIdx = 0; blockIdx < latte::MaxUniformBlocks; ++blockIdx) {
            if (mCurrentDraw->vertexShader->shader.meta.cbufferUsed[blockIdx]) {
               updateDrawUniformBuffer(ShaderStage::Vertex, blockIdx);
            } else {
               mCurrentDraw->uniformBlocks[0][blockIdx] = nullptr;
            }
         }
      } else {
         for (auto blockIdx = 0; blockIdx < latte::MaxUniformBlocks; ++blockIdx) {
            mCurrentDraw->uniformBlocks[0][blockIdx] = nullptr;
         }
      }

      if (mCurrentDraw->geometryShader) {
         for (auto blockIdx = 0; blockIdx < latte::MaxUniformBlocks; ++blockIdx) {
            if (mCurrentDraw->geometryShader->shader.meta.cbufferUsed[blockIdx]) {
               updateDrawUniformBuffer(ShaderStage::Geometry, blockIdx);
            } else {
               mCurrentDraw->uniformBlocks[1][blockIdx] = nullptr;
            }
         }
      } else {
         for (auto blockIdx = 0; blockIdx < latte::MaxUniformBlocks; ++blockIdx) {
            mCurrentDraw->uniformBlocks[1][blockIdx] = nullptr;
         }
      }

      if (mCurrentDraw->pixelShader) {
         for (auto blockIdx = 0; blockIdx < latte::MaxUniformBlocks; ++blockIdx) {
            if (mCurrentDraw->pixelShader->shader.meta.cbufferUsed[blockIdx]) {
               updateDrawUniformBuffer(ShaderStage::Pixel, blockIdx);
            } else {
               mCurrentDraw->uniformBlocks[2][blockIdx] = nullptr;
            }
         }
      } else {
         for (auto blockIdx = 0; blockIdx < latte::MaxUniformBlocks; ++blockIdx) {
            mCurrentDraw->uniformBlocks[2][blockIdx] = nullptr;
         }
      }
   } else {
      for (auto shaderStage = 0; shaderStage < 3; ++shaderStage) {
         for (auto blockIdx = 0; blockIdx < latte::MaxUniformBlocks; ++blockIdx) {
            mCurrentDraw->uniformBlocks[shaderStage][blockIdx] = nullptr;
         }
      }

      if (mCurrentDraw->vertexShader) {
         if (mCurrentDraw->vertexShader->shader.meta.cfileUsed) {
            updateDrawGprBuffer(ShaderStage::Vertex);
         } else {
            mCurrentDraw->gprBuffers[0] = nullptr;
         }
      } else {
         mCurrentDraw->gprBuffers[0] = nullptr;
      }

      if (mCurrentDraw->geometryShader) {
         if (mCurrentDraw->geometryShader->shader.meta.cfileUsed) {
            updateDrawGprBuffer(ShaderStage::Geometry);
         } else {
            mCurrentDraw->gprBuffers[1] = nullptr;
         }
      } else {
         mCurrentDraw->gprBuffers[1] = nullptr;
      }

      if (mCurrentDraw->pixelShader) {
         if (mCurrentDraw->pixelShader->shader.meta.cfileUsed) {
            updateDrawGprBuffer(ShaderStage::Pixel);
         } else {
            mCurrentDraw->gprBuffers[2] = nullptr;
         }
      } else {
         mCurrentDraw->gprBuffers[2] = nullptr;
      }
   }

   return true;
}

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
