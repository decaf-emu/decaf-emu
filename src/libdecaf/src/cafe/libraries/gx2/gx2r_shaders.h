#pragma once
#include <libcpu/be2_struct.h>

namespace cafe::gx2
{

/**
 * \defgroup gx2r_shaders GX2R Shaders
 * \ingroup gx2
 * @{
 */

struct GX2RBuffer;

void
GX2RSetVertexUniformBlock(virt_ptr<GX2RBuffer> buffer,
                          uint32_t location,
                          uint32_t offset);

void
GX2RSetPixelUniformBlock(virt_ptr<GX2RBuffer> buffer,
                         uint32_t location,
                         uint32_t offset);

void
GX2RSetGeometryUniformBlock(virt_ptr<GX2RBuffer> buffer,
                            uint32_t location,
                            uint32_t offset);

/** @} */

} // namespace cafe::gx2
