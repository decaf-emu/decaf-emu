#include "gx2_draw.h"
#include "gx2r_draw.h"

namespace gx2
{

void
GX2RSetAttributeBuffer(GX2RBuffer *buffer, uint32_t index, uint32_t stride, uint32_t offset)
{
   auto bufferPtr = static_cast<uint8_t*>(buffer->buffer.get()) + offset;
   auto bufferSize = buffer->elemCount * buffer->elemSize;
   GX2SetAttribBuffer(index, bufferSize, stride, bufferPtr);
}

} // namespace gx2
