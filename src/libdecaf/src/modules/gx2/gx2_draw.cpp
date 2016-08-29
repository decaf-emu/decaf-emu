#include "gx2_draw.h"
#include "gx2_enum_string.h"
#include "gpu/pm4_writer.h"
#include "common/decaf_assert.h"

namespace gx2
{

void
GX2SetAttribBuffer(uint32_t index,
                   uint32_t size,
                   uint32_t stride,
                   void *buffer)
{
   pm4::SetVtxResource res;
   memset(&res, 0, sizeof(pm4::SetVtxResource));
   res.id = (latte::SQ_VS_ATTRIB_RESOURCE_0 + index) * 7;
   res.baseAddress = buffer;

   res.word1 = res.word1
      .SIZE(size - 1);

   res.word2 = res.word2
      .STRIDE(stride);

   res.word6 = res.word6
      .TYPE(latte::SQ_TEX_VTX_VALID_BUFFER);

   pm4::write(res);
}

void
GX2DrawEx(GX2PrimitiveMode mode,
          uint32_t count,
          uint32_t offset,
          uint32_t numInstances)
{
   auto vgt_draw_initiator = latte::VGT_DRAW_INITIATOR::get(0);

   pm4::write(pm4::SetControlConstant { latte::Register::SQ_VTX_BASE_VTX_LOC, offset });
   pm4::write(pm4::SetConfigReg { latte::Register::VGT_PRIMITIVE_TYPE, mode });
   pm4::write(pm4::NumInstances { numInstances });
   pm4::write(pm4::DrawIndexAuto { count, vgt_draw_initiator });
}

void
GX2DrawIndexedEx(GX2PrimitiveMode mode,
                 uint32_t count,
                 GX2IndexType indexType,
                 void *indices,
                 uint32_t offset,
                 uint32_t numInstances)
{
   auto index_type = latte::VGT_INDEX::VGT_INDEX_16;
   auto swap_mode = latte::VGT_DMA_SWAP::VGT_DMA_SWAP_NONE;

   switch (indexType) {
   case GX2IndexType::U16:
      index_type = latte::VGT_INDEX::VGT_INDEX_16;
      swap_mode = latte::VGT_DMA_SWAP::VGT_DMA_SWAP_16_BIT;
      break;
   case GX2IndexType::U16_LE:
      index_type = latte::VGT_INDEX::VGT_INDEX_16;
      swap_mode = latte::VGT_DMA_SWAP::VGT_DMA_SWAP_NONE;
      break;
   case GX2IndexType::U32:
      index_type = latte::VGT_INDEX::VGT_INDEX_32;
      swap_mode = latte::VGT_DMA_SWAP::VGT_DMA_SWAP_32_BIT;
      break;
   case GX2IndexType::U32_LE:
      index_type = latte::VGT_INDEX::VGT_INDEX_32;
      swap_mode = latte::VGT_DMA_SWAP::VGT_DMA_SWAP_NONE;
      break;
   default:
      decaf_abort(fmt::format("Invalid GX2IndexType {}", enumAsString(indexType)));
   }

   auto vgt_dma_index_type = latte::VGT_DMA_INDEX_TYPE::get(0)
      .INDEX_TYPE(index_type)
      .SWAP_MODE(swap_mode);

   auto vgt_draw_initiator = latte::VGT_DRAW_INITIATOR::get(0)
      .SOURCE_SELECT(latte::VGT_DI_SRC_SEL_DMA);

   pm4::write(pm4::SetControlConstant { latte::Register::SQ_VTX_BASE_VTX_LOC, offset });
   pm4::write(pm4::SetConfigReg { latte::Register::VGT_PRIMITIVE_TYPE, mode });
   pm4::write(pm4::IndexType { vgt_dma_index_type });
   pm4::write(pm4::NumInstances { numInstances });
   pm4::write(pm4::DrawIndex2 { static_cast<uint32_t>(-1), indices, count, vgt_draw_initiator });
}

void
GX2DrawIndexedImmediateEx(GX2PrimitiveMode mode,
                          uint32_t count,
                          GX2IndexType indexType,
                          void *indices,
                          uint32_t offset,
                          uint32_t numInstances)
{
   auto index_type = latte::VGT_INDEX_16;
   auto swap_mode = latte::VGT_DMA_SWAP_NONE;
   auto indexBytes = 0u;

   switch (indexType) {
   case GX2IndexType::U16:
      index_type = latte::VGT_INDEX_16;
      swap_mode = latte::VGT_DMA_SWAP_16_BIT;
      break;
   case GX2IndexType::U16_LE:
      index_type = latte::VGT_INDEX_16;
      swap_mode = latte::VGT_DMA_SWAP_NONE;
      break;
   case GX2IndexType::U32:
      index_type = latte::VGT_INDEX_32;
      swap_mode = latte::VGT_DMA_SWAP_32_BIT;
      break;
   case GX2IndexType::U32_LE:
      index_type = latte::VGT_INDEX_32;
      swap_mode = latte::VGT_DMA_SWAP_NONE;
      break;
   default:
      decaf_abort(fmt::format("Invalid GX2IndexType {}", enumAsString(indexType)));
   }

   auto vgt_dma_index_type = latte::VGT_DMA_INDEX_TYPE::get(0)
      .INDEX_TYPE(index_type)
      .SWAP_MODE(swap_mode);

   auto vgt_draw_initiator = latte::VGT_DRAW_INITIATOR::get(0)
      .SOURCE_SELECT(latte::VGT_DI_SRC_SEL_IMMEDIATE);

   pm4::write(pm4::SetControlConstant { latte::Register::SQ_VTX_BASE_VTX_LOC, offset });
   pm4::write(pm4::SetConfigReg { latte::Register::VGT_PRIMITIVE_TYPE, mode });
   pm4::write(pm4::IndexType { vgt_dma_index_type });
   pm4::write(pm4::NumInstances { numInstances });

   auto numWords = 0u;

   if (index_type == latte::VGT_INDEX_16) {
      numWords = (count + 1) / 2;
   } else if (index_type == latte::VGT_INDEX_32) {
      numWords = count;
   } else {
      decaf_abort(fmt::format("Invalid index_type {}", index_type));
   }

   if (indexType == GX2IndexType::U16_LE) {
      pm4::write(pm4::DrawIndexImmdWriteOnly16LE {
         count,
         vgt_draw_initiator,
         gsl::as_span(reinterpret_cast<uint16_t *>(indices), count)
      });
   } else {
      pm4::write(pm4::DrawIndexImmd {
         count,
         vgt_draw_initiator,
         gsl::as_span(reinterpret_cast<uint32_t *>(indices), numWords)
      });
   }
}

void
GX2SetPrimitiveRestartIndex(uint32_t index)
{
   pm4::write(pm4::SetContextReg { latte::Register::VGT_MULTI_PRIM_IB_RESET_INDX, index });
}

} // namespace gx2
