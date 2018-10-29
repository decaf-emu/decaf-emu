#include "gx2.h"
#include "gx2_enum_string.h"
#include "gx2_format.h"

#include <common/decaf_assert.h>
#include <fmt/format.h>
#include <libgpu/latte/latte_enum_sq.h>

namespace cafe::gx2
{

struct GX2SurfaceFormatData
{
   uint8_t bpp;
   GX2SurfaceUse use;
   uint8_t endian;
   uint8_t sourceFormat; // CB_SOURCE_FORMAT
};

static GX2SurfaceFormatData
sSurfaceFormatData[] =
{
   { 0, GX2SurfaceUse::None, 0, 1 },
   { 8, GX2SurfaceUse::Texture | GX2SurfaceUse::ColorBuffer, 0, 1 },
   { 8, GX2SurfaceUse::Texture, 0, 1 },
   { 0, GX2SurfaceUse::None, 0, 1 },
   { 0, GX2SurfaceUse::None, 0, 1 },
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
   { 0, GX2SurfaceUse::None, 0, 0 },
   { 32, GX2SurfaceUse::Texture | GX2SurfaceUse::ColorBuffer, 0, 0 },
   { 0, GX2SurfaceUse::None, 0, 0 },
   { 0, GX2SurfaceUse::None, 0, 1 },
   { 32, GX2SurfaceUse::Texture | GX2SurfaceUse::ColorBuffer, 0, 1 },
   { 0, GX2SurfaceUse::None, 0, 1 },
   { 0, GX2SurfaceUse::None, 0, 1 },
   { 32, GX2SurfaceUse::Texture | GX2SurfaceUse::ColorBuffer | GX2SurfaceUse::ScanBuffer, 0, 1 },
   { 32, GX2SurfaceUse::Texture | GX2SurfaceUse::ColorBuffer | GX2SurfaceUse::ScanBuffer, 0, 1 },
   { 32, GX2SurfaceUse::Texture | GX2SurfaceUse::ColorBuffer | GX2SurfaceUse::ScanBuffer, 0, 1 },
   { 64, GX2SurfaceUse::Texture | GX2SurfaceUse::DepthBuffer, 0, 0 },
   { 64, GX2SurfaceUse::Texture | GX2SurfaceUse::ColorBuffer, 0, 0 },
   { 64, GX2SurfaceUse::Texture | GX2SurfaceUse::ColorBuffer, 0, 0 },
   { 64, GX2SurfaceUse::Texture | GX2SurfaceUse::ColorBuffer, 0, 0 },
   { 64, GX2SurfaceUse::Texture | GX2SurfaceUse::ColorBuffer, 0, 1 },
   { 0, GX2SurfaceUse::None, 0, 0 },
   { 128, GX2SurfaceUse::Texture | GX2SurfaceUse::ColorBuffer, 0, 0 },
   { 128, GX2SurfaceUse::Texture | GX2SurfaceUse::ColorBuffer, 0, 0 },
   { 0, GX2SurfaceUse::None, 0, 1 },
   { 0, GX2SurfaceUse::None, 0, 1 },
   { 0, GX2SurfaceUse::None, 0, 1 },
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
   { 0, GX2SurfaceUse::None, 0, 0 },
   { 0, GX2SurfaceUse::None, 0, 0 },
   { 0, GX2SurfaceUse::None, 0, 0 },
   { 0, GX2SurfaceUse::None, 0, 0 },
   { 0, GX2SurfaceUse::None, 0, 0 },
   { 0, GX2SurfaceUse::None, 0, 0 },
   { 0, GX2SurfaceUse::None, 0, 0 },
   { 0, GX2SurfaceUse::None, 0, 0 },
   { 0, GX2SurfaceUse::None, 0, 0 },
   { 0, GX2SurfaceUse::None, 0, 0 },
};

BOOL
GX2CheckSurfaceUseVsFormat(GX2SurfaceUse use,
                           GX2SurfaceFormat format)
{
   return internal::getSurfaceUse(format) & use ? TRUE : FALSE;
}

uint32_t
GX2GetAttribFormatBits(GX2AttribFormat format)
{
   switch (format & 0x1F) {
   case GX2AttribFormatType::TYPE_8:
   case GX2AttribFormatType::TYPE_4_4:
      return 8;
   case GX2AttribFormatType::TYPE_16:
   case GX2AttribFormatType::TYPE_16_FLOAT:
   case GX2AttribFormatType::TYPE_8_8:
      return 16;
   case GX2AttribFormatType::TYPE_32:
   case GX2AttribFormatType::TYPE_32_FLOAT:
   case GX2AttribFormatType::TYPE_16_16:
   case GX2AttribFormatType::TYPE_16_16_FLOAT:
   case GX2AttribFormatType::TYPE_10_11_11_FLOAT:
   case GX2AttribFormatType::TYPE_8_8_8_8:
   case GX2AttribFormatType::TYPE_10_10_10_2:
      return 32;
   case GX2AttribFormatType::TYPE_32_32:
   case GX2AttribFormatType::TYPE_32_32_FLOAT:
   case GX2AttribFormatType::TYPE_16_16_16_16:
   case GX2AttribFormatType::TYPE_16_16_16_16_FLOAT:
      return 64;
   case GX2AttribFormatType::TYPE_32_32_32:
   case GX2AttribFormatType::TYPE_32_32_32_FLOAT:
      return 96;
   case GX2AttribFormatType::TYPE_32_32_32_32:
   case GX2AttribFormatType::TYPE_32_32_32_32_FLOAT:
      return 128;
   default:
      decaf_abort(fmt::format("Invalid GX2AttribFormat {}", to_string(format)));
   }
}

uint32_t
GX2GetSurfaceFormatBits(GX2SurfaceFormat format)
{
   auto latteFormat = format & 0x3F;
   auto bpp = sSurfaceFormatData[latteFormat].bpp;

   if (latteFormat >= latte::SQ_DATA_FORMAT::FMT_BC1 && latteFormat <= latte::SQ_DATA_FORMAT::FMT_BC5) {
      bpp >>= 4;
   }

   return bpp;
}

uint32_t
GX2GetSurfaceFormatBitsPerElement(GX2SurfaceFormat format)
{
   return sSurfaceFormatData[format & 0x3F].bpp;
}

BOOL
GX2SurfaceIsCompressed(GX2SurfaceFormat format)
{
   auto latteFormat = format & 0x3F;

   if (latteFormat >= latte::SQ_DATA_FORMAT::FMT_BC1 && latteFormat <= latte::SQ_DATA_FORMAT::FMT_BC5) {
      return TRUE;
   }

   return FALSE;
}

namespace internal
{

uint32_t
getAttribFormatBytes(GX2AttribFormat format)
{
   return GX2GetAttribFormatBits(format) / 8;
}

latte::SQ_DATA_FORMAT
getAttribFormatDataFormat(GX2AttribFormat format)
{
   switch (format & 0x1F) {
   case GX2AttribFormatType::TYPE_8:
      return latte::SQ_DATA_FORMAT::FMT_8;
   case GX2AttribFormatType::TYPE_4_4:
      return latte::SQ_DATA_FORMAT::FMT_4_4;
   case GX2AttribFormatType::TYPE_16:
      return latte::SQ_DATA_FORMAT::FMT_16;
   case GX2AttribFormatType::TYPE_16_FLOAT:
      return latte::SQ_DATA_FORMAT::FMT_16_FLOAT;
   case GX2AttribFormatType::TYPE_8_8:
      return latte::SQ_DATA_FORMAT::FMT_8_8;
   case GX2AttribFormatType::TYPE_32:
      return latte::SQ_DATA_FORMAT::FMT_32;
   case GX2AttribFormatType::TYPE_32_FLOAT:
      return latte::SQ_DATA_FORMAT::FMT_32_FLOAT;
   case GX2AttribFormatType::TYPE_16_16:
      return latte::SQ_DATA_FORMAT::FMT_16_16;
   case GX2AttribFormatType::TYPE_16_16_FLOAT:
      return latte::SQ_DATA_FORMAT::FMT_16_16_FLOAT;
   case GX2AttribFormatType::TYPE_10_11_11_FLOAT:
      return latte::SQ_DATA_FORMAT::FMT_10_11_11_FLOAT;
   case GX2AttribFormatType::TYPE_8_8_8_8:
      return latte::SQ_DATA_FORMAT::FMT_8_8_8_8;
   case GX2AttribFormatType::TYPE_10_10_10_2:
      return latte::SQ_DATA_FORMAT::FMT_10_10_10_2;
   case GX2AttribFormatType::TYPE_32_32:
      return latte::SQ_DATA_FORMAT::FMT_32_32;
   case GX2AttribFormatType::TYPE_32_32_FLOAT:
      return latte::SQ_DATA_FORMAT::FMT_32_32_FLOAT;
   case GX2AttribFormatType::TYPE_16_16_16_16:
      return latte::SQ_DATA_FORMAT::FMT_16_16_16_16;
   case GX2AttribFormatType::TYPE_16_16_16_16_FLOAT:
      return latte::SQ_DATA_FORMAT::FMT_16_16_16_16_FLOAT;
   case GX2AttribFormatType::TYPE_32_32_32:
      return latte::SQ_DATA_FORMAT::FMT_32_32_32;
   case GX2AttribFormatType::TYPE_32_32_32_FLOAT:
      return latte::SQ_DATA_FORMAT::FMT_32_32_32_FLOAT;
   case GX2AttribFormatType::TYPE_32_32_32_32:
      return latte::SQ_DATA_FORMAT::FMT_32_32_32_32;
   case GX2AttribFormatType::TYPE_32_32_32_32_FLOAT:
      return latte::SQ_DATA_FORMAT::FMT_32_32_32_32_FLOAT;
   default:
      decaf_abort(fmt::format("Invalid GX2AttribFormat {}", to_string(format)));
   }
}

GX2EndianSwapMode
getAttribFormatSwapMode(GX2AttribFormat format)
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
      decaf_abort(fmt::format("Invalid GX2AttribFormat {}", to_string(format)));
   }
}

latte::SQ_ENDIAN
getAttribFormatEndian(GX2AttribFormat format)
{
   return getSwapModeEndian(getAttribFormatSwapMode(format));
}

uint32_t
getSurfaceFormatBytesPerElement(GX2SurfaceFormat format)
{
   return sSurfaceFormatData[format & 0x3F].bpp / 8;
}

GX2SurfaceUse
getSurfaceUse(GX2SurfaceFormat format)
{
   auto idx = format & 0x3F;
   if (idx == 17 || idx == 28) {
      return GX2SurfaceUse::DepthBuffer | GX2SurfaceUse::Texture;
   }

   return static_cast<GX2SurfaceUse>(sSurfaceFormatData[format & 0x3F].use);
}

GX2EndianSwapMode
getSurfaceFormatSwapMode(GX2SurfaceFormat format)
{
   return static_cast<GX2EndianSwapMode>(sSurfaceFormatData[format & 0x3F].endian);
}

latte::SQ_ENDIAN
getSurfaceFormatEndian(GX2SurfaceFormat format)
{
   return getSwapModeEndian(getSurfaceFormatSwapMode(format));
}

GX2SurfaceFormatType
getSurfaceFormatType(GX2SurfaceFormat format)
{
   return static_cast<GX2SurfaceFormatType>(format >> 8);
}

latte::CB_ENDIAN
getSurfaceFormatColorEndian(GX2SurfaceFormat format)
{
   return static_cast<latte::CB_ENDIAN>(sSurfaceFormatData[format & 0x3F].endian);
}

latte::CB_FORMAT
getSurfaceFormatColorFormat(GX2SurfaceFormat format)
{
   switch (format & 0x3F) {
   case latte::SQ_DATA_FORMAT::FMT_8:
      return latte::CB_FORMAT::COLOR_8;
   case latte::SQ_DATA_FORMAT::FMT_4_4:
      return latte::CB_FORMAT::COLOR_4_4;
   case latte::SQ_DATA_FORMAT::FMT_3_3_2:
      return latte::CB_FORMAT::COLOR_3_3_2;
   case latte::SQ_DATA_FORMAT::FMT_16:
      return latte::CB_FORMAT::COLOR_16;
   case latte::SQ_DATA_FORMAT::FMT_16_FLOAT:
      return latte::CB_FORMAT::COLOR_16_FLOAT;
   case latte::SQ_DATA_FORMAT::FMT_8_8:
      return latte::CB_FORMAT::COLOR_8_8;
   case latte::SQ_DATA_FORMAT::FMT_5_6_5:
      return latte::CB_FORMAT::COLOR_5_6_5;
   case latte::SQ_DATA_FORMAT::FMT_6_5_5:
      return latte::CB_FORMAT::COLOR_6_5_5;
   case latte::SQ_DATA_FORMAT::FMT_1_5_5_5:
      return latte::CB_FORMAT::COLOR_1_5_5_5;
   case latte::SQ_DATA_FORMAT::FMT_4_4_4_4:
      return latte::CB_FORMAT::COLOR_4_4_4_4;
   case latte::SQ_DATA_FORMAT::FMT_5_5_5_1:
      return latte::CB_FORMAT::COLOR_5_5_5_1;
   case latte::SQ_DATA_FORMAT::FMT_32:
      return latte::CB_FORMAT::COLOR_32;
   case latte::SQ_DATA_FORMAT::FMT_32_FLOAT:
      return latte::CB_FORMAT::COLOR_32_FLOAT;
   case latte::SQ_DATA_FORMAT::FMT_16_16:
      return latte::CB_FORMAT::COLOR_16_16;
   case latte::SQ_DATA_FORMAT::FMT_16_16_FLOAT:
      return latte::CB_FORMAT::COLOR_16_16_FLOAT;
   case latte::SQ_DATA_FORMAT::FMT_8_24:
      return latte::CB_FORMAT::COLOR_8_24;
   case latte::SQ_DATA_FORMAT::FMT_8_24_FLOAT:
      return latte::CB_FORMAT::COLOR_8_24_FLOAT;
   case latte::SQ_DATA_FORMAT::FMT_24_8:
      return latte::CB_FORMAT::COLOR_24_8;
   case latte::SQ_DATA_FORMAT::FMT_24_8_FLOAT:
      return latte::CB_FORMAT::COLOR_24_8_FLOAT;
   case latte::SQ_DATA_FORMAT::FMT_10_11_11:
      return latte::CB_FORMAT::COLOR_10_11_11;
   case latte::SQ_DATA_FORMAT::FMT_10_11_11_FLOAT:
      return latte::CB_FORMAT::COLOR_10_11_11_FLOAT;
   case latte::SQ_DATA_FORMAT::FMT_11_11_10:
      return latte::CB_FORMAT::COLOR_11_11_10;
   case latte::SQ_DATA_FORMAT::FMT_11_11_10_FLOAT:
      return latte::CB_FORMAT::COLOR_11_11_10_FLOAT;
   case latte::SQ_DATA_FORMAT::FMT_2_10_10_10:
      return latte::CB_FORMAT::COLOR_2_10_10_10;
   case latte::SQ_DATA_FORMAT::FMT_8_8_8_8:
      return latte::CB_FORMAT::COLOR_8_8_8_8;
   case latte::SQ_DATA_FORMAT::FMT_10_10_10_2:
      return latte::CB_FORMAT::COLOR_10_10_10_2;
   case latte::SQ_DATA_FORMAT::FMT_X24_8_32_FLOAT:
      return latte::CB_FORMAT::COLOR_X24_8_32_FLOAT;
   case latte::SQ_DATA_FORMAT::FMT_32_32:
      return latte::CB_FORMAT::COLOR_32_32;
   case latte::SQ_DATA_FORMAT::FMT_32_32_FLOAT:
      return latte::CB_FORMAT::COLOR_32_32_FLOAT;
   case latte::SQ_DATA_FORMAT::FMT_16_16_16_16:
      return latte::CB_FORMAT::COLOR_16_16_16_16;
   case latte::SQ_DATA_FORMAT::FMT_16_16_16_16_FLOAT:
      return latte::CB_FORMAT::COLOR_16_16_16_16_FLOAT;
   case latte::SQ_DATA_FORMAT::FMT_32_32_32_32:
      return latte::CB_FORMAT::COLOR_32_32_32_32;
   case latte::SQ_DATA_FORMAT::FMT_32_32_32_32_FLOAT:
      return latte::CB_FORMAT::COLOR_32_32_32_32_FLOAT;
   default:
      decaf_abort(fmt::format("Invalid GX2SurfaceFormat {}", to_string(format)));
   }
}

latte::CB_NUMBER_TYPE
getSurfaceFormatColorNumberType(GX2SurfaceFormat format)
{
   switch (internal::getSurfaceFormatType(format)) {
   case GX2SurfaceFormatType::UNORM:
      return latte::CB_NUMBER_TYPE::UNORM;
   case GX2SurfaceFormatType::UINT:
      return latte::CB_NUMBER_TYPE::UINT;
   case GX2SurfaceFormatType::SNORM:
      return latte::CB_NUMBER_TYPE::SNORM;
   case GX2SurfaceFormatType::SINT:
      return latte::CB_NUMBER_TYPE::SINT;
   case GX2SurfaceFormatType::SRGB:
      return latte::CB_NUMBER_TYPE::SRGB;
   case GX2SurfaceFormatType::FLOAT:
      return latte::CB_NUMBER_TYPE::FLOAT;
   default:
      decaf_abort(fmt::format("Invalid GX2SurfaceFormat {}", to_string(format)));
   }
}

latte::CB_SOURCE_FORMAT
getSurfaceFormatColorSourceFormat(GX2SurfaceFormat format)
{
   return static_cast<latte::CB_SOURCE_FORMAT>(sSurfaceFormatData[format & 0x3F].sourceFormat);
}

latte::SQ_ENDIAN
getSwapModeEndian(GX2EndianSwapMode mode)
{
   switch (mode) {
   case GX2EndianSwapMode::None:
      return latte::SQ_ENDIAN::NONE;
   case GX2EndianSwapMode::Swap8In16:
      return latte::SQ_ENDIAN::SWAP_8IN16;
   case GX2EndianSwapMode::Swap8In32:
      return latte::SQ_ENDIAN::SWAP_8IN32;
   case GX2EndianSwapMode::Default:
      return latte::SQ_ENDIAN::AUTO;
   default:
      decaf_abort(fmt::format("Invalid GX2EndianSwapMode {}", to_string(mode)));
   }
}

} // namespace internal

void
Library::registerFormatSymbols()
{
   RegisterFunctionExport(GX2CheckSurfaceUseVsFormat);
   RegisterFunctionExport(GX2GetAttribFormatBits);
   RegisterFunctionExport(GX2GetSurfaceFormatBits);
   RegisterFunctionExport(GX2GetSurfaceFormatBitsPerElement);
   RegisterFunctionExport(GX2SurfaceIsCompressed);
}

} // namespace cafe::gx2
