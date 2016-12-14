#pragma once
#include "gx2_enum.h"
#include "gx2r_buffer.h"
#include "common/types.h"

namespace gx2
{

void
GX2RBeginDisplayListEx(GX2RBuffer *displayList,
                       uint32_t unused,
                       GX2RResourceFlags flags);

uint32_t
GX2REndDisplayList(GX2RBuffer *displayList);

void
GX2RCallDisplayList(GX2RBuffer *displayList,
                    uint32_t size);

void
GX2RDirectCallDisplayList(GX2RBuffer *displayList,
                          uint32_t size);

} // namespace gx2
