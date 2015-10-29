#include "gx2_format.h"
#include "gpu/latte_untile.h"

std::pair<size_t, size_t>
GX2GetSurfaceBlockSize(GX2SurfaceFormat::Value format)
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

size_t
GX2GetSurfaceElementBytes(GX2SurfaceFormat::Value format)
{
   static const size_t bc1 = 8;
   static const size_t bc2 = 16;
   static const size_t bc3 = 16;
   static const size_t bc4 = 8;
   static const size_t bc5 = 16;

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

size_t
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

std::pair<size_t, size_t>
GX2GetMacroTileSize(GX2TileMode::Value mode)
{
   switch (mode) {
   case GX2TileMode::LinearAligned:
   case GX2TileMode::LinearSpecial:
      return { 0, 0 };
   case GX2TileMode::Tiled2DThin1:
   case GX2TileMode::Tiled2DThick:
   case GX2TileMode::Tiled2BThin1:
   case GX2TileMode::Tiled2BThick:
   case GX2TileMode::Tiled3DThin1:
   case GX2TileMode::Tiled3DThick:
   case GX2TileMode::Tiled3BThin1:
   case GX2TileMode::Tiled3BThick:
      return { latte::num_banks, latte::num_channels };
   case GX2TileMode::Tiled2DThin2:
   case GX2TileMode::Tiled2BThin2:
      return { latte::num_banks / 2, latte::num_channels * 2 };
   case GX2TileMode::Tiled2DThin4:
   case GX2TileMode::Tiled2BThin4:
      return { latte::num_banks / 4, latte::num_channels * 4 };
   default:
      throw std::logic_error("Unexpected GX2TileMode");
      return { 0, 0 };
   }
}
