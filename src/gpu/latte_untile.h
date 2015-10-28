#pragma once
#include <cstdint>
#include <vector>

struct GX2Surface;

void
untileSurface2(const GX2Surface *surface, uint8_t *imageData, std::vector<uint8_t> &out, uint32_t &pitchOut);
