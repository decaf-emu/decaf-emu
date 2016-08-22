#include "gx2r_shaders.h"
#include "gx2_shaders.h"

namespace gx2
{

void
GX2RSetVertexUniformBlock(GX2RBuffer* buffer, uint32_t location, uint32_t offset)
{
   auto bufferSize = buffer->elemSize * buffer->elemCount;
   GX2SetVertexUniformBlock(location, bufferSize - offset, static_cast<uint8_t*>(buffer->buffer.get()) + offset);
}

void
GX2RSetPixelUniformBlock(GX2RBuffer* buffer, uint32_t location, uint32_t offset)
{
   auto bufferSize = buffer->elemSize * buffer->elemCount;
   GX2SetPixelUniformBlock(location, bufferSize - offset, static_cast<uint8_t*>(buffer->buffer.get()) + offset);
}

void
GX2RSetGeometryUniformBlock(GX2RBuffer* buffer, uint32_t location, uint32_t offset)
{
   auto bufferSize = buffer->elemSize * buffer->elemCount;
   GX2SetGeometryUniformBlock(location, bufferSize - offset, static_cast<uint8_t*>(buffer->buffer.get()) + offset);
}

} // namespace gx2
