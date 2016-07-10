#pragma once
#include "common/types.h"
#include "gx2_enum.h"
#include "gx2r_buffer.h"

namespace gx2
{

void
GX2RSetAttributeBuffer(GX2RBuffer *buffer,
                       uint32_t index,
                       uint32_t stride,
                       uint32_t offset);

void
GX2RDrawIndexed(GX2PrimitiveMode mode,
                GX2RBuffer *buffer,
                GX2IndexType indexType,
                uint32_t count,
                uint32_t indexOffset,
                uint32_t vertexOffset,
                uint32_t numInstances);

} // namespace gx2
