#ifdef DECAF_VULKAN
#include "vulkan_driver.h"

namespace vulkan
{

template<bool IsRects, typename IndexType>
static void
unpackQuadRectList(uint32_t count,
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

         if (!IsRects) {
            *(dst++) = index_0;
            *(dst++) = index_2;
            *(dst++) = index_3;
         } else {
            // Rectangles use a different winding order apparently...
            *(dst++) = index_2;
            *(dst++) = index_1;
            *(dst++) = index_3;
         }
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

         if (!IsRects) {
            *(dst++) = index_0 + index;
            *(dst++) = index_2 + index;
            *(dst++) = index_3 + index;
         } else {
            // Rectangles use a different winding order apparently...
            *(dst++) = index_2 + index;
            *(dst++) = index_1 + index;
            *(dst++) = index_3 + index;
         }
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
   auto& drawDesc = mCurrentDrawDesc;

   if (drawDesc.indices) {
      if (drawDesc.indexSwapMode == latte::VGT_DMA_SWAP::SWAP_16_BIT) {
         uint32_t indexBytes = calculateIndexBufferSize(drawDesc.indexType, drawDesc.numIndices);
         mScratchIdxSwap.resize(indexBytes);

         auto swapSrc = reinterpret_cast<uint16_t *>(drawDesc.indices);
         auto swapDest = reinterpret_cast<uint16_t *>(mScratchIdxSwap.data());
         for (auto i = 0; i < indexBytes / sizeof(uint16_t); ++i) {
            swapDest[i] = byte_swap(swapSrc[i]);
         }

         drawDesc.indices = mScratchIdxSwap.data();
      } else if (drawDesc.indexSwapMode == latte::VGT_DMA_SWAP::SWAP_32_BIT) {
         uint32_t indexBytes = calculateIndexBufferSize(drawDesc.indexType, drawDesc.numIndices);
         mScratchIdxSwap.resize(indexBytes);

         auto swapSrc = reinterpret_cast<uint32_t *>(drawDesc.indices);
         auto swapDest = reinterpret_cast<uint32_t *>(mScratchIdxSwap.data());
         for (auto i = 0; i < indexBytes / sizeof(uint32_t); ++i) {
            swapDest[i] = byte_swap(swapSrc[i]);
         }

         drawDesc.indices = mScratchIdxSwap.data();
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
   auto &drawDesc = mCurrentDrawDesc;

   if (drawDesc.primitiveType == latte::VGT_DI_PRIMITIVE_TYPE::RECTLIST) {
      uint32_t indexBytes = calculateIndexBufferSize(drawDesc.indexType, drawDesc.numIndices);
      mScratchIdxDequad.resize(indexBytes / 4 * 6);

      if (drawDesc.indexType == latte::VGT_INDEX_TYPE::INDEX_16) {
         unpackQuadRectList<true>(drawDesc.numIndices,
                                  reinterpret_cast<uint16_t*>(drawDesc.indices),
                                  reinterpret_cast<uint16_t*>(mScratchIdxDequad.data()));
      } else if (drawDesc.indexType == latte::VGT_INDEX_TYPE::INDEX_32) {
         unpackQuadRectList<true>(drawDesc.numIndices,
                                  reinterpret_cast<uint32_t*>(drawDesc.indices),
                                  reinterpret_cast<uint32_t*>(mScratchIdxDequad.data()));
      } else {
         decaf_abort("Unexpected index type");
      }

      drawDesc.primitiveType = latte::VGT_DI_PRIMITIVE_TYPE::TRILIST;
      drawDesc.indices = mScratchIdxDequad.data();
      drawDesc.numIndices = drawDesc.numIndices / 4 * 6;
   } else if (drawDesc.primitiveType == latte::VGT_DI_PRIMITIVE_TYPE::QUADLIST) {
      uint32_t indexBytes = calculateIndexBufferSize(drawDesc.indexType, drawDesc.numIndices);
      mScratchIdxDequad.resize(indexBytes / 4 * 6);

      if (drawDesc.indexType == latte::VGT_INDEX_TYPE::INDEX_16) {
         unpackQuadRectList<false>(drawDesc.numIndices,
                                   reinterpret_cast<uint16_t*>(drawDesc.indices),
                                   reinterpret_cast<uint16_t*>(mScratchIdxDequad.data()));
      } else if (drawDesc.indexType == latte::VGT_INDEX_TYPE::INDEX_32) {
         unpackQuadRectList<false>(drawDesc.numIndices,
                                   reinterpret_cast<uint32_t*>(drawDesc.indices),
                                   reinterpret_cast<uint32_t*>(mScratchIdxDequad.data()));
      } else {
         decaf_abort("Unexpected index type");
      }

      drawDesc.primitiveType = latte::VGT_DI_PRIMITIVE_TYPE::TRILIST;
      drawDesc.indices = mScratchIdxDequad.data();
      drawDesc.numIndices = drawDesc.numIndices / 4 * 6;
   }
}

bool
Driver::checkCurrentIndices()
{
   maybeSwapIndices();
   maybeUnpackQuads();

   auto& drawDesc = mCurrentDrawDesc;
   if (drawDesc.indices) {
      auto indexBytes = calculateIndexBufferSize(drawDesc.indexType, drawDesc.numIndices);
      auto indicesBuf = getStagingBuffer(indexBytes);
      auto indicesPtr = mapStagingBuffer(indicesBuf, false);
      memcpy(indicesPtr, drawDesc.indices, indexBytes);
      unmapStagingBuffer(indicesBuf, true);

      mCurrentIndexBuffer = indicesBuf;
   } else {
      mCurrentIndexBuffer = nullptr;
   }

   return true;
}

void
Driver::bindIndexBuffer()
{
   if (!mCurrentIndexBuffer) {
      return;
   }

   auto& drawDesc = mCurrentDrawDesc;

   vk::IndexType bindIndexType;
   if (drawDesc.indexType == latte::VGT_INDEX_TYPE::INDEX_16) {
      bindIndexType = vk::IndexType::eUint16;
   } else if (drawDesc.indexType == latte::VGT_INDEX_TYPE::INDEX_32) {
      bindIndexType = vk::IndexType::eUint32;
   } else {
      decaf_abort("Unexpected index type");
   }

   mActiveCommandBuffer.bindIndexBuffer(mCurrentIndexBuffer->buffer, 0, bindIndexType);
}

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
