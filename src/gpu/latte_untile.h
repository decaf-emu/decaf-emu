#pragma once
#include <cstdint>
#include <vector>

struct GX2Surface;

void
untileSurface(const GX2Surface *surface, std::vector<uint8_t> &out, uint32_t &pitchOut);
