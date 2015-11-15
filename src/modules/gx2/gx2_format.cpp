#include "gx2_format.h"
#include "gpu/latte_untile.h"
#include "gpu/latte_enum_sq.h"

struct GX2SurfaceFormatData
{
   uint8_t bpp;
   uint8_t use;
   uint8_t endian;
   uint8_t unk;
};

GX2SurfaceFormatData gSurfaceFormatData[] =
{
   { 0, 0, 0, 1 },
   { 8, GX2SurfaceUse::Texture | GX2SurfaceUse::ColorBuffer, 0, 1 },
   { 8, GX2SurfaceUse::Texture, 0, 1 },
   { 0, 0, 0, 1 },
   { 0, 0, 0, 1 },
   { 16, GX2SurfaceUse::Texture | GX2SurfaceUse::ColorBuffer | GX2SurfaceUse::DepthBuffer, 0, 0 },
   { 16, GX2SurfaceUse::Texture | GX2SurfaceUse::ColorBuffer, 0, 1 },
   { 16, GX2SurfaceUse::Texture | GX2SurfaceUse::ColorBuffer, 0, 1 },
   { 16, GX2SurfaceUse::Texture | GX2SurfaceUse::ColorBuffer | GX2SurfaceUse::ScanBuffer, 0, 1 },
   { 16, GX2SurfaceUse::Texture, 0, 1 },
   { 16, GX2SurfaceUse::Texture | GX2SurfaceUse::ColorBuffer, 0, 1 },
   { 16, GX2SurfaceUse::Texture | GX2SurfaceUse::ColorBuffer, 0, 1 },
   { 16, GX2SurfaceUse::Texture | GX2SurfaceUse::ColorBuffer, 0, 1 },
   { 32, GX2SurfaceUse::Texture | GX2SurfaceUse::ColorBuffer, 0, 0 },
   { 32, GX2SurfaceUse::Texture | GX2SurfaceUse::ColorBuffer | GX2SurfaceUse::DepthBuffer, 0, 0 },
   { 32, GX2SurfaceUse::Texture | GX2SurfaceUse::ColorBuffer, 0, 0 },
   { 32, GX2SurfaceUse::Texture | GX2SurfaceUse::ColorBuffer, 0, 1 },
   { 32, GX2SurfaceUse::Texture | GX2SurfaceUse::DepthBuffer, 0, 0 },
   { 0, 0, 0, 0 },
   { 32, GX2SurfaceUse::Texture | GX2SurfaceUse::ColorBuffer, 0, 0 },
   { 0, 0, 0, 0 },
   { 0, 0, 0, 1 },
   { 32, GX2SurfaceUse::Texture | GX2SurfaceUse::ColorBuffer, 0, 1 },
   { 0, 0, 0, 1 },
   { 0, 0, 0, 1 },
   { 32, GX2SurfaceUse::Texture | GX2SurfaceUse::ColorBuffer | GX2SurfaceUse::ScanBuffer, 0, 1 },
   { 32, GX2SurfaceUse::Texture | GX2SurfaceUse::ColorBuffer | GX2SurfaceUse::ScanBuffer, 0, 1 },
   { 32, GX2SurfaceUse::Texture | GX2SurfaceUse::ColorBuffer | GX2SurfaceUse::ScanBuffer, 0, 1 },
   { 64, GX2SurfaceUse::Texture | GX2SurfaceUse::DepthBuffer, 0, 0 },
   { 64, GX2SurfaceUse::Texture | GX2SurfaceUse::ColorBuffer, 0, 0 },
   { 64, GX2SurfaceUse::Texture | GX2SurfaceUse::ColorBuffer, 0, 0 },
   { 64, GX2SurfaceUse::Texture | GX2SurfaceUse::ColorBuffer, 0, 0 },
   { 64, GX2SurfaceUse::Texture | GX2SurfaceUse::ColorBuffer, 0, 1 },
   { 0, 0, 0, 0 },
   { 128, GX2SurfaceUse::Texture | GX2SurfaceUse::ColorBuffer, 0, 0 },
   { 128, GX2SurfaceUse::Texture | GX2SurfaceUse::ColorBuffer, 0, 0 },
   { 0, 0, 0, 1 },
   { 0, 0, 0, 1 },
   { 0, 0, 0, 1 },
   { 16, GX2SurfaceUse::Texture, 0, 0 },
   { 16, GX2SurfaceUse::Texture, 0, 0 },
   { 32, GX2SurfaceUse::Texture, 0, 0 },
   { 32, GX2SurfaceUse::Texture, 0, 0 },
   { 32, GX2SurfaceUse::Texture, 0, 0 },
   { 0, GX2SurfaceUse::Texture, 0, 1 },
   { 0, GX2SurfaceUse::Texture, 0, 0 },
   { 0, GX2SurfaceUse::Texture, 0, 0 },
   { 96, GX2SurfaceUse::Texture, 0, 0 },
   { 96, GX2SurfaceUse::Texture, 0, 0 },
   { 64, GX2SurfaceUse::Texture, 0, 1 },
   { 128, GX2SurfaceUse::Texture, 0, 1 },
   { 128, GX2SurfaceUse::Texture, 0, 1 },
   { 64, GX2SurfaceUse::Texture, 0, 1 },
   { 128, GX2SurfaceUse::Texture, 0, 1 },
   { 0, 0, 0, 0 },
   { 0, 0, 0, 0 },
   { 0, 0, 0, 0 },
   { 0, 0, 0, 0 },
   { 0, 0, 0, 0 },
   { 0, 0, 0, 0 },
   { 0, 0, 0, 0 },
   { 0, 0, 0, 0 },
   { 0, 0, 0, 0 },
   { 0, 0, 0, 0 },
};

size_t
GX2GetAttribFormatBytes(GX2AttribFormat::Value format)
{
   switch (format & 0x1f) {
   case GX2AttribFormatType::TYPE_8:
   case GX2AttribFormatType::TYPE_4_4:
      return 1;
   case GX2AttribFormatType::TYPE_16:
   case GX2AttribFormatType::TYPE_16_FLOAT:
   case GX2AttribFormatType::TYPE_8_8:
      return 2;
   case GX2AttribFormatType::TYPE_32:
   case GX2AttribFormatType::TYPE_32_FLOAT:
   case GX2AttribFormatType::TYPE_16_16:
   case GX2AttribFormatType::TYPE_16_16_FLOAT:
   case GX2AttribFormatType::TYPE_10_11_11_FLOAT:
   case GX2AttribFormatType::TYPE_8_8_8_8:
   case GX2AttribFormatType::TYPE_10_10_10_2:
      return 4;
   case GX2AttribFormatType::TYPE_32_32:
   case GX2AttribFormatType::TYPE_32_32_FLOAT:
   case GX2AttribFormatType::TYPE_16_16_16_16:
   case GX2AttribFormatType::TYPE_16_16_16_16_FLOAT:
      return 8;
   case GX2AttribFormatType::TYPE_32_32_32:
   case GX2AttribFormatType::TYPE_32_32_32_FLOAT:
      return 12;
   case GX2AttribFormatType::TYPE_32_32_32_32:
   case GX2AttribFormatType::TYPE_32_32_32_32_FLOAT:
      return 16;
   default:
      throw std::logic_error("Invalid GX2AttribFormat format");
      return 1;
   }
}

GX2EndianSwapMode::Value
GX2GetAttribFormatSwapMode(GX2AttribFormat::Value format)
{
   switch (format & 0x1F) {
   case GX2AttribFormatType::TYPE_8:
   case GX2AttribFormatType::TYPE_4_4:
   case GX2AttribFormatType::TYPE_8_8:
   case GX2AttribFormatType::TYPE_8_8_8_8:
      return GX2EndianSwapMode::None;
   case GX2AttribFormatType::TYPE_16:
   case GX2AttribFormatType::TYPE_16_FLOAT:
   case GX2AttribFormatType::TYPE_16_16:
   case GX2AttribFormatType::TYPE_16_16_FLOAT:
   case GX2AttribFormatType::TYPE_16_16_16_16:
   case GX2AttribFormatType::TYPE_16_16_16_16_FLOAT:
      return GX2EndianSwapMode::Swap8In16;
   case GX2AttribFormatType::TYPE_32:
   case GX2AttribFormatType::TYPE_32_FLOAT:
   case GX2AttribFormatType::TYPE_10_11_11_FLOAT:
   case GX2AttribFormatType::TYPE_10_10_10_2:
   case GX2AttribFormatType::TYPE_32_32:
   case GX2AttribFormatType::TYPE_32_32_FLOAT:
   case GX2AttribFormatType::TYPE_32_32_32:
   case GX2AttribFormatType::TYPE_32_32_32_FLOAT:
   case GX2AttribFormatType::TYPE_32_32_32_32:
   case GX2AttribFormatType::TYPE_32_32_32_32_FLOAT:
      return GX2EndianSwapMode::Swap8In32;
   default:
      throw std::logic_error("Invalid GX2AttribFormat format");
      return GX2EndianSwapMode::None;
   }
}

latte::SQ_DATA_FORMAT
GX2GetAttribFormatDataFormat(GX2AttribFormat::Value format)
{
   switch (format & 0x1F) {
   case GX2AttribFormatType::TYPE_8:
      return latte::FMT_8;
   case GX2AttribFormatType::TYPE_4_4:
      return latte::FMT_4_4;
   case GX2AttribFormatType::TYPE_16:
      return latte::FMT_16;
   case GX2AttribFormatType::TYPE_16_FLOAT:
      return latte::FMT_16_FLOAT;
   case GX2AttribFormatType::TYPE_8_8:
      return latte::FMT_8_8;
   case GX2AttribFormatType::TYPE_32:
      return latte::FMT_32;
   case GX2AttribFormatType::TYPE_32_FLOAT:
      return latte::FMT_32_FLOAT;
   case GX2AttribFormatType::TYPE_16_16:
      return latte::FMT_16_16;
   case GX2AttribFormatType::TYPE_16_16_FLOAT:
      return latte::FMT_16_16_FLOAT;
   case GX2AttribFormatType::TYPE_10_11_11_FLOAT:
      return latte::FMT_10_11_11_FLOAT;
   case GX2AttribFormatType::TYPE_8_8_8_8:
      return latte::FMT_8_8_8_8;
   case GX2AttribFormatType::TYPE_10_10_10_2:
      return latte::FMT_10_10_10_2;
   case GX2AttribFormatType::TYPE_32_32:
      return latte::FMT_32_32;
   case GX2AttribFormatType::TYPE_32_32_FLOAT:
      return latte::FMT_32_32_FLOAT;
   case GX2AttribFormatType::TYPE_16_16_16_16:
      return latte::FMT_16_16_16_16;
   case GX2AttribFormatType::TYPE_16_16_16_16_FLOAT:
      return latte::FMT_16_16_16_16_FLOAT;
   case GX2AttribFormatType::TYPE_32_32_32:
      return latte::FMT_32_32_32;
   case GX2AttribFormatType::TYPE_32_32_32_FLOAT:
      return latte::FMT_32_32_32_FLOAT;
   case GX2AttribFormatType::TYPE_32_32_32_32:
      return latte::FMT_32_32_32_32;
   case GX2AttribFormatType::TYPE_32_32_32_32_FLOAT:
      return latte::FMT_32_32_32_32_FLOAT;
   default:
      throw std::logic_error("Invalid GX2AttribFormat format");
      return latte::FMT_8;
   }
}

std::pair<size_t, size_t>
GX2GetSurfaceBlockSize(GX2SurfaceFormat::Value format)
{
   switch (format & 0x3F) {
   case latte::FMT_BC1:
   case latte::FMT_BC2:
   case latte::FMT_BC3:
   case latte::FMT_BC4:
   case latte::FMT_BC5:
      return { 4, 4 };
   default:
      return { 1, 1 };
   }
}

size_t
GX2GetSurfaceElementBytes(GX2SurfaceFormat::Value format)
{
   return gSurfaceFormatData[format & 0x3F].bpp / 8;
}

GX2SurfaceUse::Value
GX2GetSurfaceUse(GX2SurfaceFormat::Value format)
{
   return static_cast<GX2SurfaceUse::Value>(gSurfaceFormatData[format & 0x3F].use);
}

GX2EndianSwapMode::Value
GX2GetSurfaceSwap(GX2SurfaceFormat::Value format)
{
   return static_cast<GX2EndianSwapMode::Value>(gSurfaceFormatData[format & 0x3F].endian);
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
