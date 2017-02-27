#pragma once
#include "gx2_enum.h"
#include <cstdint>

namespace gx2
{

void
GX2Invalidate(GX2InvalidateMode mode,
              void *buffer,
              uint32_t size);

} // namespace gx2
