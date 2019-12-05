#pragma once
#include "gpu7_tiling.h"

namespace gpu7::tiling::cpu
{

void
untile(const RetileInfo& desc,
       uint8_t* untiled,
       uint8_t* tiled,
       uint32_t firstSlice,
       uint32_t numSlices);

void
tile(const RetileInfo& desc,
     uint8_t* untiled,
     uint8_t* tiled,
     uint32_t firstSlice,
     uint32_t numSlices);

}  // namespace gpu7::tiling::cpu
