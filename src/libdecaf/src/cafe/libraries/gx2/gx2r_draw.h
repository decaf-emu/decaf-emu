#pragma once
#include "gx2_enum.h"
#include <libcpu/be2_struct.h>

namespace cafe::gx2
{

/**
 * \defgroup gx2r_draw GX2R Draw
 * \ingroup gx2
 * @{
 */

struct GX2RBuffer;

void
GX2RSetAttributeBuffer(virt_ptr<GX2RBuffer> buffer,
                       uint32_t index,
                       uint32_t stride,
                       uint32_t offset);

void
GX2RDrawIndexed(GX2PrimitiveMode mode,
                virt_ptr<GX2RBuffer> buffer,
                GX2IndexType indexType,
                uint32_t count,
                uint32_t indexOffset,
                uint32_t vertexOffset,
                uint32_t numInstances);

/** @{ */

} // namespace cafe::gx2
