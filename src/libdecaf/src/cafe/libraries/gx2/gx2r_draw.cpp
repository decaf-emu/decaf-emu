#include "gx2.h"
#include "gx2_draw.h"
#include "gx2r_buffer.h"
#include "gx2r_draw.h"

namespace cafe::gx2
{

void
GX2RSetAttributeBuffer(virt_ptr<GX2RBuffer> buffer,
                       uint32_t index,
                       uint32_t stride,
                       uint32_t offset)
{
   GX2SetAttribBuffer(index,
                      buffer->elemCount * buffer->elemSize,
                      stride,
                      virt_cast<uint8_t*>(buffer->buffer) + offset);
}

void
GX2RDrawIndexed(GX2PrimitiveMode mode,
                virt_ptr<GX2RBuffer> buffer,
                GX2IndexType indexType,
                uint32_t count,
                uint32_t indexOffset,
                uint32_t vertexOffset,
                uint32_t numInstances)
{
   GX2DrawIndexedEx(mode,
                    count,
                    indexType,
                    virt_cast<uint8_t *>(buffer->buffer) + indexOffset * buffer->elemSize,
                    vertexOffset,
                    numInstances);
}

void
Library::registerGx2rDrawSymbols()
{
   RegisterFunctionExport(GX2RSetAttributeBuffer);
   RegisterFunctionExport(GX2RDrawIndexed);
}

} // namespace cafe::gx2
