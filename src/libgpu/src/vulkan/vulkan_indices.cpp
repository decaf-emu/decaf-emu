#ifdef DECAF_VULKAN
#include "vulkan_driver.h"
#include <common/byte_swap_array.h>

namespace vulkan
{

template<typename IndexType>
static void
unpackQuadList(uint32_t count,
               const IndexType *src,
               IndexType *dst)
{
   // Unpack quad indices into triangle indices
   if (src) {
      for (auto i = 0u; i < count / 4; ++i) {
         auto index_0 = *src++;
         auto index_1 = *src++;
         auto index_2 = *src++;
         auto index_3 = *src++;

         *(dst++) = index_0;
         *(dst++) = index_1;
         *(dst++) = index_2;

         *(dst++) = index_0;
         *(dst++) = index_2;
         *(dst++) = index_3;
      }
   } else {
      auto index_0 = 0u;
      auto index_1 = 1u;
      auto index_2 = 2u;
      auto index_3 = 3u;

      for (auto i = 0u; i < count / 4; ++i) {
         auto index = i * 4;
         *(dst++) = index_0 + index;
         *(dst++) = index_1 + index;
         *(dst++) = index_2 + index;

         *(dst++) = index_0 + index;
         *(dst++) = index_2 + index;
         *(dst++) = index_3 + index;
      }
   }
}

static inline uint32_t
calculateIndexBufferSize(latte::VGT_INDEX_TYPE indexType, uint32_t numIndices)
{
   switch (indexType) {
   case latte::VGT_INDEX_TYPE::INDEX_16:
      return numIndices * 2;
   case latte::VGT_INDEX_TYPE::INDEX_32:
      return numIndices * 4;
   }

   decaf_abort("Unexpected index type");
}

void
Driver::maybeSwapIndices()
{
   auto& drawDesc = *mCurrentDraw;
   auto& indices = mCurrentDraw->indices;

   if (indices) {
      if (mCurrentDraw->indexSwapMode == latte::VGT_DMA_SWAP::SWAP_16_BIT) {
         uint32_t indexBytes = calculateIndexBufferSize(mCurrentDraw->indexType, drawDesc.numIndices);
         indices = byte_swap_to_scratch<uint16_t>(indices, indexBytes, mScratchIdxSwap);
      } else if (drawDesc.indexSwapMode == latte::VGT_DMA_SWAP::SWAP_32_BIT) {
         uint32_t indexBytes = calculateIndexBufferSize(mCurrentDraw->indexType, drawDesc.numIndices);
         indices = byte_swap_to_scratch<uint32_t>(indices, indexBytes, mScratchIdxSwap);
      } else if (drawDesc.indexSwapMode == latte::VGT_DMA_SWAP::NONE) {
         // Nothing to do here!
      } else {
         decaf_abort(fmt::format("Unimplemented vgt_dma_index_type.SWAP_MODE {}", drawDesc.indexSwapMode));
      }
   }
}

void
Driver::maybeUnpackQuads()
{
   auto &drawDesc = *mCurrentDraw;
   auto &indices = mCurrentDraw->indices;

   if (drawDesc.primitiveType == latte::VGT_DI_PRIMITIVE_TYPE::QUADLIST) {
      uint32_t indexBytes = calculateIndexBufferSize(drawDesc.indexType, drawDesc.numIndices);
      mScratchIdxDequad.resize(indexBytes / 4 * 6);

      if (drawDesc.indexType == latte::VGT_INDEX_TYPE::INDEX_16) {
         unpackQuadList(drawDesc.numIndices,
                        reinterpret_cast<uint16_t*>(indices),
                        reinterpret_cast<uint16_t*>(mScratchIdxDequad.data()));
      } else if (drawDesc.indexType == latte::VGT_INDEX_TYPE::INDEX_32) {
         unpackQuadList(drawDesc.numIndices,
                        reinterpret_cast<uint32_t*>(indices),
                        reinterpret_cast<uint32_t*>(mScratchIdxDequad.data()));
      } else {
         decaf_abort("Unexpected index type");
      }

      drawDesc.primitiveType = latte::VGT_DI_PRIMITIVE_TYPE::TRILIST;
      drawDesc.numIndices = drawDesc.numIndices / 4 * 6;
      indices = mScratchIdxDequad.data();
   }
}

bool
Driver::checkCurrentIndices()
{
   auto& drawDesc = *mCurrentDraw;

   if (mLastIndexBufferSet) {
      if (drawDesc.indices == mLastIndexBuffer.indexData &&
          drawDesc.indexType == mLastIndexBuffer.indexType &&
          drawDesc.numIndices == mLastIndexBuffer.numIndices &&
          drawDesc.indexSwapMode == mLastIndexBuffer.swapMode &&
          drawDesc.primitiveType == mLastIndexBuffer.primitiveType)
      {
         drawDesc.primitiveType = mLastIndexBuffer.newPrimitiveType;
         drawDesc.numIndices = mLastIndexBuffer.newNumIndices;
         drawDesc.indexBuffer = mLastIndexBuffer.indexBuffer;
         return true;
      }
   }

   mLastIndexBuffer.indexData = drawDesc.indices;
   mLastIndexBuffer.indexType = drawDesc.indexType;
   mLastIndexBuffer.numIndices = drawDesc.numIndices;
   mLastIndexBuffer.swapMode = drawDesc.indexSwapMode;
   mLastIndexBuffer.primitiveType = drawDesc.primitiveType;

   maybeSwapIndices();
   maybeUnpackQuads();

   if (drawDesc.indices) {
      auto indexBytes = calculateIndexBufferSize(drawDesc.indexType, drawDesc.numIndices);
      auto indicesBuf = getStagingBuffer(indexBytes, StagingBufferType::CpuToGpu);
      copyToStagingBuffer(indicesBuf, 0, drawDesc.indices, indexBytes);
      transitionStagingBuffer(indicesBuf, ResourceUsage::IndexBuffer);

      drawDesc.indexBuffer = indicesBuf;
   } else {
      drawDesc.indexBuffer = nullptr;
   }

   mLastIndexBuffer.newPrimitiveType = drawDesc.primitiveType;
   mLastIndexBuffer.newNumIndices = drawDesc.numIndices;
   mLastIndexBuffer.indexBuffer = drawDesc.indexBuffer;
   mLastIndexBufferSet = true;

   return true;
}

void
Driver::bindIndexBuffer()
{
   if (!mCurrentDraw->indexBuffer) {
      return;
   }

   auto& drawDesc = *mCurrentDraw;

   vk::IndexType bindIndexType;
   if (drawDesc.indexType == latte::VGT_INDEX_TYPE::INDEX_16) {
      bindIndexType = vk::IndexType::eUint16;
   } else if (drawDesc.indexType == latte::VGT_INDEX_TYPE::INDEX_32) {
      bindIndexType = vk::IndexType::eUint32;
   } else {
      decaf_abort("Unexpected index type");
   }

   mActiveCommandBuffer.bindIndexBuffer(mCurrentDraw->indexBuffer->buffer, 0, bindIndexType);
}

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
