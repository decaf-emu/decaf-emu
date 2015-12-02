#include "gx2_draw.h"
#include "gpu/pm4_writer.h"

void
GX2SetAttribBuffer(uint32_t index,
                   uint32_t size,
                   uint32_t stride,
                   void *buffer)
{
   pm4::SetVtxResource res;
   memset(&res, 0, sizeof(pm4::SetVtxResource));
   res.id = (index * 7) + latte::SQ_VS_ATTRIB_RESOURCE_0;
   res.baseAddress = buffer;
   res.size = size - 1;
   res.word2.STRIDE = stride;
   res.word6.TYPE = latte::SQ_TEX_VTX_VALID_BUFFER;
   pm4::write(res);
}

void
GX2DrawEx(GX2PrimitiveMode mode,
          uint32_t numVertices,
          uint32_t offset,
          uint32_t numInstances)
{
   pm4::write(pm4::SetControlConstant { latte::Register::SQ_VTX_BASE_VTX_LOC, offset });
   pm4::write(pm4::SetConfigReg { latte::Register::VGT_PRIMITIVE_TYPE, mode });
   pm4::write(pm4::IndexType { GX2IndexType::U32 });
   pm4::write(pm4::NumInstances { numInstances });
   pm4::write(pm4::DrawIndexAuto { numVertices, 0 });
}

void
GX2DrawIndexedEx(GX2PrimitiveMode mode,
                 uint32_t numVertices,
                 GX2IndexType indexType,
                 void *indices,
                 uint32_t offset,
                 uint32_t numInstances)
{
   pm4::write(pm4::SetControlConstant { latte::Register::SQ_VTX_BASE_VTX_LOC, offset });
   pm4::write(pm4::SetConfigReg { latte::Register::VGT_PRIMITIVE_TYPE, mode });
   pm4::write(pm4::IndexType { indexType });
   pm4::write(pm4::NumInstances { numInstances });
   pm4::write(pm4::DrawIndex2 { static_cast<uint32_t>(-1), indices, numVertices, 0 });
}

void
GX2SetPrimitiveRestartIndex(uint32_t index)
{
   pm4::write(pm4::SetContextReg { latte::Register::VGT_MULTI_PRIM_IB_RESET_INDX, index });
}
