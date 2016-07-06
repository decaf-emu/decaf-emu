#include <stdexcept>
#include "gx2_format.h"
#include "gpu/latte_enum_sq.h"

namespace gx2
{

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
GX2GetAttribFormatBytes(GX2AttribFormat format)
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
   }
}

GX2EndianSwapMode
GX2GetAttribFormatSwapMode(GX2AttribFormat format)
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
   }
}

latte::SQ_DATA_FORMAT
GX2GetAttribFormatDataFormat(GX2AttribFormat format)
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
   }
}

std::pair<size_t, size_t>
GX2GetSurfaceBlockSize(GX2SurfaceFormat format)
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

uint32_t
GX2GetSurfaceFormatBits(GX2SurfaceFormat format)
{
   auto latteFormat = format & 0x3F;
   auto bpp = gSurfaceFormatData[latteFormat].bpp;

   if (latteFormat >= latte::FMT_BC1 && latteFormat <= latte::FMT_BC5) {
      bpp >>= 4;
   }

   return bpp;
}

uint32_t
GX2GetSurfaceFormatBitsPerElement(GX2SurfaceFormat format)
{
   return gSurfaceFormatData[format & 0x3F].bpp;
}

uint32_t
GX2GetSurfaceFormatBytesPerElement(GX2SurfaceFormat format)
{
   return gSurfaceFormatData[format & 0x3F].bpp / 8;
}

GX2SurfaceUse
GX2GetSurfaceUse(GX2SurfaceFormat format)
{
   return static_cast<GX2SurfaceUse>(gSurfaceFormatData[format & 0x3F].use);
}

GX2EndianSwapMode
GX2GetSurfaceSwap(GX2SurfaceFormat format)
{
   return static_cast<GX2EndianSwapMode>(gSurfaceFormatData[format & 0x3F].endian);
}

latte::DB_DEPTH_FORMAT
GX2GetSurfaceDepthFormat(GX2SurfaceFormat format)
{
   switch (format & 0x3F) {
   case latte::FMT_16:
      return latte::DEPTH_16;
   case latte::FMT_8_24:
      return latte::DEPTH_8_24;
   case latte::FMT_8_24_FLOAT:
      return latte::DEPTH_8_24_FLOAT;
   case latte::FMT_32_FLOAT:
      return latte::DEPTH_32_FLOAT;
   case latte::FMT_X24_8_32_FLOAT:
      return latte::DEPTH_X24_8_32_FLOAT;
   default:
      throw std::logic_error("Invalid GX2SurfaceFormat depth format");
   }
}

latte::CB_FORMAT
GX2GetSurfaceColorFormat(GX2SurfaceFormat format)
{
   switch (format & 0x3F) {
   case latte::FMT_8:
      return latte::COLOR_8;
   case latte::FMT_4_4:
      return latte::COLOR_4_4;
   case latte::FMT_3_3_2:
      return latte::COLOR_3_3_2;
   case latte::FMT_16:
      return latte::COLOR_16;
   case latte::FMT_16_FLOAT:
      return latte::COLOR_16_FLOAT;
   case latte::FMT_8_8:
      return latte::COLOR_8_8;
   case latte::FMT_5_6_5:
      return latte::COLOR_5_6_5;
   case latte::FMT_6_5_5:
      return latte::COLOR_6_5_5;
   case latte::FMT_1_5_5_5:
      return latte::COLOR_1_5_5_5;
   case latte::FMT_4_4_4_4:
      return latte::COLOR_4_4_4_4;
   case latte::FMT_5_5_5_1:
      return latte::COLOR_5_5_5_1;
   case latte::FMT_32:
      return latte::COLOR_32;
   case latte::FMT_32_FLOAT:
      return latte::COLOR_32_FLOAT;
   case latte::FMT_16_16:
      return latte::COLOR_16_16;
   case latte::FMT_16_16_FLOAT:
      return latte::COLOR_16_16_FLOAT;
   case latte::FMT_8_24:
      return latte::COLOR_8_24;
   case latte::FMT_8_24_FLOAT:
      return latte::COLOR_8_24_FLOAT;
   case latte::FMT_24_8:
      return latte::COLOR_24_8;
   case latte::FMT_24_8_FLOAT:
      return latte::COLOR_24_8_FLOAT;
   case latte::FMT_10_11_11:
      return latte::COLOR_10_11_11;
   case latte::FMT_10_11_11_FLOAT:
      return latte::COLOR_10_11_11_FLOAT;
   case latte::FMT_11_11_10:
      return latte::COLOR_11_11_10;
   case latte::FMT_11_11_10_FLOAT:
      return latte::COLOR_11_11_10_FLOAT;
   case latte::FMT_2_10_10_10:
      return latte::COLOR_2_10_10_10;
   case latte::FMT_8_8_8_8:
      return latte::COLOR_8_8_8_8;
   case latte::FMT_10_10_10_2:
      return latte::COLOR_10_10_10_2;
   case latte::FMT_X24_8_32_FLOAT:
      return latte::COLOR_X24_8_32_FLOAT;
   case latte::FMT_32_32:
      return latte::COLOR_32_32;
   case latte::FMT_32_32_FLOAT:
      return latte::COLOR_32_32_FLOAT;
   case latte::FMT_16_16_16_16:
      return latte::COLOR_16_16_16_16;
   case latte::FMT_16_16_16_16_FLOAT:
      return latte::COLOR_16_16_16_16_FLOAT;
   case latte::FMT_32_32_32_32:
      return latte::COLOR_32_32_32_32;
   case latte::FMT_32_32_32_32_FLOAT:
      return latte::COLOR_32_32_32_32_FLOAT;
   default:
      throw std::logic_error("Invalid GX2SurfaceFormat color format");
   }
}

} // namespace gx2
