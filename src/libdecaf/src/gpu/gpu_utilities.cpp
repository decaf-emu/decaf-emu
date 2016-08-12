#include "common/decaf_assert.h"
#include "common/log.h"
#include "gpu_utilities.h"

uint32_t
getDataFormatBitsPerElement(latte::SQ_DATA_FORMAT format)
{
   switch (format) {
   case latte::FMT_8:
   case latte::FMT_3_3_2:
      return 8;

   case latte::FMT_8_8:
   case latte::FMT_16:
   case latte::FMT_16_FLOAT:
   case latte::FMT_5_6_5:
   case latte::FMT_5_5_5_1:
   case latte::FMT_1_5_5_5:
   case latte::FMT_4_4_4_4:
      return 16;

   case latte::FMT_8_8_8:
      return 24;

   case latte::FMT_8_8_8_8:
   case latte::FMT_16_16:
   case latte::FMT_16_16_FLOAT:
   case latte::FMT_32:
   case latte::FMT_32_FLOAT:
   case latte::FMT_10_10_10_2:
   case latte::FMT_2_10_10_10:
   case latte::FMT_10_11_11:
   case latte::FMT_10_11_11_FLOAT:
   case latte::FMT_11_11_10:
   case latte::FMT_11_11_10_FLOAT:
   case latte::FMT_8_24:
   case latte::FMT_8_24_FLOAT:
      return 32;

   case latte::FMT_16_16_16:
   case latte::FMT_16_16_16_FLOAT:
      return 48;

   case latte::FMT_16_16_16_16:
   case latte::FMT_16_16_16_16_FLOAT:
   case latte::FMT_32_32:
   case latte::FMT_32_32_FLOAT:
   case latte::FMT_BC1:
   case latte::FMT_BC4:
      return 64;

   case latte::FMT_32_32_32:
   case latte::FMT_32_32_32_FLOAT:
      return 96;

   case latte::FMT_32_32_32_32:
   case latte::FMT_32_32_32_32_FLOAT:
   case latte::FMT_BC2:
   case latte::FMT_BC3:
   case latte::FMT_BC5:
      return 128;

   default:
      decaf_abort(fmt::format("Unimplemented data format {}", format));
   }
}

bool
getDataFormatIsCompressed(latte::SQ_DATA_FORMAT format)
{
   switch (format) {
   case latte::FMT_BC1:
   case latte::FMT_BC2:
   case latte::FMT_BC3:
   case latte::FMT_BC4:
   case latte::FMT_BC5:
      return true;
   default:
      return false;
   }
}

std::string
getDataFormatName(latte::SQ_DATA_FORMAT format)
{
   switch (format) {
   case latte::FMT_8:
      return "FMT_8";
   case latte::FMT_16:
      return "FMT_16";
   case latte::FMT_16_FLOAT:
      return "FMT_16_FLOAT";
   case latte::FMT_32:
      return "FMT_32";
   case latte::FMT_32_FLOAT:
      return "FMT_32_FLOAT";
   case latte::FMT_8_8:
      return "FMT_8_8";
   case latte::FMT_16_16:
      return "FMT_16_16";
   case latte::FMT_16_16_FLOAT:
      return "FMT_16_16_FLOAT";
   case latte::FMT_32_32:
      return "FMT_32_32";
   case latte::FMT_32_32_FLOAT:
      return "FMT_32_32_FLOAT";
   case latte::FMT_8_8_8:
      return "FMT_8_8_8";
   case latte::FMT_16_16_16:
      return "FMT_16_16_16";
   case latte::FMT_16_16_16_FLOAT:
      return "FMT_16_16_16_FLOAT";
   case latte::FMT_32_32_32:
      return "FMT_32_32_32";
   case latte::FMT_32_32_32_FLOAT:
      return "FMT_32_32_32_FLOAT";
   case latte::FMT_8_8_8_8:
      return "FMT_8_8_8_8";
   case latte::FMT_16_16_16_16:
      return "FMT_16_16_16_16";
   case latte::FMT_16_16_16_16_FLOAT:
      return "FMT_16_16_16_16_FLOAT";
   case latte::FMT_32_32_32_32:
      return "FMT_32_32_32_32";
   case latte::FMT_32_32_32_32_FLOAT:
      return "FMT_32_32_32_32_FLOAT";
   case latte::FMT_2_10_10_10:
      return "FMT_2_10_10_10";
   case latte::FMT_10_10_10_2:
      return "FMT_10_10_10_2";
   default:
      decaf_abort(fmt::format("Unimplemented attribute format: {}", format));
   }
}

uint32_t
getDataFormatComponents(latte::SQ_DATA_FORMAT format)
{
   switch (format) {
   case latte::FMT_8:
   case latte::FMT_16:
   case latte::FMT_16_FLOAT:
   case latte::FMT_32:
   case latte::FMT_32_FLOAT:
      return 1;
   case latte::FMT_8_8:
   case latte::FMT_16_16:
   case latte::FMT_16_16_FLOAT:
   case latte::FMT_32_32:
   case latte::FMT_32_32_FLOAT:
      return 2;
   case latte::FMT_8_8_8:
   case latte::FMT_16_16_16:
   case latte::FMT_16_16_16_FLOAT:
   case latte::FMT_32_32_32:
   case latte::FMT_32_32_32_FLOAT:
      return 3;
   case latte::FMT_2_10_10_10:
   case latte::FMT_10_10_10_2:
   case latte::FMT_8_8_8_8:
   case latte::FMT_16_16_16_16:
   case latte::FMT_16_16_16_16_FLOAT:
   case latte::FMT_32_32_32_32:
   case latte::FMT_32_32_32_32_FLOAT:
      return 4;
   default:
      decaf_abort(fmt::format("Unimplemented attribute format: {}", format));
   }
}

uint32_t
getDataFormatComponentBits(latte::SQ_DATA_FORMAT format)
{
   switch (format) {
   case latte::FMT_8:
   case latte::FMT_8_8:
   case latte::FMT_8_8_8:
   case latte::FMT_8_8_8_8:
      return 8;
   case latte::FMT_16:
   case latte::FMT_16_FLOAT:
   case latte::FMT_16_16:
   case latte::FMT_16_16_FLOAT:
   case latte::FMT_16_16_16:
   case latte::FMT_16_16_16_FLOAT:
   case latte::FMT_16_16_16_16:
   case latte::FMT_16_16_16_16_FLOAT:
      return 16;
   case latte::FMT_32:
   case latte::FMT_32_FLOAT:
   case latte::FMT_32_32:
   case latte::FMT_32_32_FLOAT:
   case latte::FMT_32_32_32:
   case latte::FMT_32_32_32_FLOAT:
   case latte::FMT_32_32_32_32:
   case latte::FMT_32_32_32_32_FLOAT:
      return 32;
   default:
      decaf_abort(fmt::format("Unimplemented attribute format: {}", format));
   }
}

bool
getDataFormatIsFloat(latte::SQ_DATA_FORMAT format)
{
   switch (format) {

   case latte::FMT_16_FLOAT:
   case latte::FMT_32_FLOAT:
   case latte::FMT_16_16_FLOAT:
   case latte::FMT_32_32_FLOAT:
   case latte::FMT_16_16_16_FLOAT:
   case latte::FMT_32_32_32_FLOAT:
   case latte::FMT_16_16_16_16_FLOAT:
   case latte::FMT_32_32_32_32_FLOAT:
      return true;
   case latte::FMT_8:
   case latte::FMT_16:
   case latte::FMT_32:
   case latte::FMT_8_8:
   case latte::FMT_16_16:
   case latte::FMT_32_32:
   case latte::FMT_8_8_8:
   case latte::FMT_16_16_16:
   case latte::FMT_32_32_32:
   case latte::FMT_2_10_10_10:
   case latte::FMT_10_10_10_2:
   case latte::FMT_8_8_8_8:
   case latte::FMT_16_16_16_16:
   case latte::FMT_32_32_32_32:
      return false;
   default:
      decaf_abort(fmt::format("Unimplemented attribute format: {}", format));
   }
}
