#include <algorithm>
#include <cstdint>
#include <exception>
#include <vector>
#include "modules/gx2/gx2_format.h"
#include "modules/gx2/gx2_surface.h"
#include "latte_untile.h"
#include "utils/bitutils.h"
#include "utils/align.h"

template<typename Type>
constexpr Type integral_log2(Type n, Type p = 0)
{
   return (n <= 1) ? p : integral_log2(n / 2, p + 1);
}

struct TileInfo
{
   GX2SurfaceFormat::Value surfaceFormat;
   GX2TileMode::Value tileMode;
   size_t numSamples;
   size_t curSample;
   size_t pitchElements;
   size_t width;
   size_t height;
   size_t swizzle;
   size_t elementBytes;
   size_t tileThickness;
   bool isDepthTexture;
   bool isStencilTexture;
};

namespace latte
{

static inline bool
GX2TileModeNeedsChannelRotation(GX2TileMode::Value mode)
{
   switch (mode) {
   case GX2TileMode::Tiled2DThin1:
   case GX2TileMode::Tiled2DThick:
   case GX2TileMode::Tiled2BThin1:
   case GX2TileMode::Tiled2BThick:
   case GX2TileMode::Tiled2DThin2:
   case GX2TileMode::Tiled2BThin2:
   case GX2TileMode::Tiled2DThin4:
   case GX2TileMode::Tiled2BThin4:
      return false;
   case GX2TileMode::Tiled3DThin1:
   case GX2TileMode::Tiled3DThick:
   case GX2TileMode::Tiled3BThin1:
   case GX2TileMode::Tiled3BThick:
      return true;
   default:
      throw std::logic_error("Unexpected GX2TileMode");
      return false;
   }
}

static inline bool
GX2TileModeNeedsBankSwapping(GX2TileMode::Value mode)
{
   switch (mode) {
   case GX2TileMode::Tiled2DThin1:
   case GX2TileMode::Tiled2DThick:
   case GX2TileMode::Tiled2DThin2:
   case GX2TileMode::Tiled2DThin4:
   case GX2TileMode::Tiled3DThin1:
   case GX2TileMode::Tiled3DThick:
      return false;
   case GX2TileMode::Tiled2BThin1:
   case GX2TileMode::Tiled2BThick:
   case GX2TileMode::Tiled2BThin2:
   case GX2TileMode::Tiled2BThin4:
   case GX2TileMode::Tiled3BThin1:
   case GX2TileMode::Tiled3BThick:
      return true;
   default:
      throw std::logic_error("Unexpected GX2TileMode");
      return false;
   }
}

static inline size_t
get_pixel_number(const TileInfo &info, size_t x, size_t y, size_t z)
{
   size_t pixel_number = 0;

   if (info.isDepthTexture) {
      // Depth textures have a special pixel number
      pixel_number |= get_bit<0>(x) << 0; // pn[0] = x[0]
      pixel_number |= get_bit<0>(y) << 1; // pn[1] = y[0]
      pixel_number |= get_bit<1>(x) << 2; // pn[2] = x[1]
      pixel_number |= get_bit<1>(y) << 3; // pn[3] = y[1]
      pixel_number |= get_bit<2>(x) << 4; // pn[4] = x[2]
      pixel_number |= get_bit<2>(y) << 5; // pn[5] = y[2]
   } else {
      switch (info.elementBytes) {
      case 1:
         pixel_number |= get_bit<0>(x) << 0; // pn[0] = x[0]
         pixel_number |= get_bit<1>(x) << 1; // pn[1] = x[1]
         pixel_number |= get_bit<2>(x) << 2; // pn[2] = x[2]
         pixel_number |= get_bit<1>(y) << 3; // pn[3] = y[1]
         pixel_number |= get_bit<0>(y) << 4; // pn[4] = y[0]
         pixel_number |= get_bit<2>(y) << 5; // pn[5] = y[2]
         break;
      case 2:
         pixel_number |= get_bit<0>(x) << 0; // pn[0] = x[0]
         pixel_number |= get_bit<1>(x) << 1; // pn[1] = x[1]
         pixel_number |= get_bit<2>(x) << 2; // pn[2] = x[2]
         pixel_number |= get_bit<0>(y) << 3; // pn[3] = y[0]
         pixel_number |= get_bit<1>(y) << 4; // pn[4] = y[1]
         pixel_number |= get_bit<2>(y) << 5; // pn[5] = y[2]
         break;
      case 4:
         pixel_number |= get_bit<0>(x) << 0; // pn[0] = x[0]
         pixel_number |= get_bit<1>(x) << 1; // pn[1] = x[1]
         pixel_number |= get_bit<0>(y) << 2; // pn[2] = y[0]
         pixel_number |= get_bit<2>(x) << 3; // pn[3] = x[2]
         pixel_number |= get_bit<1>(y) << 4; // pn[4] = y[1]
         pixel_number |= get_bit<2>(y) << 5; // pn[5] = y[2]
         break;
      case 8:
         pixel_number |= get_bit<0>(x) << 0; // pn[0] = x[0]
         pixel_number |= get_bit<0>(y) << 1; // pn[1] = y[0]
         pixel_number |= get_bit<1>(x) << 2; // pn[2] = x[1]
         pixel_number |= get_bit<2>(x) << 3; // pn[3] = x[2]
         pixel_number |= get_bit<1>(y) << 4; // pn[4] = y[1]
         pixel_number |= get_bit<2>(y) << 5; // pn[5] = y[2]
         break;
      case 16:
         pixel_number |= get_bit<0>(y) << 0; // pn[0] = y[0]
         pixel_number |= get_bit<0>(x) << 1; // pn[1] = x[0]
         pixel_number |= get_bit<1>(x) << 2; // pn[2] = x[1]
         pixel_number |= get_bit<2>(x) << 3; // pn[3] = x[2]
         pixel_number |= get_bit<1>(y) << 4; // pn[4] = y[1]
         pixel_number |= get_bit<2>(y) << 5; // pn[5] = y[2]
         break;
      default:
         throw std::logic_error("Invalid surface format bytes per element");
      }
   }

   // If tile is thick
   if (info.tileThickness == 4) {
      pixel_number |= get_bit<0>(z) << 6; // pn[6] = z[0]
      pixel_number |= get_bit<1>(z) << 7; // pn[7] = z[1]
   }

   return pixel_number;
}

static inline size_t
get_element_offset(const TileInfo &info, size_t pixel_number, size_t tile_bytes)
{
   static const size_t stencil_data_size = 1;
   static const size_t depth_data_size = 3;
   auto element_bytes = info.elementBytes;
   auto num_samples = info.numSamples;
   auto sample_number = info.curSample;
   auto element_offset = static_cast<size_t>(0);

   if (!info.isDepthTexture) {
      auto sample_offset = sample_number * (tile_bytes / num_samples);
      element_offset = sample_offset + (pixel_number * element_bytes);
   } else if (element_bytes == 2) {
      auto pixel_offset = pixel_number * element_bytes * num_samples;
      element_offset = pixel_offset + (sample_number * element_bytes);
   } else if (element_bytes == 4) {
      size_t pixel_offset;

      if (info.isStencilTexture) {
         pixel_offset = pixel_number * stencil_data_size;
      } else {
         pixel_offset = (8 * 8 * stencil_data_size) + pixel_number * depth_data_size;
      }

      element_offset = pixel_offset + (sample_number * element_bytes);
   } else {
      throw std::logic_error("Unexpected depth texture element_bytes");
   }

   return element_offset;
}

static inline size_t
get_channel(size_t x, size_t y)
{
   size_t channel = 0;
   //channel[0] = x[3] ^ y[3]
   channel |= (get_bit<3>(x) ^ get_bit<3>(y)) << 0;
   return channel;
}

static inline size_t
get_bank(size_t x, size_t y)
{
   size_t bank = 0;
   //bank[0] = x[3] ^ y[4 + log2(num_channels)]
   bank |= (get_bit<3>(x) ^ get_bit(y, 4 + channel_bits)) << 0;

   //bank[1] = x[4] ^ y[3 + log2(num_channels)]
   bank |= (get_bit<4>(x) ^ get_bit(y, 3 + channel_bits)) << 1;
   return bank;
}

size_t
get_1d_offset(const TileInfo &info, size_t x, size_t y, size_t z)
{
   // Setup vars
   auto tile_thickness = info.tileThickness;
   auto element_bytes = info.elementBytes;
   auto num_samples = info.numSamples;
   auto pitch_elements = info.pitchElements;
   auto height = info.height;
   auto sample_number = info.curSample;
   auto pixel_number = get_pixel_number(info, x, y, z);

   // Fancy computations
   auto tile_bytes = tile_width * tile_height * tile_thickness * element_bytes * num_samples;
   auto tiles_per_row = pitch_elements / tile_width;
   auto tiles_per_slice = tiles_per_row * (height / tile_height);
   auto slice_offset = (z / tile_thickness) * tiles_per_slice * tile_bytes;

   auto tile_row_index = y / tile_height;
   auto tile_column_index = x / tile_width;
   auto tile_offset = ((tile_row_index * tiles_per_row) + tile_column_index) * tile_bytes;

   auto element_offset = get_element_offset(info, pixel_number, tile_bytes);
   auto offset = slice_offset + tile_offset + element_offset;

   return offset;
}

size_t
get_2d_offset(const TileInfo &info, size_t x, size_t y, size_t z)
{
   // Setup vars
   auto tile_thickness = info.tileThickness;
   auto element_bytes = info.elementBytes;
   auto num_samples = info.numSamples;
   auto pitch_elements = align_up(info.pitchElements, 32);
   auto height = info.height;
   auto sample_number = info.curSample;
   auto pixel_number = get_pixel_number(info, x, y, z);

   auto macro_tile_size = GX2GetMacroTileSize(info.tileMode);
   auto macro_tile_width = macro_tile_size.first;
   auto macro_tile_height = macro_tile_size.second;

   // Fancy calculations
   auto tile_bytes = tile_width * tile_height * tile_thickness * element_bytes * num_samples;
   auto macro_tile_bytes = macro_tile_width * macro_tile_height * tile_bytes;
   auto macro_tiles_per_row = pitch_elements / tile_width / macro_tile_width;
   auto macro_tiles_per_slice = macro_tiles_per_row * (height / tile_height / macro_tile_height);
   auto slice_offset = (z / tile_thickness) * macro_tiles_per_slice * macro_tile_bytes;

   auto macro_tile_row_index = y / tile_height / macro_tile_height;
   auto macro_tile_column_index = x / tile_width / macro_tile_width;
   auto macro_tile_offset = ((macro_tile_row_index * macro_tiles_per_row) + macro_tile_column_index) * macro_tile_bytes;

   auto element_offset = get_element_offset(info, pixel_number, tile_bytes);

   auto total_offset = (slice_offset + macro_tile_offset) >> (channel_bits + bank_bits);
   total_offset = total_offset + element_offset;

   // Calculate bank & channel
   size_t bank_slice_rotation = 0;
   size_t bank_swap_rotation = 0;
   size_t channel_slice_rotation = 0;
   size_t sample_slice_rotation = 0;

   auto do_channel_rotate = GX2TileModeNeedsChannelRotation(info.tileMode);
   auto do_bank_swapping = GX2TileModeNeedsBankSwapping(info.tileMode);

   // All formats do bank rotation
   auto slice = z / tile_thickness;
   sample_slice_rotation = (((num_banks / 2) + 1) * sample_number);

   if (do_channel_rotate) {
      // 3D & 3B formats
      bank_slice_rotation = (std::max(1ull, (num_channels / 2) - 1) * slice) / num_channels;
      channel_slice_rotation = std::max(1ull, (num_channels / 2) - 1) * slice;
   } else {
      // 2D & 2B formats
      bank_slice_rotation = (((num_banks / 2) - 1) * slice);
   }

   if (do_bank_swapping) {
      auto macro_tile_index = ((macro_tile_row_index * macro_tiles_per_row) + macro_tile_column_index);
      auto bank_swap_interval = (element_bytes * 8 * 2 / bank_swap_bytes);
      bank_swap_rotation = (macro_tile_index / bank_swap_interval) % num_banks;
   }

   auto channel_swizzle = (info.swizzle >> 8) & 1;
   auto bank_swizzle = (info.swizzle >> 9) & 3;

   auto bank = get_bank(x, y);
   bank ^= (bank_swizzle + bank_slice_rotation) & (num_banks - 1);
   bank ^= sample_slice_rotation;
   bank ^= bank_swap_rotation;

   auto channel = get_channel(x, y);
   channel ^= (channel_swizzle + channel_slice_rotation) & (num_channels - 1);

   // Final offset
   auto group_mask = (1ull << group_bits) - 1;
   auto offset_low = total_offset & group_mask;
   auto offset_high = (total_offset & ~group_mask) << (channel_bits + bank_bits);
   auto offset = (bank << (group_bits + channel_bits)) + (channel << group_bits) + offset_low + offset_high;

   return offset;
}

void
untileSurface(const GX2Surface *surface, std::vector<uint8_t> &out, size_t &pitchOut)
{
   if (surface->tileMode == GX2TileMode::LinearAligned) {
      out.resize(surface->imageSize);
      memcpy(&out[0], surface->image, surface->imageSize);
      pitchOut = surface->pitch;
      return;
   }

   if (surface->tileMode == GX2TileMode::LinearSpecial) {
      throw std::runtime_error("Unsupported tile mode LinearSpecial");
   }

   if (surface->tileMode == GX2TileMode::Default) {
      throw std::runtime_error("Unsupported tile mode Default");
   }

   auto blockSize = GX2GetSurfaceBlockSize(surface->format);
   auto bpe = GX2GetSurfaceElementBytes(surface->format);
   auto surfaceWidth = align_up<uint32_t>(surface->width, blockSize.first);
   auto surfaceHeight = align_up<uint32_t>(surface->height, blockSize.second);

   // Setup tiling info
   TileInfo info;
   info.surfaceFormat = surface->format;
   info.tileMode = surface->tileMode;
   info.numSamples = 1; // Used when multisampling
   info.curSample = 0;
   info.pitchElements = surfaceWidth / blockSize.first;
   info.width = surfaceWidth / blockSize.first;
   info.height = surfaceHeight / blockSize.second;
   info.swizzle = surface->swizzle;
   info.isDepthTexture = false;
   info.isStencilTexture = false;
   info.elementBytes = bpe;
   info.tileThickness = GX2GetTileThickness(surface->tileMode);

   // Setup dst
   out.resize(surface->imageSize);

   auto src = reinterpret_cast<uint8_t*>(surface->image.get());
   auto dst = out.data();
   auto pitch = info.pitchElements * bpe;

   if (info.tileMode == GX2TileMode::Tiled1DThick || info.tileMode == GX2TileMode::Tiled1DThin1) {
      for (size_t y = 0; y < info.height; ++y) {
         for (size_t x = 0; x < info.width; ++x) {
            auto offset = get_1d_offset(info, x, y, 0);
            std::memcpy(dst + (y * pitch) + (x * bpe), src + offset, bpe);
         }
      }
   } else {
      for (size_t y = 0; y < info.height; ++y) {
         for (size_t x = 0; x < info.width; ++x) {
            auto offset = get_2d_offset(info, x, y, 0);
            std::memcpy(dst + (y * pitch) + (x * bpe), src + offset, bpe);
         }
      }
   }

   pitchOut = pitch;
}

}
