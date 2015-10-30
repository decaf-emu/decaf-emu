#pragma once
#include <cstdint>
#include <vector>

struct GX2Surface;

namespace latte
{

static const size_t num_channels = 2;
static const size_t num_banks = 4;
static const size_t group_bytes = 256;
static const size_t row_bytes = 2 * 1024;
static const size_t bank_swap_bytes = 256;
static const size_t sample_split_bytes = 2 * 1024;
static const size_t channel_bits = 1; // log2(num_channels)
static const size_t bank_bits = 2; // log2(num_banks)
static const size_t group_bits = 8; // log2(group_bytes)
static const size_t tile_width = 8;
static const size_t tile_height = 8;

void
untileSurface(const GX2Surface *surface, std::vector<uint8_t> &out, size_t &pitchOut);

void
untileSurface(const GX2Surface *surface, const uint8_t *imageData, std::vector<uint8_t> &out, size_t &pitchOut);

}
