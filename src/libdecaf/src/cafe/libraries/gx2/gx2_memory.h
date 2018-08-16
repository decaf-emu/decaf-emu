#pragma once
#include "gx2_enum.h"
#include <libcpu/be2_struct.h>

namespace cafe::gx2
{

void
GX2Invalidate(GX2InvalidateMode mode,
              virt_ptr<void> buffer,
              uint32_t size);

} // namespace cafe::gx2
