#include "gx2_draw.h"
#include "gpu/pm4_writer.h"

void
GX2SetAttribBuffer(uint32_t index,
                   uint32_t size,
                   uint32_t stride,
                   void *buffer)
{
   pm4::SetResourceAttrib attrib;
   memset(&attrib, 0, sizeof(pm4::SetResourceAttrib));
   attrib.id = (index * 7) + 0x8c0;
   attrib.baseAddress = buffer;
   attrib.size = size;
   attrib.word2.STRIDE = stride;
   attrib.word6.TYPE = latte::SQ_TEX_VTX_VALID_BUFFER;
   pm4::write(attrib);
}

void
GX2DrawEx(GX2PrimitiveMode::Value mode,
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
GX2DrawIndexedEx(GX2PrimitiveMode::Value mode,
                 uint32_t numVertices,
                 GX2IndexType::Value indexType,
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
