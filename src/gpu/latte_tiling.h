#pragma once
#include <vector>
#include "systemtypes.h"
#include "modules/gx2/gx2_surface.h"

void untileSurface(GX2Surface *surface, std::vector<uint8_t>& data, uint32_t& rowPitch);