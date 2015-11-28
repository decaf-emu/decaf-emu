#pragma once
#include <stdexcept>
#include <utility>
#include <vector>
#include "latte_enum_sq.h"

namespace latte
{

static const size_t num_channels       = 2;
static const size_t num_banks          = 4;
static const size_t group_bytes        = 256;
static const size_t row_bytes          = 2 * 1024;
static const size_t bank_swap_bytes    = 256;
static const size_t sample_split_bytes = 2 * 1024;
static const size_t channel_bits       = 1;        // log2(num_channels)
static const size_t bank_bits          = 2;        // log2(num_banks)
static const size_t group_bits         = 8;        // log2(group_bytes)
static const size_t tile_width         = 8;
static const size_t tile_height        = 8;

bool
untile(const uint8_t *image, size_t width, size_t height, size_t depth, size_t pitch, latte::SQ_DATA_FORMAT format, latte::SQ_TILE_MODE tileMode, uint32_t swizzle, std::vector<uint8_t> &out);

inline std::pair<size_t, size_t>
macroTileSize(latte::SQ_TILE_MODE mode)
{
   switch (mode) {
   case latte::SQ_TILE_MODE_TILED_2D_THIN1:
   case latte::SQ_TILE_MODE_TILED_2D_THICK:
   case latte::SQ_TILE_MODE_TILED_2B_THIN1:
   case latte::SQ_TILE_MODE_TILED_2B_THICK:
   case latte::SQ_TILE_MODE_TILED_3D_THIN1:
   case latte::SQ_TILE_MODE_TILED_3D_THICK:
   case latte::SQ_TILE_MODE_TILED_3B_THIN1:
   case latte::SQ_TILE_MODE_TILED_3B_THICK:
      return { latte::num_banks, latte::num_channels };
   case latte::SQ_TILE_MODE_TILED_2D_THIN2:
   case latte::SQ_TILE_MODE_TILED_2B_THIN2:
      return { latte::num_banks / 2, latte::num_channels * 2 };
   case latte::SQ_TILE_MODE_TILED_2D_THIN4:
   case latte::SQ_TILE_MODE_TILED_2B_THIN4:
      return { latte::num_banks / 4, latte::num_channels * 4 };
   default:
      throw std::logic_error("Unexpected tile mode.");
      return { 0, 0 };
   }
}

inline bool
tileModeNeedsChannelRotation(latte::SQ_TILE_MODE mode)
{
   switch (mode) {
   case latte::SQ_TILE_MODE_TILED_2D_THIN1:
   case latte::SQ_TILE_MODE_TILED_2D_THICK:
   case latte::SQ_TILE_MODE_TILED_2B_THIN1:
   case latte::SQ_TILE_MODE_TILED_2B_THICK:
   case latte::SQ_TILE_MODE_TILED_2D_THIN2:
   case latte::SQ_TILE_MODE_TILED_2B_THIN2:
   case latte::SQ_TILE_MODE_TILED_2D_THIN4:
   case latte::SQ_TILE_MODE_TILED_2B_THIN4:
      return false;
   case latte::SQ_TILE_MODE_TILED_3D_THIN1:
   case latte::SQ_TILE_MODE_TILED_3D_THICK:
   case latte::SQ_TILE_MODE_TILED_3B_THIN1:
   case latte::SQ_TILE_MODE_TILED_3B_THICK:
      return true;
   default:
      throw std::logic_error("Unexpected tile mode.");
      return false;
   }
}

inline bool
tileModeNeedsBankSwapping(latte::SQ_TILE_MODE mode)
{
   switch (mode) {
   case latte::SQ_TILE_MODE_TILED_2D_THIN1:
   case latte::SQ_TILE_MODE_TILED_2D_THICK:
   case latte::SQ_TILE_MODE_TILED_2D_THIN2:
   case latte::SQ_TILE_MODE_TILED_2D_THIN4:
   case latte::SQ_TILE_MODE_TILED_3D_THIN1:
   case latte::SQ_TILE_MODE_TILED_3D_THICK:
      return false;
   case latte::SQ_TILE_MODE_TILED_2B_THIN1:
   case latte::SQ_TILE_MODE_TILED_2B_THICK:
   case latte::SQ_TILE_MODE_TILED_2B_THIN2:
   case latte::SQ_TILE_MODE_TILED_2B_THIN4:
   case latte::SQ_TILE_MODE_TILED_3B_THIN1:
   case latte::SQ_TILE_MODE_TILED_3B_THICK:
      return true;
   default:
      throw std::logic_error("Unexpected tile mode.");
      return false;
   }
}

inline size_t
tileThickness(latte::SQ_TILE_MODE mode)
{
   switch (mode) {
   case latte::SQ_TILE_MODE_LINEAR_ALIGNED:
   case latte::SQ_TILE_MODE_LINEAR_SPECIAL:
   case latte::SQ_TILE_MODE_TILED_1D_THIN1:
   case latte::SQ_TILE_MODE_TILED_2D_THIN1:
   case latte::SQ_TILE_MODE_TILED_2D_THIN2:
   case latte::SQ_TILE_MODE_TILED_2D_THIN4:
   case latte::SQ_TILE_MODE_TILED_2B_THIN1:
   case latte::SQ_TILE_MODE_TILED_2B_THIN2:
   case latte::SQ_TILE_MODE_TILED_2B_THIN4:
   case latte::SQ_TILE_MODE_TILED_3D_THIN1:
   case latte::SQ_TILE_MODE_TILED_3B_THIN1:
      return 1;
   case latte::SQ_TILE_MODE_TILED_1D_THICK:
   case latte::SQ_TILE_MODE_TILED_2D_THICK:
   case latte::SQ_TILE_MODE_TILED_2B_THICK:
   case latte::SQ_TILE_MODE_TILED_3D_THICK:
   case latte::SQ_TILE_MODE_TILED_3B_THICK:
      return 4;
   default:
      throw std::runtime_error("Unexpected tile mode.");
      return 0;
   }
}

size_t
formatBytesPerElement(latte::SQ_DATA_FORMAT format);

} // namespace latte
