#include "latte/latte_formats.h"

#include <common/decaf_assert.h>
#include <common/log.h>
#include <fmt/format.h>

namespace latte
{

DataFormat
getColorBufferDataFormat(latte::CB_FORMAT format, latte::CB_NUMBER_TYPE numberType)
{
   DataFormat formatOut;

   // This is safe only becase CB_FORMAT perfectly overlays SQ_DATA_FORMAT
   formatOut.format = static_cast<latte::SQ_DATA_FORMAT>(format);

   switch (numberType) {
   case latte::CB_NUMBER_TYPE::UNORM:
      formatOut.numFormat = latte::SQ_NUM_FORMAT::NORM;
      formatOut.formatComp = latte::SQ_FORMAT_COMP::UNSIGNED;
      formatOut.degamma = 0;
      break;
   case latte::CB_NUMBER_TYPE::SNORM:
      formatOut.numFormat = latte::SQ_NUM_FORMAT::NORM;
      formatOut.formatComp = latte::SQ_FORMAT_COMP::SIGNED;
      formatOut.degamma = 0;
      break;
   case latte::CB_NUMBER_TYPE::UINT:
      formatOut.numFormat = latte::SQ_NUM_FORMAT::INT;
      formatOut.formatComp = latte::SQ_FORMAT_COMP::UNSIGNED;
      formatOut.degamma = 0;
      break;
   case latte::CB_NUMBER_TYPE::SINT:
      formatOut.numFormat = latte::SQ_NUM_FORMAT::INT;
      formatOut.formatComp = latte::SQ_FORMAT_COMP::SIGNED;
      formatOut.degamma = 0;
      break;
   case latte::CB_NUMBER_TYPE::FLOAT:
      formatOut.numFormat = latte::SQ_NUM_FORMAT::SCALED;
      formatOut.formatComp = latte::SQ_FORMAT_COMP::UNSIGNED;
      formatOut.degamma = 0;
      break;
   case latte::CB_NUMBER_TYPE::SRGB:
      formatOut.numFormat = latte::SQ_NUM_FORMAT::NORM;
      formatOut.formatComp = latte::SQ_FORMAT_COMP::UNSIGNED;
      formatOut.degamma = 1;
      break;
   default:
      decaf_abort(fmt::format("Color buffer with unsupported number type {}", numberType));
   }

   return formatOut;
}

DataFormat
getDepthBufferDataFormat(latte::DB_FORMAT format)
{
   DataFormat formatOut;

   switch (format) {
   case latte::DB_FORMAT::DEPTH_16:
      formatOut.format = latte::SQ_DATA_FORMAT::FMT_16;
      formatOut.numFormat = latte::SQ_NUM_FORMAT::NORM;
      formatOut.formatComp = latte::SQ_FORMAT_COMP::UNSIGNED;
      formatOut.degamma = 0;
      break;
   case latte::DB_FORMAT::DEPTH_X8_24:
      formatOut.format = latte::SQ_DATA_FORMAT::FMT_8_24;
      formatOut.numFormat = latte::SQ_NUM_FORMAT::SCALED;
      formatOut.formatComp = latte::SQ_FORMAT_COMP::UNSIGNED;
      formatOut.degamma = 0;
      break;
   case latte::DB_FORMAT::DEPTH_8_24:
      formatOut.format = latte::SQ_DATA_FORMAT::FMT_8_24;
      formatOut.numFormat = latte::SQ_NUM_FORMAT::NORM;
      formatOut.formatComp = latte::SQ_FORMAT_COMP::UNSIGNED;
      formatOut.degamma = 0;
      break;
   case latte::DB_FORMAT::DEPTH_X8_24_FLOAT:
      formatOut.format = latte::SQ_DATA_FORMAT::FMT_8_24_FLOAT;
      formatOut.numFormat = latte::SQ_NUM_FORMAT::SCALED;
      formatOut.formatComp = latte::SQ_FORMAT_COMP::UNSIGNED;
      formatOut.degamma = 0;
      break;
   case latte::DB_FORMAT::DEPTH_8_24_FLOAT:
      formatOut.format = latte::SQ_DATA_FORMAT::FMT_8_24_FLOAT;
      formatOut.numFormat = latte::SQ_NUM_FORMAT::SCALED;
      formatOut.formatComp = latte::SQ_FORMAT_COMP::UNSIGNED;
      formatOut.degamma = 0;
      break;
   case latte::DB_FORMAT::DEPTH_32_FLOAT:
      formatOut.format = latte::SQ_DATA_FORMAT::FMT_32_FLOAT;
      formatOut.numFormat = latte::SQ_NUM_FORMAT::SCALED;
      formatOut.formatComp = latte::SQ_FORMAT_COMP::UNSIGNED;
      formatOut.degamma = 0;
      break;
   case latte::DB_FORMAT::DEPTH_X24_8_32_FLOAT:
      formatOut.format = latte::SQ_DATA_FORMAT::FMT_X24_8_32_FLOAT;
      formatOut.numFormat = latte::SQ_NUM_FORMAT::SCALED;
      formatOut.formatComp = latte::SQ_FORMAT_COMP::UNSIGNED;
      formatOut.degamma = 0;
      break;
   default:
      decaf_abort(fmt::format("Depth buffer with unsupported format {}", format));
   }

   return formatOut;
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
   switch (mode) {
   case latte::BUFFER_ARRAY_MODE::LINEAR_GENERAL:
      return latte::SQ_TILE_MODE::DEFAULT;
   case latte::BUFFER_ARRAY_MODE::LINEAR_ALIGNED:
      return latte::SQ_TILE_MODE::LINEAR_ALIGNED;
   case latte::BUFFER_ARRAY_MODE::TILED_2D_THIN1:
      return latte::SQ_TILE_MODE::TILED_2D_THIN1;
   default:
      decaf_abort(fmt::format("Unimplemented surface array mode: {}", mode));
   }
}

} // namespace latte
