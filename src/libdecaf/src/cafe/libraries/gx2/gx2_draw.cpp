#include "gx2.h"
#include "gx2_cbpool.h"
#include "gx2_draw.h"
#include "gx2_enum_string.h"
#include "gx2_shaders.h"

#include "cafe/libraries/coreinit/coreinit_memory.h"

#include <common/decaf_assert.h>
#include <cstring>
#include <fmt/format.h>

namespace cafe::gx2
{

using namespace cafe::coreinit;

void
GX2SetAttribBuffer(uint32_t index,
                   uint32_t size,
                   uint32_t stride,
                   virt_ptr<void> buffer)
{
   latte::pm4::SetVtxResource res;
   std::memset(&res, 0, sizeof(latte::pm4::SetVtxResource));

   res.id = (latte::SQ_RES_OFFSET::VS_ATTRIB_RESOURCE_0 + index) * 7;
   res.baseAddress = OSEffectiveToPhysical(virt_cast<virt_addr>(buffer));

   res.word1 = res.word1
      .SIZE(size - 1);

   res.word2 = res.word2
      .STRIDE(stride);

   res.word6 = res.word6
      .TYPE(latte::SQ_TEX_VTX_TYPE::VALID_BUFFER);

   internal::writePM4(res);
}

void
GX2DrawEx(GX2PrimitiveMode mode,
          uint32_t count,
          uint32_t offset,
          uint32_t numInstances)
{
   auto vgt_draw_initiator = latte::VGT_DRAW_INITIATOR::get(0);

   internal::writePM4(latte::pm4::SetControlConstant {
      latte::Register::SQ_VTX_BASE_VTX_LOC,
      offset
   });

   internal::writePM4(latte::pm4::SetConfigReg {
      latte::Register::VGT_PRIMITIVE_TYPE,
      mode
   });

   internal::writePM4(latte::pm4::NumInstances {
      numInstances
   });

   internal::writePM4(latte::pm4::DrawIndexAuto {
      count,
      vgt_draw_initiator
   });
}

void
GX2DrawEx2(GX2PrimitiveMode mode,
           uint32_t count,
           uint32_t offset,
           uint32_t numInstances,
           uint32_t baseInstance)
{
   internal::writePM4(latte::pm4::SetControlConstant {
      latte::Register::SQ_VTX_START_INST_LOC,
      latte::SQ_VTX_START_INST_LOC::get(0)
         .OFFSET(baseInstance)
         .value
   });

   GX2DrawEx(mode, count, offset, numInstances);

   internal::writePM4(latte::pm4::SetControlConstant {
      latte::Register::SQ_VTX_START_INST_LOC,
      latte::SQ_VTX_START_INST_LOC::get(0)
         .OFFSET(0)
         .value
   });
}

void
GX2DrawIndexedEx(GX2PrimitiveMode mode,
                 uint32_t count,
                 GX2IndexType indexType,
                 virt_ptr<void> indices,
                 uint32_t offset,
                 uint32_t numInstances)
{
   auto index_type = latte::VGT_INDEX_TYPE::INDEX_16;
   auto swap_mode = latte::VGT_DMA_SWAP::NONE;

   switch (indexType) {
   case GX2IndexType::U16:
      index_type = latte::VGT_INDEX_TYPE::INDEX_16;
      swap_mode = latte::VGT_DMA_SWAP::SWAP_16_BIT;
      break;
   case GX2IndexType::U16_LE:
      index_type = latte::VGT_INDEX_TYPE::INDEX_16;
      swap_mode = latte::VGT_DMA_SWAP::NONE;
      break;
   case GX2IndexType::U32:
      index_type = latte::VGT_INDEX_TYPE::INDEX_32;
      swap_mode = latte::VGT_DMA_SWAP::SWAP_32_BIT;
      break;
   case GX2IndexType::U32_LE:
      index_type = latte::VGT_INDEX_TYPE::INDEX_32;
      swap_mode = latte::VGT_DMA_SWAP::NONE;
      break;
   default:
      decaf_abort(fmt::format("Invalid GX2IndexType {}", to_string(indexType)));
   }

   auto vgt_dma_index_type = latte::VGT_DMA_INDEX_TYPE::get(0)
      .INDEX_TYPE(index_type)
      .SWAP_MODE(swap_mode);

   auto vgt_draw_initiator = latte::VGT_DRAW_INITIATOR::get(0)
      .SOURCE_SELECT(latte::VGT_DI_SRC_SEL::DMA);

   if (mode & 0x80) {
      vgt_draw_initiator = vgt_draw_initiator
         .MAJOR_MODE(latte::VGT_DI_MAJOR_MODE::MODE1);
   }

   internal::writePM4(latte::pm4::SetControlConstant {
      latte::Register::SQ_VTX_BASE_VTX_LOC,
      offset
   });

   internal::writePM4(latte::pm4::SetConfigReg {
      latte::Register::VGT_PRIMITIVE_TYPE, mode
   });

   internal::writePM4(latte::pm4::IndexType {
      vgt_dma_index_type
   });

   internal::writePM4(latte::pm4::NumInstances {
      numInstances
   });

   internal::writePM4(latte::pm4::DrawIndex2 {
      static_cast<uint32_t>(-1),
      OSEffectiveToPhysical(virt_cast<virt_addr>(indices)),
      count,
      vgt_draw_initiator
   });
}

void
GX2DrawIndexedEx2(GX2PrimitiveMode mode,
                  uint32_t count,
                  GX2IndexType indexType,
                  virt_ptr<void> indices,
                  uint32_t offset,
                  uint32_t numInstances,
                  uint32_t baseInstance)
{
   internal::writePM4(latte::pm4::SetControlConstant {
      latte::Register::SQ_VTX_START_INST_LOC,
      latte::SQ_VTX_START_INST_LOC::get(0)
         .OFFSET(baseInstance)
         .value
   });

   GX2DrawIndexedEx(mode, count, indexType, indices, offset, numInstances);

   internal::writePM4(latte::pm4::SetControlConstant {
      latte::Register::SQ_VTX_START_INST_LOC,
      latte::SQ_VTX_START_INST_LOC::get(0)
         .OFFSET(0)
         .value
   });
}

void
GX2DrawIndexedImmediateEx(GX2PrimitiveMode mode,
                          uint32_t count,
                          GX2IndexType indexType,
                          virt_ptr<void> indices,
                          uint32_t offset,
                          uint32_t numInstances)
{
   auto index_type = latte::VGT_INDEX_TYPE::INDEX_16;
   auto swap_mode = latte::VGT_DMA_SWAP::NONE;
   auto indexBytes = 0u;

   switch (indexType) {
   case GX2IndexType::U16:
      index_type = latte::VGT_INDEX_TYPE::INDEX_16;
      swap_mode = latte::VGT_DMA_SWAP::SWAP_16_BIT;
      break;
   case GX2IndexType::U16_LE:
      index_type = latte::VGT_INDEX_TYPE::INDEX_16;
      swap_mode = latte::VGT_DMA_SWAP::NONE;
      break;
   case GX2IndexType::U32:
      index_type = latte::VGT_INDEX_TYPE::INDEX_32;
      swap_mode = latte::VGT_DMA_SWAP::SWAP_32_BIT;
      break;
   case GX2IndexType::U32_LE:
      index_type = latte::VGT_INDEX_TYPE::INDEX_32;
      swap_mode = latte::VGT_DMA_SWAP::NONE;
      break;
   default:
      decaf_abort(fmt::format("Invalid GX2IndexType {}", to_string(indexType)));
   }

   auto vgt_dma_index_type = latte::VGT_DMA_INDEX_TYPE::get(0)
      .INDEX_TYPE(index_type)
      .SWAP_MODE(swap_mode);

   auto vgt_draw_initiator = latte::VGT_DRAW_INITIATOR::get(0)
      .SOURCE_SELECT(latte::VGT_DI_SRC_SEL::IMMEDIATE);

   internal::writePM4(latte::pm4::SetControlConstant {
      latte::Register::SQ_VTX_BASE_VTX_LOC,
      offset
   });

   internal::writePM4(latte::pm4::SetConfigReg {
      latte::Register::VGT_PRIMITIVE_TYPE,
      mode
   });

   internal::writePM4(latte::pm4::IndexType {
      vgt_dma_index_type
   });

   internal::writePM4(latte::pm4::NumInstances {
      numInstances
   });

   auto numWords = 0u;

   if (index_type == latte::VGT_INDEX_TYPE::INDEX_16) {
      numWords = (count + 1) / 2;
   } else if (index_type == latte::VGT_INDEX_TYPE::INDEX_32) {
      numWords = count;
   } else {
      decaf_abort(fmt::format("Invalid index_type {}", index_type));
   }

   if (indexType == GX2IndexType::U16_LE) {
      internal::writePM4(latte::pm4::DrawIndexImmdWriteOnly16LE {
         count,
         vgt_draw_initiator,
         gsl::make_span(reinterpret_cast<uint16_t *>(indices.getRawPointer()), count)
      });
   } else {
      internal::writePM4(latte::pm4::DrawIndexImmd {
         count,
         vgt_draw_initiator,
         gsl::make_span(reinterpret_cast<uint32_t *>(indices.getRawPointer()), numWords)
      });
   }
}

void
GX2DrawStreamOut(GX2PrimitiveMode mode,
                 virt_ptr<GX2OutputStream> buffer)
{
   internal::writePM4(latte::pm4::SetControlConstant {
      latte::Register::SQ_VTX_BASE_VTX_LOC,
      0u
   });

   internal::writePM4(latte::pm4::SetConfigReg {
      latte::Register::VGT_PRIMITIVE_TYPE,
      mode & GX2PrimitiveModeFlags::ModeMask
   });

   internal::writePM4(latte::pm4::NumInstances {
      0
   });

   auto stride = 0u;
   if (buffer->buffer) {
      stride = buffer->stride;
   } else {
      stride = buffer->gx2rData.elemSize;
   }

   internal::writePM4(latte::pm4::SetContextReg {
      latte::Register::VGT_STRMOUT_DRAW_OPAQUE_VERTEX_STRIDE,
      stride >> 2
   });

   internal::writePM4(latte::pm4::CopyDw {
      latte::pm4::COPY_DW_SELECT::get(0)
         .SRC(latte::pm4::COPY_DW_SEL_MEMORY)
         .DST(latte::pm4::COPY_DW_SEL_REGISTER),
      OSEffectiveToPhysical(virt_cast<virt_addr>(buffer->context)),
      0u,
      latte::Register::VGT_STRMOUT_DRAW_OPAQUE_BUFFER_FILLED_SIZE,
      0u
   });

   auto vgt_draw_initiator = latte::VGT_DRAW_INITIATOR::get(0)
      .SOURCE_SELECT(latte::VGT_DI_SRC_SEL::AUTO_INDEX)
      .USE_OPAQUE(true);

   if (mode & GX2PrimitiveModeFlags::Tessellate) {
      internal::writePM4(latte::pm4::IndexType {
         latte::VGT_DMA_INDEX_TYPE::get(0)
            .INDEX_TYPE(latte::VGT_INDEX_TYPE::INDEX_32)
            .SWAP_MODE(latte::VGT_DMA_SWAP::SWAP_32_BIT)
      });

      vgt_draw_initiator = vgt_draw_initiator
         .MAJOR_MODE(latte::VGT_DI_MAJOR_MODE::MODE1);
   }

   // TODO: This type3 packet should have the predicate bool set to true
   internal::writePM4(latte::pm4::DrawIndexAuto {
      0u,
      vgt_draw_initiator
   });
}

void
GX2SetPrimitiveRestartIndex(uint32_t index)
{
   internal::writePM4(latte::pm4::SetContextReg {
      latte::Register::VGT_MULTI_PRIM_IB_RESET_INDX,
      index
   });
}

void
Library::registerDrawSymbols()
{
   RegisterFunctionExport(GX2SetAttribBuffer);
   RegisterFunctionExport(GX2DrawEx);
   RegisterFunctionExport(GX2DrawEx2);
   RegisterFunctionExport(GX2DrawIndexedEx);
   RegisterFunctionExport(GX2DrawIndexedEx2);
   RegisterFunctionExport(GX2DrawIndexedImmediateEx);
   RegisterFunctionExport(GX2DrawStreamOut);
   RegisterFunctionExport(GX2SetPrimitiveRestartIndex);
}

} // namespace cafe::gx2
