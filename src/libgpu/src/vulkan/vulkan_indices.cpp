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
      for (IndexType i = 0u; i < count / 4; ++i) {
         IndexType index_0 = *src++;
         IndexType index_1 = *src++;
         IndexType index_2 = *src++;
         IndexType index_3 = *src++;

         *(dst++) = index_0;
         *(dst++) = index_1;
         *(dst++) = index_2;

         *(dst++) = index_0;
         *(dst++) = index_2;
         *(dst++) = index_3;
      }
   } else {
      IndexType index_0 = 0u;
      IndexType index_1 = 1u;
      IndexType index_2 = 2u;
      IndexType index_3 = 3u;

      for (IndexType i = 0u; i < count / 4; ++i) {
         IndexType index = i * 4;

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
Driver::maybeUnpackPrimitiveIndices()
{
   auto &drawDesc = *mCurrentDraw;
   auto &indices = mCurrentDraw->indices;


   if (drawDesc.primitiveType == latte::VGT_DI_PRIMITIVE_TYPE::QUADLIST) {
      auto indexBytes = calculateIndexBufferSize(drawDesc.indexType, drawDesc.numIndices);
      mScratchIdxPrim.resize(indexBytes / 4 * 6);

      if (drawDesc.indexType == latte::VGT_INDEX_TYPE::INDEX_16) {
         unpackQuadList(drawDesc.numIndices,
                        reinterpret_cast<uint16_t*>(indices),
                        reinterpret_cast<uint16_t*>(mScratchIdxPrim.data()));
      } else if (drawDesc.indexType == latte::VGT_INDEX_TYPE::INDEX_32) {
         unpackQuadList(drawDesc.numIndices,
                        reinterpret_cast<uint32_t*>(indices),
                        reinterpret_cast<uint32_t*>(mScratchIdxPrim.data()));
      } else {
         decaf_abort("Unexpected index type");
      }

      drawDesc.primitiveType = latte::VGT_DI_PRIMITIVE_TYPE::TRILIST;
      drawDesc.numIndices = drawDesc.numIndices / 4 * 6;
      indices = mScratchIdxPrim.data();
   } else if (drawDesc.primitiveType == latte::VGT_DI_PRIMITIVE_TYPE::LINELOOP) {
      auto indexBytes = calculateIndexBufferSize(drawDesc.indexType, drawDesc.numIndices + 1);
      mScratchIdxPrim.resize(indexBytes);

      if (drawDesc.indexType == latte::VGT_INDEX_TYPE::INDEX_16) {
         auto dst = reinterpret_cast<uint16_t *>(mScratchIdxPrim.data());
         std::memcpy(dst, indices, indexBytes - 2);
         dst[drawDesc.numIndices] = dst[0];
      } else if (drawDesc.indexType == latte::VGT_INDEX_TYPE::INDEX_32) {
         auto dst = reinterpret_cast<uint32_t *>(mScratchIdxPrim.data());
         std::memcpy(dst, indices, indexBytes - 4);
         dst[drawDesc.numIndices] = dst[0];
      } else {
         decaf_abort("Unexpected index type");
      }

      drawDesc.primitiveType = latte::VGT_DI_PRIMITIVE_TYPE::LINESTRIP;
      drawDesc.numIndices = drawDesc.numIndices + 1;
      indices = mScratchIdxPrim.data();
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
   maybeUnpackPrimitiveIndices();

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
