#include "latte/latte_formats.h"

#include <common/decaf_assert.h>
#include <common/log.h>
#include <fmt/format.h>

namespace latte
{

enum SurfaceFormatType : uint32_t
{
   Unorm = 0x0,
   Uint = 0x1,
   Snorm = 0x2,
   Sint = 0x3,
   Srgb = 0x4,
   Float = 0x8,
};

static void
validateSurfaceFormat(SurfaceFormat format)
{
   switch (format) {
   case SurfaceFormat::R8Unorm:
   case SurfaceFormat::R8Uint:
   case SurfaceFormat::R8Snorm:
   case SurfaceFormat::R8Sint:
   case SurfaceFormat::R4G4Unorm:
   case SurfaceFormat::R16Unorm:
   case SurfaceFormat::R16Uint:
   case SurfaceFormat::R16Snorm:
   case SurfaceFormat::R16Sint:
   case SurfaceFormat::R16Float:
   case SurfaceFormat::R8G8Unorm:
   case SurfaceFormat::R8G8Uint:
   case SurfaceFormat::R8G8Snorm:
   case SurfaceFormat::R8G8Sint:
   case SurfaceFormat::R5G6B5Unorm:
   case SurfaceFormat::R5G5B5A1Unorm:
   case SurfaceFormat::R4G4B4A4Unorm:
   case SurfaceFormat::A1B5G5R5Unorm:
   case SurfaceFormat::R32Uint:
   case SurfaceFormat::R32Sint:
   case SurfaceFormat::R32Float:
   case SurfaceFormat::R16G16Unorm:
   case SurfaceFormat::R16G16Uint:
   case SurfaceFormat::R16G16Snorm:
   case SurfaceFormat::R16G16Sint:
   case SurfaceFormat::R16G16Float:
   case SurfaceFormat::D24UnormS8Uint:
   case SurfaceFormat::X24G8Uint:
   case SurfaceFormat::R11G11B10Float:
   case SurfaceFormat::R10G10B10A2Unorm:
   case SurfaceFormat::R10G10B10A2Uint:
   case SurfaceFormat::R10G10B10A2Snorm:
   case SurfaceFormat::R10G10B10A2Sint:
   case SurfaceFormat::R8G8B8A8Unorm:
   case SurfaceFormat::R8G8B8A8Uint:
   case SurfaceFormat::R8G8B8A8Snorm:
   case SurfaceFormat::R8G8B8A8Sint:
   case SurfaceFormat::R8G8B8A8Srgb:
   case SurfaceFormat::A2B10G10R10Unorm:
   case SurfaceFormat::A2B10G10R10Uint:
   case SurfaceFormat::D32FloatS8UintX24:
   case SurfaceFormat::D32G8UintX24:
   case SurfaceFormat::R32G32Uint:
   case SurfaceFormat::R32G32Sint:
   case SurfaceFormat::R32G32Float:
   case SurfaceFormat::R16G16B16A16Unorm:
   case SurfaceFormat::R16G16B16A16Uint:
   case SurfaceFormat::R16G16B16A16Snorm:
   case SurfaceFormat::R16G16B16A16Sint:
   case SurfaceFormat::R16G16B16A16Float:
   case SurfaceFormat::R32G32B32A32Uint:
   case SurfaceFormat::R32G32B32A32Sint:
   case SurfaceFormat::R32G32B32A32Float:
   case SurfaceFormat::BC1Unorm:
   case SurfaceFormat::BC1Srgb:
   case SurfaceFormat::BC2Unorm:
   case SurfaceFormat::BC2Srgb:
   case SurfaceFormat::BC3Unorm:
   case SurfaceFormat::BC3Srgb:
   case SurfaceFormat::BC4Unorm:
   case SurfaceFormat::BC4Snorm:
   case SurfaceFormat::BC5Unorm:
   case SurfaceFormat::BC5Snorm:
   case SurfaceFormat::NV12:
      return;
   default:
      decaf_abort("Unexpected generated surface format");
   }
}

static SurfaceFormatType
getSurfaceFormatType(latte::SQ_NUM_FORMAT numFormat,
                     latte::SQ_FORMAT_COMP formatComp,
                     bool forceDegamma)
{
   if (forceDegamma) {
      decaf_check(numFormat == latte::SQ_NUM_FORMAT::NORM);
      decaf_check(formatComp == latte::SQ_FORMAT_COMP::UNSIGNED);

      return SurfaceFormatType::Srgb;
   } else {
      if (numFormat == latte::SQ_NUM_FORMAT::NORM) {
         if (formatComp == latte::SQ_FORMAT_COMP::UNSIGNED) {
            return SurfaceFormatType::Unorm;
         } else if (formatComp == latte::SQ_FORMAT_COMP::SIGNED) {
            return SurfaceFormatType::Snorm;
         } else {
            decaf_abort(fmt::format("Unexpected surface format comp {}", formatComp));
         }
      } else if (numFormat == latte::SQ_NUM_FORMAT::INT) {
         if (formatComp == latte::SQ_FORMAT_COMP::UNSIGNED) {
            return SurfaceFormatType::Uint;
         } else if (formatComp == latte::SQ_FORMAT_COMP::SIGNED) {
            return SurfaceFormatType::Sint;
         } else {
            decaf_abort(fmt::format("Unexpected surface format comp {}", formatComp));
         }
      } else if (numFormat == latte::SQ_NUM_FORMAT::SCALED) {
         decaf_check(formatComp == latte::SQ_FORMAT_COMP::UNSIGNED);

         return SurfaceFormatType::Float;
      } else {
         decaf_abort(fmt::format("Unexpected surface number format {}", numFormat));
      }
   }
}

SurfaceFormat
getSurfaceFormat(latte::SQ_DATA_FORMAT dataFormat,
                 latte::SQ_NUM_FORMAT numFormat,
                 latte::SQ_FORMAT_COMP formatComp,
                 bool forceDegamma)
{
   if (dataFormat == latte::SQ_DATA_FORMAT::FMT_INVALID) {
      return latte::SurfaceFormat::Invalid;
   }

   auto formatType = getSurfaceFormatType(numFormat, formatComp, forceDegamma);

   auto formatTypeBits = static_cast<uint32_t>(formatType) << 8;
   auto dataFormatBits = static_cast<uint32_t>(dataFormat);

   auto surfaceFormat = static_cast<SurfaceFormat>(formatTypeBits | dataFormatBits);
   validateSurfaceFormat(surfaceFormat);

   return surfaceFormat;
}

latte::SQ_DATA_FORMAT
getSurfaceFormatDataFormat(latte::SurfaceFormat format)
{
   return static_cast<latte::SQ_DATA_FORMAT>(format & 0x00FF);
}

static SurfaceFormatType
getColorBufferSurfaceFormatType(latte::CB_NUMBER_TYPE numberType)
{
   switch (numberType) {
   case latte::CB_NUMBER_TYPE::UNORM:
      return SurfaceFormatType::Unorm;
   case latte::CB_NUMBER_TYPE::SNORM:
      return SurfaceFormatType::Snorm;
   case latte::CB_NUMBER_TYPE::UINT:
      return SurfaceFormatType::Uint;
   case latte::CB_NUMBER_TYPE::SINT:
      return SurfaceFormatType::Sint;
   case latte::CB_NUMBER_TYPE::FLOAT:
      return SurfaceFormatType::Float;
   case latte::CB_NUMBER_TYPE::SRGB:
      return SurfaceFormatType::Srgb;
   default:
      decaf_abort(fmt::format("Unexpected color buffer number type {}", numberType));
   }
}

SurfaceFormat
getColorBufferSurfaceFormat(latte::CB_FORMAT format, latte::CB_NUMBER_TYPE numberType)
{
   // Pick the correct surface format type for this number type
   auto formatType = getColorBufferSurfaceFormatType(numberType);

   // The format types belong in the upper portion of the bits
   auto formatTypeBits = static_cast<uint32_t>(formatType) << 8;

   // This is safe only becase CB_FORMAT perfectly overlays SQ_DATA_FORMAT
   auto dataFormatBits = static_cast<uint32_t>(format);

   // Generate our surface format
   auto surfaceFormat = static_cast<SurfaceFormat>(formatTypeBits | dataFormatBits);
   validateSurfaceFormat(surfaceFormat);

   return surfaceFormat;
}

SurfaceFormat
getDepthBufferSurfaceFormat(latte::DB_FORMAT format)
{
   switch (format) {
   case latte::DB_FORMAT::DEPTH_16:
      return SurfaceFormat::R16Unorm;
   case latte::DB_FORMAT::DEPTH_8_24:
      return SurfaceFormat::D24UnormS8Uint;
   //case latte::DB_FORMAT::DEPTH_8_24_FLOAT:
      // I don't believe this format is supported by the WiiU
   case latte::DB_FORMAT::DEPTH_32_FLOAT:
      return SurfaceFormat::R32Float;
   case latte::DB_FORMAT::DEPTH_X24_8_32_FLOAT:
      return SurfaceFormat::D32G8UintX24;
   }

   decaf_abort(fmt::format("Depth buffer with unsupported format {}", format));
}

uint32_t
getDataFormatBitsPerElement(latte::SQ_DATA_FORMAT format)
{
   switch (format) {
   case latte::SQ_DATA_FORMAT::FMT_8:
   case latte::SQ_DATA_FORMAT::FMT_3_3_2:
      return 8;

   case latte::SQ_DATA_FORMAT::FMT_8_8:
   case latte::SQ_DATA_FORMAT::FMT_16:
   case latte::SQ_DATA_FORMAT::FMT_16_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_5_6_5:
   case latte::SQ_DATA_FORMAT::FMT_5_5_5_1:
   case latte::SQ_DATA_FORMAT::FMT_1_5_5_5:
   case latte::SQ_DATA_FORMAT::FMT_4_4_4_4:
      return 16;

   case latte::SQ_DATA_FORMAT::FMT_8_8_8:
      return 24;

   case latte::SQ_DATA_FORMAT::FMT_8_8_8_8:
   case latte::SQ_DATA_FORMAT::FMT_16_16:
   case latte::SQ_DATA_FORMAT::FMT_16_16_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_32:
   case latte::SQ_DATA_FORMAT::FMT_32_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_10_10_10_2:
   case latte::SQ_DATA_FORMAT::FMT_2_10_10_10:
   case latte::SQ_DATA_FORMAT::FMT_10_11_11:
   case latte::SQ_DATA_FORMAT::FMT_10_11_11_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_11_11_10:
   case latte::SQ_DATA_FORMAT::FMT_11_11_10_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_8_24:
   case latte::SQ_DATA_FORMAT::FMT_8_24_FLOAT:
      return 32;

   case latte::SQ_DATA_FORMAT::FMT_16_16_16:
   case latte::SQ_DATA_FORMAT::FMT_16_16_16_FLOAT:
      return 48;

   case latte::SQ_DATA_FORMAT::FMT_16_16_16_16:
   case latte::SQ_DATA_FORMAT::FMT_16_16_16_16_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_32_32:
   case latte::SQ_DATA_FORMAT::FMT_32_32_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_BC1:
   case latte::SQ_DATA_FORMAT::FMT_BC4:
      return 64;

   case latte::SQ_DATA_FORMAT::FMT_32_32_32:
   case latte::SQ_DATA_FORMAT::FMT_32_32_32_FLOAT:
      return 96;

   case latte::SQ_DATA_FORMAT::FMT_32_32_32_32:
   case latte::SQ_DATA_FORMAT::FMT_32_32_32_32_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_BC2:
   case latte::SQ_DATA_FORMAT::FMT_BC3:
   case latte::SQ_DATA_FORMAT::FMT_BC5:
      return 128;

   default:
      decaf_abort(fmt::format("Unimplemented data format {}", format));
   }
}

bool
getDataFormatIsCompressed(latte::SQ_DATA_FORMAT format)
{
   switch (format) {
   case latte::SQ_DATA_FORMAT::FMT_BC1:
   case latte::SQ_DATA_FORMAT::FMT_BC2:
   case latte::SQ_DATA_FORMAT::FMT_BC3:
   case latte::SQ_DATA_FORMAT::FMT_BC4:
   case latte::SQ_DATA_FORMAT::FMT_BC5:
      return true;
   default:
      return false;
   }
}

std::string
getDataFormatName(latte::SQ_DATA_FORMAT format)
{
   switch (format) {
   case latte::SQ_DATA_FORMAT::FMT_8:
      return "FMT_8";
   case latte::SQ_DATA_FORMAT::FMT_16:
      return "FMT_16";
   case latte::SQ_DATA_FORMAT::FMT_16_FLOAT:
      return "FMT_16_FLOAT";
   case latte::SQ_DATA_FORMAT::FMT_32:
      return "FMT_32";
   case latte::SQ_DATA_FORMAT::FMT_32_FLOAT:
      return "FMT_32_FLOAT";
   case latte::SQ_DATA_FORMAT::FMT_8_8:
      return "FMT_8_8";
   case latte::SQ_DATA_FORMAT::FMT_16_16:
      return "FMT_16_16";
   case latte::SQ_DATA_FORMAT::FMT_16_16_FLOAT:
      return "FMT_16_16_FLOAT";
   case latte::SQ_DATA_FORMAT::FMT_32_32:
      return "FMT_32_32";
   case latte::SQ_DATA_FORMAT::FMT_32_32_FLOAT:
      return "FMT_32_32_FLOAT";
   case latte::SQ_DATA_FORMAT::FMT_8_8_8:
      return "FMT_8_8_8";
   case latte::SQ_DATA_FORMAT::FMT_16_16_16:
      return "FMT_16_16_16";
   case latte::SQ_DATA_FORMAT::FMT_16_16_16_FLOAT:
      return "FMT_16_16_16_FLOAT";
   case latte::SQ_DATA_FORMAT::FMT_32_32_32:
      return "FMT_32_32_32";
   case latte::SQ_DATA_FORMAT::FMT_32_32_32_FLOAT:
      return "FMT_32_32_32_FLOAT";
   case latte::SQ_DATA_FORMAT::FMT_8_8_8_8:
      return "FMT_8_8_8_8";
   case latte::SQ_DATA_FORMAT::FMT_16_16_16_16:
      return "FMT_16_16_16_16";
   case latte::SQ_DATA_FORMAT::FMT_16_16_16_16_FLOAT:
      return "FMT_16_16_16_16_FLOAT";
   case latte::SQ_DATA_FORMAT::FMT_32_32_32_32:
      return "FMT_32_32_32_32";
   case latte::SQ_DATA_FORMAT::FMT_32_32_32_32_FLOAT:
      return "FMT_32_32_32_32_FLOAT";
   case latte::SQ_DATA_FORMAT::FMT_2_10_10_10:
      return "FMT_2_10_10_10";
   case latte::SQ_DATA_FORMAT::FMT_10_10_10_2:
      return "FMT_10_10_10_2";
   default:
      decaf_abort(fmt::format("Unimplemented attribute format: {}", format));
   }
}

DataFormatMeta
getDataFormatMeta(latte::SQ_DATA_FORMAT format)
{
   static const auto DFT_NONE = DataFormatMetaType::None;
   static const auto DFT_UINT = DataFormatMetaType::UINT;
   static const auto DFT_FLOAT = DataFormatMetaType::FLOAT;
   static const auto BADELEM = DataFormatMetaElem { 0, 0, 0 };

   // Note: In order to reduce the likelyhood of encountering an unsupported
   // 8-bit type in the SPIRV, we intentionally collapse some types...

   switch (format) {
   case latte::SQ_DATA_FORMAT::FMT_8:
      return{ 8, 1, DFT_UINT, {{ 0, 0, 8 }, BADELEM, BADELEM, BADELEM } };
   case latte::SQ_DATA_FORMAT::FMT_4_4:
      return{ 8, 1, DFT_UINT, {{0, 0, 4}, {0, 4, 4}, BADELEM, BADELEM } };
   case latte::SQ_DATA_FORMAT::FMT_3_3_2:
      return{ 8, 1, DFT_UINT, {{0, 0, 3}, {0, 3, 3}, {0, 6, 2}, BADELEM } };
   case latte::SQ_DATA_FORMAT::FMT_16:
      return{ 16, 1,DFT_UINT, {{ 0, 0, 16 }, BADELEM, BADELEM, BADELEM } };
   case latte::SQ_DATA_FORMAT::FMT_16_FLOAT:
      return{ 16, 1,DFT_FLOAT, {{ 0, 0, 16 }, BADELEM, BADELEM, BADELEM } };
   case latte::SQ_DATA_FORMAT::FMT_8_8:
      return{ 16, 1, DFT_UINT, {{ 0, 0, 8 },{ 0, 8, 8 }, BADELEM, BADELEM } };
   case latte::SQ_DATA_FORMAT::FMT_5_6_5:
      return{ 16, 1, DFT_UINT, {{0, 0, 5}, {0, 5, 6}, {0, 11, 5}, BADELEM } };
   case latte::SQ_DATA_FORMAT::FMT_6_5_5:
      return{ 16, 1, DFT_UINT, {{ 0, 0, 6 },{ 0, 6, 5 },{ 0, 11, 5 }, BADELEM } };
   case latte::SQ_DATA_FORMAT::FMT_1_5_5_5:
      return{ 16, 1, DFT_UINT, {{0, 11, 5}, {0, 6, 5}, {0, 1, 5}, {0, 0, 1}  } };
   case latte::SQ_DATA_FORMAT::FMT_4_4_4_4:
      return{ 16, 1, DFT_UINT, {{0, 0, 4}, {0, 4, 4}, {0, 8, 4}, {0, 12, 4} } };
   case latte::SQ_DATA_FORMAT::FMT_5_5_5_1:
      return{ 16, 1, DFT_UINT, {{0, 0, 5}, {0, 5, 5}, {0, 10, 5}, {0, 15, 1} } };
   case latte::SQ_DATA_FORMAT::FMT_32:
      return{ 32, 1,DFT_UINT, {{ 0, 0, 32 }, BADELEM, BADELEM, BADELEM } };
   case latte::SQ_DATA_FORMAT::FMT_32_FLOAT:
      return{ 32, 1,DFT_FLOAT, {{ 0, 0, 32 }, BADELEM, BADELEM, BADELEM } };
   case latte::SQ_DATA_FORMAT::FMT_16_16:
      return{ 16, 2,DFT_UINT, {{ 0, 0, 16 },{ 1, 0, 16 }, BADELEM, BADELEM } };
   case latte::SQ_DATA_FORMAT::FMT_16_16_FLOAT:
      return{ 16, 2,DFT_FLOAT, {{ 0, 0, 16 },{ 1, 0, 16 }, BADELEM, BADELEM } };
   case latte::SQ_DATA_FORMAT::FMT_8_24:
      return{ 32, 1, DFT_UINT, {{0, 0, 8}, {0, 8, 24}, BADELEM, BADELEM } };
   case latte::SQ_DATA_FORMAT::FMT_8_24_FLOAT:
      return{ 32, 1, DFT_FLOAT, {{ 0, 0, 8 }, { 0, 8, 24 }, BADELEM, BADELEM } };
   case latte::SQ_DATA_FORMAT::FMT_24_8:
      return{ 32, 1, DFT_UINT, {{0, 0, 24}, {0, 24, 8}, BADELEM, BADELEM  } };
   case latte::SQ_DATA_FORMAT::FMT_24_8_FLOAT:
      return{ 32, 1, DFT_FLOAT, {{0, 0, 24}, {0, 24, 8}, BADELEM, BADELEM } };
 /*case latte::SQ_DATA_FORMAT::FMT_10_11_11:
      return{ 32, 1, DFT_UINT, {{0, 0, 11}, {0, 11, 11}, {0, 22, 10}, BADELEM } };*/
   case latte::SQ_DATA_FORMAT::FMT_10_11_11_FLOAT:
      return{ 32, 1, DFT_FLOAT,{{0, 0, 11}, {0, 11, 11}, {0, 22, 10}, BADELEM } };
   // This attribute format appears to oddly have the same layout as FMT_10_10_10_2?
   case latte::SQ_DATA_FORMAT::FMT_2_10_10_10:
      return{ 32, 1, DFT_UINT, {{0, 0, 10}, {0, 10, 10}, { 0, 20, 10 }, {0, 30, 2} } };
   case latte::SQ_DATA_FORMAT::FMT_8_8_8_8:
      return{ 32, 1, DFT_UINT,{{ 0, 0, 8 },{ 0, 8, 8 },{ 0, 16, 8 },{0, 24, 8 } } };
   case latte::SQ_DATA_FORMAT::FMT_10_10_10_2:
      return{ 32, 1, DFT_UINT, {{0, 0, 10}, {0, 10, 10}, {0, 20, 10}, {0, 30, 2} } };
   case latte::SQ_DATA_FORMAT::FMT_X24_8_32_FLOAT:
      return{ 32, 2, DFT_FLOAT, {{0, 24, 8}, {1, 0, 32}, BADELEM, BADELEM } };
   case latte::SQ_DATA_FORMAT::FMT_32_32:
      return{ 32, 2,DFT_UINT,{{ 0, 0, 32 },{ 1, 0, 32 }, BADELEM, BADELEM } };
   case latte::SQ_DATA_FORMAT::FMT_32_32_FLOAT:
      return{ 32, 2,DFT_FLOAT,{{ 0, 0, 32 },{ 1, 0, 32 }, BADELEM, BADELEM } };
   case latte::SQ_DATA_FORMAT::FMT_16_16_16_16:
      return{ 16, 4,DFT_UINT,{{ 0, 0, 16 },{ 1, 0, 16 },{ 2, 0, 16 },{ 3, 0, 16 }  } };
   case latte::SQ_DATA_FORMAT::FMT_16_16_16_16_FLOAT:
      return{ 16, 4,DFT_FLOAT,{{ 0, 0, 16 },{ 1, 0, 16 },{ 2, 0, 16 },{ 3, 0, 16 }  } };
   case latte::SQ_DATA_FORMAT::FMT_32_32_32_32:
      return{ 32, 4,DFT_UINT,{{ 0, 0, 32 },{ 1, 0, 32 },{ 2, 0, 32 },{ 3, 0, 32 } } };
   case latte::SQ_DATA_FORMAT::FMT_32_32_32_32_FLOAT:
      return{ 32, 4,DFT_FLOAT,{{ 0, 0, 32 },{ 1, 0, 32 },{ 2, 0, 32 },{ 3, 0, 32 }  } };
      //case latte::SQ_DATA_FORMAT::FMT_1:
      //case latte::SQ_DATA_FORMAT::FMT_GB_GR:
      //case latte::SQ_DATA_FORMAT::FMT_BG_RG:
      //case latte::SQ_DATA_FORMAT::FMT_32_AS_8:
      //case latte::SQ_DATA_FORMAT::FMT_32_AS_8_8:
      //case latte::SQ_DATA_FORMAT::FMT_5_9_9_9_SHAREDEXP:
   case latte::SQ_DATA_FORMAT::FMT_8_8_8:
      return{ 8, 3,DFT_UINT,{{ 0, 0, 8 },{ 1, 0, 8 },{ 2, 0, 8 }, BADELEM } };
   case latte::SQ_DATA_FORMAT::FMT_16_16_16:
      return{ 16, 3,DFT_UINT,{{ 0, 0, 16 },{ 1, 0, 16 },{ 2, 0, 16 }, BADELEM } };
   case latte::SQ_DATA_FORMAT::FMT_16_16_16_FLOAT:
      return{ 16, 3,DFT_FLOAT,{{ 0, 0, 16 },{ 1, 0, 16 },{ 2, 0, 16 }, BADELEM } };
   case latte::SQ_DATA_FORMAT::FMT_32_32_32:
      return{ 32, 3,DFT_UINT,{{ 0, 0, 32 },{ 1, 0, 32 },{ 2, 0, 32 }, BADELEM } };
   case latte::SQ_DATA_FORMAT::FMT_32_32_32_FLOAT:
      return{ 32, 3,DFT_FLOAT,{{ 0, 0, 32 },{ 1, 0, 32 },{ 2, 0, 32 }, BADELEM } };
      //case latte::SQ_DATA_FORMAT::FMT_BC1:
      //case latte::SQ_DATA_FORMAT::FMT_BC2:
      //case latte::SQ_DATA_FORMAT::FMT_BC3:
      //case latte::SQ_DATA_FORMAT::FMT_BC4:
      //case latte::SQ_DATA_FORMAT::FMT_BC5:
      //case latte::SQ_DATA_FORMAT::FMT_APC0:
      //case latte::SQ_DATA_FORMAT::FMT_APC1:
      //case latte::SQ_DATA_FORMAT::FMT_APC2:
      //case latte::SQ_DATA_FORMAT::FMT_APC3:
      //case latte::SQ_DATA_FORMAT::FMT_APC4:
      //case latte::SQ_DATA_FORMAT::FMT_APC5:
      //case latte::SQ_DATA_FORMAT::FMT_APC6:
      //case latte::SQ_DATA_FORMAT::FMT_APC7:
      //case latte::SQ_DATA_FORMAT::FMT_CTX1:
   default:
      decaf_abort(fmt::format("Unimplemented attribute format: {}", format));
   }
}

uint32_t
getDataFormatComponents(latte::SQ_DATA_FORMAT format)
{
   switch (format) {
   case latte::SQ_DATA_FORMAT::FMT_8:
   case latte::SQ_DATA_FORMAT::FMT_16:
   case latte::SQ_DATA_FORMAT::FMT_16_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_32:
   case latte::SQ_DATA_FORMAT::FMT_32_FLOAT:
      return 1;
   case latte::SQ_DATA_FORMAT::FMT_8_8:
   case latte::SQ_DATA_FORMAT::FMT_16_16:
   case latte::SQ_DATA_FORMAT::FMT_16_16_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_32_32:
   case latte::SQ_DATA_FORMAT::FMT_32_32_FLOAT:
      return 2;
   case latte::SQ_DATA_FORMAT::FMT_8_8_8:
   case latte::SQ_DATA_FORMAT::FMT_16_16_16:
   case latte::SQ_DATA_FORMAT::FMT_16_16_16_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_32_32_32:
   case latte::SQ_DATA_FORMAT::FMT_32_32_32_FLOAT:
      return 3;
   case latte::SQ_DATA_FORMAT::FMT_2_10_10_10:
   case latte::SQ_DATA_FORMAT::FMT_10_10_10_2:
   case latte::SQ_DATA_FORMAT::FMT_8_8_8_8:
   case latte::SQ_DATA_FORMAT::FMT_16_16_16_16:
   case latte::SQ_DATA_FORMAT::FMT_16_16_16_16_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_32_32_32_32:
   case latte::SQ_DATA_FORMAT::FMT_32_32_32_32_FLOAT:
      return 4;
   default:
      decaf_abort(fmt::format("Unimplemented attribute format: {}", format));
   }
}

uint32_t
getDataFormatComponentBits(latte::SQ_DATA_FORMAT format)
{
   switch (format) {
   case latte::SQ_DATA_FORMAT::FMT_8:
   case latte::SQ_DATA_FORMAT::FMT_8_8:
   case latte::SQ_DATA_FORMAT::FMT_8_8_8:
   case latte::SQ_DATA_FORMAT::FMT_8_8_8_8:
      return 8;
   case latte::SQ_DATA_FORMAT::FMT_16:
   case latte::SQ_DATA_FORMAT::FMT_16_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_16_16:
   case latte::SQ_DATA_FORMAT::FMT_16_16_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_16_16_16:
   case latte::SQ_DATA_FORMAT::FMT_16_16_16_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_16_16_16_16:
   case latte::SQ_DATA_FORMAT::FMT_16_16_16_16_FLOAT:
      return 16;
   case latte::SQ_DATA_FORMAT::FMT_32:
   case latte::SQ_DATA_FORMAT::FMT_32_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_32_32:
   case latte::SQ_DATA_FORMAT::FMT_32_32_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_32_32_32:
   case latte::SQ_DATA_FORMAT::FMT_32_32_32_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_32_32_32_32:
   case latte::SQ_DATA_FORMAT::FMT_32_32_32_32_FLOAT:
      return 32;
   default:
      decaf_abort(fmt::format("Unimplemented attribute format: {}", format));
   }
}

bool
getDataFormatIsFloat(latte::SQ_DATA_FORMAT format)
{
   switch (format) {

   case latte::SQ_DATA_FORMAT::FMT_16_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_32_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_16_16_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_32_32_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_16_16_16_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_32_32_32_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_16_16_16_16_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_32_32_32_32_FLOAT:
      return true;
   case latte::SQ_DATA_FORMAT::FMT_8:
   case latte::SQ_DATA_FORMAT::FMT_16:
   case latte::SQ_DATA_FORMAT::FMT_32:
   case latte::SQ_DATA_FORMAT::FMT_8_8:
   case latte::SQ_DATA_FORMAT::FMT_16_16:
   case latte::SQ_DATA_FORMAT::FMT_32_32:
   case latte::SQ_DATA_FORMAT::FMT_8_8_8:
   case latte::SQ_DATA_FORMAT::FMT_16_16_16:
   case latte::SQ_DATA_FORMAT::FMT_32_32_32:
   case latte::SQ_DATA_FORMAT::FMT_2_10_10_10:
   case latte::SQ_DATA_FORMAT::FMT_10_10_10_2:
   case latte::SQ_DATA_FORMAT::FMT_8_8_8_8:
   case latte::SQ_DATA_FORMAT::FMT_16_16_16_16:
   case latte::SQ_DATA_FORMAT::FMT_32_32_32_32:
      return false;
   default:
      decaf_abort(fmt::format("Unimplemented attribute format: {}", format));
   }
}

uint32_t
getTexDimDimensions(latte::SQ_TEX_DIM dim)
{
   switch (dim) {
   case latte::SQ_TEX_DIM::DIM_1D:
      return 1;
   case latte::SQ_TEX_DIM::DIM_2D:
   case latte::SQ_TEX_DIM::DIM_2D_MSAA:
   case latte::SQ_TEX_DIM::DIM_1D_ARRAY:
      return 2;
   case latte::SQ_TEX_DIM::DIM_2D_ARRAY:
   case latte::SQ_TEX_DIM::DIM_2D_ARRAY_MSAA:
   case latte::SQ_TEX_DIM::DIM_CUBEMAP:
   case latte::SQ_TEX_DIM::DIM_3D:
      return 3;
      break;
   default:
      decaf_abort(fmt::format("Unsupported texture dim: {}", dim));
   }
}

latte::SQ_TILE_MODE
getArrayModeTileMode(latte::BUFFER_ARRAY_MODE mode)
{
   // The buffer array modes match up with our SQ_TILE_MODE's perfectly.
   return static_cast<latte::SQ_TILE_MODE>(mode);
}

} // namespace latte
