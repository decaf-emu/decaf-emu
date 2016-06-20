#pragma once
#include "common/types.h"
#include "gx2_enum.h"
#pragma once
#include "gx2r_buffer.h"

namespace gx2
{

void
GX2RSetAttributeBuffer(GX2RBuffer *buffer, uint32_t index, uint32_t stride, uint32_t offset);

} // namespace gx2
