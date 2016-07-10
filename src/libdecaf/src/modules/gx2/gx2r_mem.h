#pragma once
#include <cstdint>
#include "gx2_enum.h"

namespace gx2
{

void
GX2RInvalidateMemory(GX2RResourceFlags flags,
                     void *buffer,
                     uint32_t size);

} // namespace gx2
