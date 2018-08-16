#include "gx2.h"
#include "gx2r_shaders.h"
#include "gx2_shaders.h"

namespace cafe::gx2
{

void
GX2RSetVertexUniformBlock(virt_ptr<GX2RBuffer> buffer,
                          uint32_t location,
                          uint32_t offset)
{
   GX2SetVertexUniformBlock(location,
                            (buffer->elemSize * buffer->elemCount) - offset,
                            virt_cast<uint8_t *>(buffer->buffer) + offset);
}

void
GX2RSetPixelUniformBlock(virt_ptr<GX2RBuffer> buffer,
                         uint32_t location,
                         uint32_t offset)
{
   GX2SetPixelUniformBlock(location,
                           (buffer->elemSize * buffer->elemCount) - offset,
                           virt_cast<uint8_t *>(buffer->buffer) + offset);
}

void
GX2RSetGeometryUniformBlock(virt_ptr<GX2RBuffer> buffer,
                            uint32_t location,
                            uint32_t offset)
{
   GX2SetGeometryUniformBlock(location,
                              (buffer->elemSize * buffer->elemCount) - offset,
                              virt_cast<uint8_t *>(buffer->buffer) + offset);
}

void
Library::registerGx2rShadersSymbols()
{
   RegisterFunctionExport(GX2RSetVertexUniformBlock);
   RegisterFunctionExport(GX2RSetPixelUniformBlock);
   RegisterFunctionExport(GX2RSetGeometryUniformBlock);
}

} // namespace cafe::gx2
