#include <algorithm>
#include <cstdint>
#include <exception>
#include <vector>
#include "modules/gx2/gx2_surface.h"
#include "utils/bitutils.h"
#include "utils/align.h"

template<typename Type>
constexpr Type integral_log2(Type n, Type p = 0)
{
   return (n <= 1) ? p : integral_log2(n / 2, p + 1);
}

static const auto num_channels = 2u;
static const auto num_banks = 4u;
static const auto group_bytes = 256u;
static const auto row_bytes = 2 * 1024u;
static const auto bank_swap_bytes = 256u;
static const auto sample_split_bytes = 2 * 1024u;
static const auto channel_bits = integral_log2(num_channels);
static const auto bank_bits = integral_log2(num_banks);
static const auto group_bits = integral_log2(group_bytes);
static const auto tile_width = 8u;
static const auto tile_height = 8u;

struct TileInfo
{
   GX2SurfaceFormat::Value surfaceFormat;
   GX2TileMode::Value tileMode;
   unsigned numSamples;
   unsigned curSample;
   unsigned pitchElements;
   unsigned width;
   unsigned height;
   unsigned swizzle;
   unsigned elementBytes;
   unsigned tileThickness;
   bool isDepthTexture;
   bool isStencilTexture;
};

static inline std::pair<unsigned, unsigned>
GX2GetBlockSize(GX2SurfaceFormat::Value format)
{
   switch (format) {
   case GX2SurfaceFormat::UNORM_BC1:
   case GX2SurfaceFormat::UNORM_BC2:
   case GX2SurfaceFormat::UNORM_BC3:
   case GX2SurfaceFormat::UNORM_BC4:
   case GX2SurfaceFormat::UNORM_BC5:
   case GX2SurfaceFormat::SNORM_BC4:
   case GX2SurfaceFormat::SNORM_BC5:
   case GX2SurfaceFormat::SRGB_BC1:
   case GX2SurfaceFormat::SRGB_BC2:
   case GX2SurfaceFormat::SRGB_BC3:
      return { 4, 4 };

   case GX2SurfaceFormat::UNORM_R4_G4:
   case GX2SurfaceFormat::UNORM_R4_G4_B4_A4:
   case GX2SurfaceFormat::UNORM_R8:
   case GX2SurfaceFormat::UNORM_R8_G8:
   case GX2SurfaceFormat::UNORM_R8_G8_B8_A8:
   case GX2SurfaceFormat::UNORM_R16:
   case GX2SurfaceFormat::UNORM_R16_G16:
   case GX2SurfaceFormat::UNORM_R16_G16_B16_A16:
   case GX2SurfaceFormat::UNORM_R5_G6_B5:
   case GX2SurfaceFormat::UNORM_R5_G5_B5_A1:
   case GX2SurfaceFormat::UNORM_A1_B5_G5_R5:
   case GX2SurfaceFormat::UNORM_R24_X8:
   case GX2SurfaceFormat::UNORM_A2_B10_G10_R10:
   case GX2SurfaceFormat::UNORM_R10_G10_B10_A2:
   case GX2SurfaceFormat::UNORM_NV12:
   case GX2SurfaceFormat::UINT_R8:
   case GX2SurfaceFormat::UINT_R8_G8:
   case GX2SurfaceFormat::UINT_R8_G8_B8_A8:
   case GX2SurfaceFormat::UINT_R16:
   case GX2SurfaceFormat::UINT_R16_G16:
   case GX2SurfaceFormat::UINT_R16_G16_B16_A16:
   case GX2SurfaceFormat::UINT_R32:
   case GX2SurfaceFormat::UINT_R32_G32:
   case GX2SurfaceFormat::UINT_R32_G32_B32_A32:
   case GX2SurfaceFormat::UINT_A2_B10_G10_R10:
   case GX2SurfaceFormat::UINT_R10_G10_B10_A2:
   case GX2SurfaceFormat::UINT_X24_G8:
   case GX2SurfaceFormat::UINT_G8_X24:
   case GX2SurfaceFormat::SNORM_R8:
   case GX2SurfaceFormat::SNORM_R8_G8:
   case GX2SurfaceFormat::SNORM_R8_G8_B8_A8:
   case GX2SurfaceFormat::SNORM_R16:
   case GX2SurfaceFormat::SNORM_R16_G16:
   case GX2SurfaceFormat::SNORM_R16_G16_B16_A16:
   case GX2SurfaceFormat::SNORM_R10_G10_B10_A2:
   case GX2SurfaceFormat::SINT_R8:
   case GX2SurfaceFormat::SINT_R8_G8:
   case GX2SurfaceFormat::SINT_R8_G8_B8_A8:
   case GX2SurfaceFormat::SINT_R16:
   case GX2SurfaceFormat::SINT_R16_G16:
   case GX2SurfaceFormat::SINT_R16_G16_B16_A16:
   case GX2SurfaceFormat::SINT_R32:
   case GX2SurfaceFormat::SINT_R32_G32:
   case GX2SurfaceFormat::SINT_R32_G32_B32_A32:
   case GX2SurfaceFormat::SINT_R10_G10_B10_A2:
   case GX2SurfaceFormat::SRGB_R8_G8_B8_A8:
   case GX2SurfaceFormat::FLOAT_R32:
   case GX2SurfaceFormat::FLOAT_R32_G32:
   case GX2SurfaceFormat::FLOAT_R32_G32_B32_A32:
   case GX2SurfaceFormat::FLOAT_R16:
   case GX2SurfaceFormat::FLOAT_R16_G16:
   case GX2SurfaceFormat::FLOAT_R16_G16_B16_A16:
   case GX2SurfaceFormat::FLOAT_R11_G11_B10:
   case GX2SurfaceFormat::FLOAT_D24_S8:
   case GX2SurfaceFormat::FLOAT_X8_X24:
      return { 1, 1 };

   case GX2SurfaceFormat::INVALID:
   default:
      throw std::runtime_error("Unexpected GX2SurfaceFormat");
      return { 1, 1 };
   }
}

static inline unsigned
GX2GetElementBytes(GX2SurfaceFormat::Value format)
{
   static const auto bc1 = 8;
   static const auto bc2 = 16;
   static const auto bc3 = 16;
   static const auto bc4 = 8;
   static const auto bc5 = 16;

   switch (format) {
   case GX2SurfaceFormat::UNORM_R4_G4:
      return 1;
   case GX2SurfaceFormat::UNORM_R4_G4_B4_A4:
      return 2;
   case GX2SurfaceFormat::UNORM_R8:
      return 1;
   case GX2SurfaceFormat::UNORM_R8_G8:
      return 2;
   case GX2SurfaceFormat::UNORM_R8_G8_B8_A8:
      return 4;
   case GX2SurfaceFormat::UNORM_R16:
      return 2;
   case GX2SurfaceFormat::UNORM_R16_G16:
      return 4;
   case GX2SurfaceFormat::UNORM_R16_G16_B16_A16:
      return 8;
   case GX2SurfaceFormat::UNORM_R5_G6_B5:
      return 2;
   case GX2SurfaceFormat::UNORM_R5_G5_B5_A1:
      return 2;
   case GX2SurfaceFormat::UNORM_A1_B5_G5_R5:
      return 2;
   case GX2SurfaceFormat::UNORM_R24_X8:
      return 4;
   case GX2SurfaceFormat::UNORM_A2_B10_G10_R10:
      return 4;
   case GX2SurfaceFormat::UNORM_R10_G10_B10_A2:
      return 4;
   case GX2SurfaceFormat::UNORM_BC1:
      return bc1;
   case GX2SurfaceFormat::UNORM_BC2:
      return bc2;
   case GX2SurfaceFormat::UNORM_BC3:
      return bc3;
   case GX2SurfaceFormat::UNORM_BC4:
      return bc4;
   case GX2SurfaceFormat::UNORM_BC5:
      return bc5;
   case GX2SurfaceFormat::UNORM_NV12:
      return 0;

   case GX2SurfaceFormat::UINT_R8:
      return 1;
   case GX2SurfaceFormat::UINT_R8_G8:
      return 2;
   case GX2SurfaceFormat::UINT_R8_G8_B8_A8:
      return 4;
   case GX2SurfaceFormat::UINT_R16:
      return 2;
   case GX2SurfaceFormat::UINT_R16_G16:
      return 4;
   case GX2SurfaceFormat::UINT_R16_G16_B16_A16:
      return 8;
   case GX2SurfaceFormat::UINT_R32:
      return 4;
   case GX2SurfaceFormat::UINT_R32_G32:
      return 8;
   case GX2SurfaceFormat::UINT_R32_G32_B32_A32:
      return 16;
   case GX2SurfaceFormat::UINT_A2_B10_G10_R10:
      return 4;
   case GX2SurfaceFormat::UINT_R10_G10_B10_A2:
      return 4;
   case GX2SurfaceFormat::UINT_X24_G8:
      return 4;
   case GX2SurfaceFormat::UINT_G8_X24:
      return 4;

   case GX2SurfaceFormat::SNORM_R8:
      return 1;
   case GX2SurfaceFormat::SNORM_R8_G8:
      return 2;
   case GX2SurfaceFormat::SNORM_R8_G8_B8_A8:
      return 4;
   case GX2SurfaceFormat::SNORM_R16:
      return 2;
   case GX2SurfaceFormat::SNORM_R16_G16:
      return 4;
   case GX2SurfaceFormat::SNORM_R16_G16_B16_A16:
      return 8;
   case GX2SurfaceFormat::SNORM_R10_G10_B10_A2:
      return 4;
   case GX2SurfaceFormat::SNORM_BC4:
      return bc4;
   case GX2SurfaceFormat::SNORM_BC5:
      return bc5;

   case GX2SurfaceFormat::SINT_R8:
      return 1;
   case GX2SurfaceFormat::SINT_R8_G8:
      return 2;
   case GX2SurfaceFormat::SINT_R8_G8_B8_A8:
      return 4;
   case GX2SurfaceFormat::SINT_R16:
      return 2;
   case GX2SurfaceFormat::SINT_R16_G16:
      return 4;
   case GX2SurfaceFormat::SINT_R16_G16_B16_A16:
      return 8;
   case GX2SurfaceFormat::SINT_R32:
      return 4;
   case GX2SurfaceFormat::SINT_R32_G32:
      return 8;
   case GX2SurfaceFormat::SINT_R32_G32_B32_A32:
      return 16;
   case GX2SurfaceFormat::SINT_R10_G10_B10_A2:
      return 4;

   case GX2SurfaceFormat::SRGB_R8_G8_B8_A8:
      return 4;
   case GX2SurfaceFormat::SRGB_BC1:
      return bc1;
   case GX2SurfaceFormat::SRGB_BC2:
      return bc2;
   case GX2SurfaceFormat::SRGB_BC3:
      return bc3;

   case GX2SurfaceFormat::FLOAT_R32:
      return 4;
   case GX2SurfaceFormat::FLOAT_R32_G32:
      return 8;
   case GX2SurfaceFormat::FLOAT_R32_G32_B32_A32:
      return 16;
   case GX2SurfaceFormat::FLOAT_R16:
      return 2;
   case GX2SurfaceFormat::FLOAT_R16_G16:
      return 4;
   case GX2SurfaceFormat::FLOAT_R16_G16_B16_A16:
      return 8;
   case GX2SurfaceFormat::FLOAT_R11_G11_B10:
      return 4;
   case GX2SurfaceFormat::FLOAT_D24_S8:
      return 4;
   case GX2SurfaceFormat::FLOAT_X8_X24:
      return 4;

   case GX2SurfaceFormat::INVALID:
   default:
      throw std::runtime_error("Unexpected GX2SurfaceFormat");
      return 0;
   }
}

static inline unsigned
GX2GetTileThickness(GX2TileMode::Value mode)
{
   switch (mode) {
   case GX2TileMode::LinearAligned:
   case GX2TileMode::LinearSpecial:
   case GX2TileMode::Tiled1DThin1:
   case GX2TileMode::Tiled2DThin1:
   case GX2TileMode::Tiled2DThin2:
   case GX2TileMode::Tiled2DThin4:
   case GX2TileMode::Tiled2BThin1:
   case GX2TileMode::Tiled2BThin2:
   case GX2TileMode::Tiled2BThin4:
   case GX2TileMode::Tiled3DThin1:
   case GX2TileMode::Tiled3BThin1:
      return 1;
   case GX2TileMode::Tiled1DThick:
   case GX2TileMode::Tiled2DThick:
   case GX2TileMode::Tiled2BThick:
   case GX2TileMode::Tiled3DThick:
   case GX2TileMode::Tiled3BThick:
      return 4;
   case GX2TileMode::Default:
   default:
      throw std::runtime_error("Unexpected GX2TileMode");
      return 0;
   }
}

static inline std::pair<unsigned, unsigned>
GX2GetMacroTileSize(GX2TileMode::Value mode)
{
   switch (mode) {
   case GX2TileMode::Tiled2DThin1:
   case GX2TileMode::Tiled2DThick:
   case GX2TileMode::Tiled2BThin1:
   case GX2TileMode::Tiled2BThick:
   case GX2TileMode::Tiled3DThin1:
   case GX2TileMode::Tiled3DThick:
   case GX2TileMode::Tiled3BThin1:
   case GX2TileMode::Tiled3BThick:
      return { num_banks, num_channels };
   case GX2TileMode::Tiled2DThin2:
   case GX2TileMode::Tiled2BThin2:
      return { num_banks / 2, num_channels * 2 };
   case GX2TileMode::Tiled2DThin4:
   case GX2TileMode::Tiled2BThin4:
      return { num_banks / 4, num_channels * 4 };
   default:
      throw std::logic_error("Unexpected GX2TileMode");
      return { 0, 0 };
   }
}

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

static inline unsigned
get_pixel_number(const TileInfo &info, unsigned x, unsigned y, unsigned z)
{
   auto pixel_number = 0u;

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

static inline unsigned
get_element_offset(const TileInfo &info, unsigned pixel_number, unsigned tile_bytes)
{
   static const auto stencil_data_size = 1u;
   static const auto depth_data_size = 3u;
   auto element_bytes = info.elementBytes;
   auto num_samples = info.numSamples;
   auto sample_number = info.curSample;
   auto element_offset = 0u;

   if (!info.isDepthTexture) {
      auto sample_offset = sample_number * (tile_bytes / num_samples);
      element_offset = sample_offset + (pixel_number * element_bytes);
   } else if (element_bytes == 2) {
      auto pixel_offset = pixel_number * element_bytes * num_samples;
      element_offset = pixel_offset + (sample_number * element_bytes);
   } else if (element_bytes == 4) {
      unsigned pixel_offset;

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

static inline unsigned
get_channel(unsigned x, unsigned y)
{
   auto channel = 0u;
   //channel[0] = x[3] ^ y[3]
   channel |= (get_bit<3>(x) ^ get_bit<3>(y)) << 0;
   return channel;
}

static inline unsigned
get_bank(unsigned x, unsigned y)
{
   auto bank = 0u;
   //bank[0] = x[3] ^ y[4 + log2(num_channels)]
   bank |= (get_bit<3>(x) ^ get_bit(y, 4 + channel_bits)) << 0;

   //bank[1] = x[4] ^ y[3 + log2(num_channels)]
   bank |= (get_bit<4>(x) ^ get_bit(y, 3 + channel_bits)) << 1;
   return bank;
}

unsigned
get_1d_offset(const TileInfo &info, unsigned x, unsigned y, unsigned z)
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

unsigned
get_2d_offset(const TileInfo &info, unsigned x, unsigned y, unsigned z)
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
   auto bank_slice_rotation = 0u;
   auto bank_swap_rotation = 0u;
   auto channel_slice_rotation = 0u;
   auto sample_slice_rotation = 0u;

   auto do_channel_rotate = GX2TileModeNeedsChannelRotation(info.tileMode);
   auto do_bank_swapping = GX2TileModeNeedsBankSwapping(info.tileMode);

   // All formats do bank rotation
   auto slice = z / tile_thickness;
   sample_slice_rotation = (((num_banks / 2) + 1) * sample_number);

   if (do_channel_rotate) {
      // 3D & 3B formats
      bank_slice_rotation = (std::max(1u, (num_channels / 2) - 1) * slice) / num_channels;
      channel_slice_rotation = std::max(1u, (num_channels / 2) - 1) * slice;
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
   auto group_mask = (1u << group_bits) - 1;
   auto offset_low = total_offset & group_mask;
   auto offset_high = (total_offset & ~group_mask) << (channel_bits + bank_bits);
   auto offset = (bank << (group_bits + channel_bits)) + (channel << group_bits) + offset_low + offset_high;

   return offset;
}

void
untileSurface(const GX2Surface *surface, std::vector<uint8_t> &out, uint32_t &pitchOut)
{
   if (surface->tileMode == GX2TileMode::LinearAligned) {
      throw std::runtime_error("Unsupported tile mode LinearAligned");
   }

   if (surface->tileMode == GX2TileMode::LinearSpecial) {
      throw std::runtime_error("Unsupported tile mode LinearSpecial");
   }

   if (surface->tileMode == GX2TileMode::Default) {
      throw std::runtime_error("Unsupported tile mode Default");
   }

   auto blockSize = GX2GetBlockSize(surface->format);
   auto bpe = GX2GetElementBytes(surface->format);
   uint32_t surfaceWidth = align_up(static_cast<uint32_t>(surface->width), blockSize.first);
   uint32_t surfaceHeight = align_up(static_cast<uint32_t>(surface->height), blockSize.second);

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
      for (auto y = 0u; y < info.height; ++y) {
         for (auto x = 0u; x < info.width; ++x) {
            auto offset = get_1d_offset(info, x, y, 0);
            std::memcpy(dst + (y * pitch) + (x * bpe), src + offset, bpe);
         }
      }
   } else {
      for (auto y = 0u; y < info.height; ++y) {
         for (auto x = 0u; x < info.width; ++x) {
            auto offset = get_2d_offset(info, x, y, 0);
            std::memcpy(dst + (y * pitch) + (x * bpe), src + offset, bpe);
         }
      }
   }

   pitchOut = pitch;
}
