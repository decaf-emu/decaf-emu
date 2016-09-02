#pragma once
#include "gx2r_buffer.h"

namespace gx2
{

void
GX2RSetVertexUniformBlock(GX2RBuffer* buffer,
                          uint32_t location,
                          uint32_t offset);

void
GX2RSetPixelUniformBlock(GX2RBuffer* buffer,
                         uint32_t location,
                         uint32_t offset);

void
GX2RSetGeometryUniformBlock(GX2RBuffer* buffer,
                            uint32_t location,
                            uint32_t offset);

} // namespace gx2
