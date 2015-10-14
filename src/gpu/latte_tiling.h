#pragma once
#include <cstdint>
#include <vector>
#include "modules/gx2/gx2_surface.h"

void untileSurface(GX2Surface *surface, std::vector<uint8_t> &data, uint32_t &rowPitch);
