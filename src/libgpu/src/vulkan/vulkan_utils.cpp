#ifdef DECAF_VULKAN
#include "vulkan_utils.h"

#include <common/decaf_assert.h>
#include <common/log.h>

namespace vulkan
{

vk::CompareOp
getVkCompareOp(latte::REF_FUNC func)
{
   switch (func) {
   case latte::REF_FUNC::NEVER:
      return vk::CompareOp::eNever;
   case latte::REF_FUNC::LESS:
      return vk::CompareOp::eLess;
   case latte::REF_FUNC::EQUAL:
      return vk::CompareOp::eEqual;
   case latte::REF_FUNC::LESS_EQUAL:
      return vk::CompareOp::eLessOrEqual;
   case latte::REF_FUNC::GREATER:
      return vk::CompareOp::eGreater;
   case latte::REF_FUNC::NOT_EQUAL:
      return vk::CompareOp::eNotEqual;
   case latte::REF_FUNC::GREATER_EQUAL:
      return vk::CompareOp::eGreaterOrEqual;
   case latte::REF_FUNC::ALWAYS:
      return vk::CompareOp::eAlways;
   default:
      decaf_abort("Unexpected compare op");
   }
}

vk::BlendFactor
getVkBlendFactor(latte::CB_BLEND_FUNC func)
{
   switch (func) {
   case latte::CB_BLEND_FUNC::ZERO:
      return vk::BlendFactor::eZero;
   case latte::CB_BLEND_FUNC::ONE:
      return vk::BlendFactor::eOne;
   case latte::CB_BLEND_FUNC::SRC_COLOR:
      return vk::BlendFactor::eSrcColor;
   case latte::CB_BLEND_FUNC::ONE_MINUS_SRC_COLOR:
      return vk::BlendFactor::eOneMinusSrcColor;
   case latte::CB_BLEND_FUNC::SRC_ALPHA:
      return vk::BlendFactor::eSrcAlpha;
   case latte::CB_BLEND_FUNC::ONE_MINUS_SRC_ALPHA:
      return vk::BlendFactor::eOneMinusSrcAlpha;
   case latte::CB_BLEND_FUNC::DST_ALPHA:
      return vk::BlendFactor::eDstAlpha;
   case latte::CB_BLEND_FUNC::ONE_MINUS_DST_ALPHA:
      return vk::BlendFactor::eOneMinusDstAlpha;
   case latte::CB_BLEND_FUNC::DST_COLOR:
      return vk::BlendFactor::eDstColor;
   case latte::CB_BLEND_FUNC::ONE_MINUS_DST_COLOR:
      return vk::BlendFactor::eOneMinusDstColor;
   case latte::CB_BLEND_FUNC::SRC_ALPHA_SATURATE:
      return vk::BlendFactor::eSrcAlphaSaturate;
   case latte::CB_BLEND_FUNC::BOTH_SRC_ALPHA:
      decaf_abort("Unsupported BOTH_SRC_ALPHA blend function");
   case latte::CB_BLEND_FUNC::BOTH_INV_SRC_ALPHA:
      decaf_abort("Unsupported BOTH_INV_SRC_ALPHA blend function");
   case latte::CB_BLEND_FUNC::CONSTANT_COLOR:
      return vk::BlendFactor::eConstantColor;
   case latte::CB_BLEND_FUNC::ONE_MINUS_CONSTANT_COLOR:
      return vk::BlendFactor::eOneMinusConstantColor;
   case latte::CB_BLEND_FUNC::SRC1_COLOR:
      return vk::BlendFactor::eSrc1Color;
   case latte::CB_BLEND_FUNC::ONE_MINUS_SRC1_COLOR:
      return vk::BlendFactor::eOneMinusSrc1Color;
   case latte::CB_BLEND_FUNC::SRC1_ALPHA:
      return vk::BlendFactor::eSrc1Alpha;
   case latte::CB_BLEND_FUNC::ONE_MINUS_SRC1_ALPHA:
      return vk::BlendFactor::eOneMinusSrc1Alpha;
   case latte::CB_BLEND_FUNC::CONSTANT_ALPHA:
      return vk::BlendFactor::eConstantAlpha;
   case latte::CB_BLEND_FUNC::ONE_MINUS_CONSTANT_ALPHA:
      return vk::BlendFactor::eOneMinusConstantAlpha;
   default:
      decaf_abort("Unexpected blend function");
   }
}

vk::BlendOp
getVkBlendOp(latte::CB_COMB_FUNC func)
{
   switch (func) {
   case latte::CB_COMB_FUNC::DST_PLUS_SRC:
      return vk::BlendOp::eAdd;
   case latte::CB_COMB_FUNC::SRC_MINUS_DST:
      return vk::BlendOp::eSubtract;
   case latte::CB_COMB_FUNC::MIN_DST_SRC:
      return vk::BlendOp::eMin;
   case latte::CB_COMB_FUNC::MAX_DST_SRC:
      return vk::BlendOp::eMax;
   case latte::CB_COMB_FUNC::DST_MINUS_SRC:
      return vk::BlendOp::eReverseSubtract;
   default:
      decaf_abort("Unexpected blend op");
   }
}

DataFormatUsage
getDataFormatUsageFlags(latte::SQ_DATA_FORMAT format)
{
   int flags = 0;

   switch (format) {
   case latte::SQ_DATA_FORMAT::FMT_8:
   case latte::SQ_DATA_FORMAT::FMT_4_4:
   case latte::SQ_DATA_FORMAT::FMT_3_3_2:
   case latte::SQ_DATA_FORMAT::FMT_16:
   case latte::SQ_DATA_FORMAT::FMT_16_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_8_8:
   case latte::SQ_DATA_FORMAT::FMT_5_6_5:
   case latte::SQ_DATA_FORMAT::FMT_6_5_5:
   case latte::SQ_DATA_FORMAT::FMT_1_5_5_5:
   case latte::SQ_DATA_FORMAT::FMT_4_4_4_4:
   case latte::SQ_DATA_FORMAT::FMT_5_5_5_1:
   case latte::SQ_DATA_FORMAT::FMT_32:
   case latte::SQ_DATA_FORMAT::FMT_16_16:
   case latte::SQ_DATA_FORMAT::FMT_16_16_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_10_11_11:
   case latte::SQ_DATA_FORMAT::FMT_10_11_11_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_11_11_10:
   case latte::SQ_DATA_FORMAT::FMT_11_11_10_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_2_10_10_10:
   case latte::SQ_DATA_FORMAT::FMT_8_8_8_8:
   case latte::SQ_DATA_FORMAT::FMT_10_10_10_2:
   case latte::SQ_DATA_FORMAT::FMT_32_32:
   case latte::SQ_DATA_FORMAT::FMT_32_32_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_16_16_16_16:
   case latte::SQ_DATA_FORMAT::FMT_16_16_16_16_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_32_32_32_32:
   case latte::SQ_DATA_FORMAT::FMT_32_32_32_32_FLOAT:
   //case latte::SQ_DATA_FORMAT::FMT_1:
   //case latte::SQ_DATA_FORMAT::FMT_GB_GR:
   //case latte::SQ_DATA_FORMAT::FMT_BG_RG:
   //case latte::SQ_DATA_FORMAT::FMT_32_AS_8:
   //case latte::SQ_DATA_FORMAT::FMT_32_AS_8_8:
   //case latte::SQ_DATA_FORMAT::FMT_5_9_9_9_SHAREDEXP:
   case latte::SQ_DATA_FORMAT::FMT_8_8_8:
   case latte::SQ_DATA_FORMAT::FMT_16_16_16:
   case latte::SQ_DATA_FORMAT::FMT_16_16_16_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_32_32_32:
   case latte::SQ_DATA_FORMAT::FMT_32_32_32_FLOAT:
      flags |= DataFormatUsage::FORMAT_MAYBE_COLOR;
      flags |= DataFormatUsage::FORMAT_ALLOW_RENDER_TARGET;
      break;
   case latte::SQ_DATA_FORMAT::FMT_32_FLOAT:
      flags |= DataFormatUsage::FORMAT_MAYBE_COLOR;
      flags |= DataFormatUsage::FORMAT_MAYBE_DEPTH;
      flags |= DataFormatUsage::FORMAT_ALLOW_RENDER_TARGET;
      break;
   case latte::SQ_DATA_FORMAT::FMT_BC1:
   case latte::SQ_DATA_FORMAT::FMT_BC2:
   case latte::SQ_DATA_FORMAT::FMT_BC3:
   case latte::SQ_DATA_FORMAT::FMT_BC4:
   case latte::SQ_DATA_FORMAT::FMT_BC5:
      flags |= DataFormatUsage::FORMAT_MAYBE_COLOR;
      break;
   case latte::SQ_DATA_FORMAT::FMT_8_24:
   case latte::SQ_DATA_FORMAT::FMT_8_24_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_24_8:
   case latte::SQ_DATA_FORMAT::FMT_24_8_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_X24_8_32_FLOAT:
      flags |= DataFormatUsage::FORMAT_ALLOW_RENDER_TARGET;
      flags |= DataFormatUsage::FORMAT_MAYBE_DEPTH;
      flags |= DataFormatUsage::FORMAT_MAYBE_STENCIL;
      break;
   default:
      decaf_abort("Unexpected texture format");
   }

   return DataFormatUsage(flags);
}

vk::SampleCountFlags
getVkSampleCount(uint32_t samples)
{
   switch (samples) {
   case 1:
      return vk::SampleCountFlagBits::e1;
   case 2:
      return vk::SampleCountFlagBits::e2;
   case 4:
      return vk::SampleCountFlagBits::e4;
   case 8:
      return vk::SampleCountFlagBits::e8;
   case 16:
      return vk::SampleCountFlagBits::e16;
   case 32:
      return vk::SampleCountFlagBits::e32;
   case 64:
      return vk::SampleCountFlagBits::e64;
   }
   decaf_abort("Unexpected surface samples value");
}

static vk::Format
getSurfaceFormat(latte::SQ_NUM_FORMAT numFormat,
                latte::SQ_FORMAT_COMP formatComp,
                uint32_t degamma,
                vk::Format unorm,
                vk::Format snorm,
                vk::Format uint,
                vk::Format sint,
                vk::Format srgb,
                vk::Format scaled)
{
   if (!degamma) {
      if (numFormat == latte::SQ_NUM_FORMAT::NORM) {
         if (formatComp == latte::SQ_FORMAT_COMP::SIGNED) {
            return snorm;
         } else if (formatComp == latte::SQ_FORMAT_COMP::UNSIGNED) {
            return unorm;
         } else {
            return vk::Format::eUndefined;
         }
      } else if (numFormat == latte::SQ_NUM_FORMAT::INT) {
         if (formatComp == latte::SQ_FORMAT_COMP::SIGNED) {
            return sint;
         } else if (formatComp == latte::SQ_FORMAT_COMP::UNSIGNED) {
            return uint;
         } else {
            return vk::Format::eUndefined;
         }
      } else if (numFormat == latte::SQ_NUM_FORMAT::SCALED) {
         if (formatComp == latte::SQ_FORMAT_COMP::UNSIGNED) {
            return scaled;
         } else {
            return vk::Format::eUndefined;
         }
      } else {
         return vk::Format::eUndefined;
      }
   } else {
      if (numFormat == 0 && formatComp == 0) {
         return srgb;
      } else {
         return vk::Format::eUndefined;
      }
   }
}

vk::Format
getSurfaceFormat(latte::SQ_DATA_FORMAT format,
                latte::SQ_NUM_FORMAT numFormat,
                latte::SQ_FORMAT_COMP formatComp,
                uint32_t degamma,
                bool forDepthBuffer)
{
   static const vk::Format BADFMT = vk::Format::eUndefined;
   auto pick = [=](vk::Format unorm, vk::Format snorm, vk::Format uint, vk::Format sint, vk::Format srgb, vk::Format scaled)
   {
      auto pickedFormat = getSurfaceFormat(numFormat, formatComp, degamma, unorm, snorm, uint, sint, srgb, scaled);
      if (pickedFormat == BADFMT) {
         decaf_abort(fmt::format("Failed to pick vulkan format for latte format: {} {} {} {}", format, numFormat, formatComp, degamma));
      }
      return pickedFormat;
   };

   bool isSigned = formatComp == latte::SQ_FORMAT_COMP::SIGNED;

   if (forDepthBuffer) {
      /*
      vk::Format::eD32Sfloat;
      vk::Format::eD32SfloatS8Uint
      vk::Format::eD16Unorm;
      vk::Format::eD16UnormS8Uint;
      vk::Format::eD24UnormS8Uint;
      */

      switch (format) {
      case latte::SQ_DATA_FORMAT::FMT_32_FLOAT:
         return pick(BADFMT, BADFMT, BADFMT, BADFMT, BADFMT, vk::Format::eD32Sfloat);
      case latte::SQ_DATA_FORMAT::FMT_8_24:
         return pick(vk::Format::eD24UnormS8Uint, BADFMT, BADFMT, BADFMT, BADFMT, BADFMT);

      default:
         decaf_abort("Unexpected depth buffer format for vulkan");
      }
   } else {
      switch (format) {
      case latte::SQ_DATA_FORMAT::FMT_8:
         return pick(vk::Format::eR8Unorm, vk::Format::eR8Snorm, vk::Format::eR8Uint, vk::Format::eR8Sint, vk::Format::eR8Srgb, vk::Format::eR8Sscaled);
      //case latte::SQ_DATA_FORMAT::FMT_4_4:
      //case latte::SQ_DATA_FORMAT::FMT_3_3_2:
      case latte::SQ_DATA_FORMAT::FMT_16:
         return pick(vk::Format::eR16Unorm, vk::Format::eR16Snorm, vk::Format::eR16Uint, vk::Format::eR16Sint, BADFMT, vk::Format::eR16Sscaled);
      case latte::SQ_DATA_FORMAT::FMT_16_FLOAT:
         return pick(BADFMT, BADFMT, BADFMT, BADFMT, BADFMT, vk::Format::eR16Sfloat);
      case latte::SQ_DATA_FORMAT::FMT_8_8:
         return pick(vk::Format::eR8G8Unorm, vk::Format::eR8G8Snorm, vk::Format::eR8G8Uint, vk::Format::eR8G8Sint, vk::Format::eR8G8Srgb, vk::Format::eR8G8Sscaled);
      case latte::SQ_DATA_FORMAT::FMT_5_6_5:
         return pick(vk::Format::eB5G6R5UnormPack16, BADFMT, BADFMT, BADFMT, BADFMT, BADFMT);
      //case latte::SQ_DATA_FORMAT::FMT_6_5_5:
      //case latte::SQ_DATA_FORMAT::FMT_1_5_5_5:
      //case latte::SQ_DATA_FORMAT::FMT_4_4_4_4:
      //case latte::SQ_DATA_FORMAT::FMT_5_5_5_1:
      case latte::SQ_DATA_FORMAT::FMT_32:
         return pick(BADFMT, BADFMT, vk::Format::eR32Uint, vk::Format::eR32Sint, BADFMT, BADFMT);
      case latte::SQ_DATA_FORMAT::FMT_32_FLOAT:
         return pick(BADFMT, BADFMT, BADFMT, BADFMT, BADFMT, vk::Format::eR32Sfloat);
      case latte::SQ_DATA_FORMAT::FMT_16_16:
         return pick(vk::Format::eR16G16Unorm, vk::Format::eR16G16Snorm, vk::Format::eR16G16Uint, vk::Format::eR16G16Sint, BADFMT, BADFMT);
      case latte::SQ_DATA_FORMAT::FMT_16_16_FLOAT:
         return pick(BADFMT, BADFMT, BADFMT, BADFMT, BADFMT, vk::Format::eR16G16Sfloat);
      //case latte::SQ_DATA_FORMAT::FMT_8_24:
      //case latte::SQ_DATA_FORMAT::FMT_8_24_FLOAT:
      case latte::SQ_DATA_FORMAT::FMT_24_8:
         return pick(BADFMT, BADFMT, BADFMT, BADFMT, BADFMT, BADFMT);
      //case latte::SQ_DATA_FORMAT::FMT_24_8_FLOAT:
      //case latte::SQ_DATA_FORMAT::FMT_10_11_11:
      case latte::SQ_DATA_FORMAT::FMT_10_11_11_FLOAT:
         decaf_abort("Encountered bit-reversed surface format");
         //return pick(BADFMT, BADFMT, BADFMT, BADFMT, BADFMT, vk::Format::eB10G11R11UfloatPack32);
      //case latte::SQ_DATA_FORMAT::FMT_11_11_10:
      case latte::SQ_DATA_FORMAT::FMT_11_11_10_FLOAT:
         decaf_abort("Encountered bit-reversed surface format");
         //return pick(BADFMT, BADFMT, BADFMT, BADFMT, BADFMT, vk::Format::eB10G11R11UfloatPack32);
      case latte::SQ_DATA_FORMAT::FMT_2_10_10_10:
         decaf_abort("Encountered bit-reversed surface format");
         //return pick(vk::Format::eA2B10G10R10UnormPack32, vk::Format::eA2B10G10R10SnormPack32, vk::Format::eA2B10G10R10UintPack32, vk::Format::eA2B10G10R10UnormPack32, BADFMT, BADFMT);
      case latte::SQ_DATA_FORMAT::FMT_8_8_8_8:
         return pick(vk::Format::eR8G8B8A8Unorm, vk::Format::eR8G8B8A8Snorm, vk::Format::eR8G8B8A8Uint, vk::Format::eR8G8B8A8Sint, vk::Format::eR8G8B8A8Srgb, vk::Format::eR8G8B8A8Sscaled);
      case latte::SQ_DATA_FORMAT::FMT_10_10_10_2:
         return pick(vk::Format::eA2B10G10R10UnormPack32, vk::Format::eA2B10G10R10SnormPack32, vk::Format::eA2B10G10R10UintPack32, vk::Format::eA2B10G10R10SintPack32, BADFMT, vk::Format::eA2B10G10R10SscaledPack32);
      //case latte::SQ_DATA_FORMAT::FMT_X24_8_32_FLOAT:
      case latte::SQ_DATA_FORMAT::FMT_32_32:
         return pick(BADFMT, BADFMT, vk::Format::eR32G32Uint, vk::Format::eR32G32Sint, BADFMT, BADFMT);
      case latte::SQ_DATA_FORMAT::FMT_32_32_FLOAT:
         return pick(BADFMT, BADFMT, BADFMT, BADFMT, BADFMT, vk::Format::eR32G32Sfloat);
      case latte::SQ_DATA_FORMAT::FMT_16_16_16_16:
         return pick(vk::Format::eR16G16B16A16Unorm, vk::Format::eR16G16B16A16Snorm, vk::Format::eR16G16B16A16Uint, vk::Format::eR16G16B16A16Sint, BADFMT, BADFMT);
      case latte::SQ_DATA_FORMAT::FMT_16_16_16_16_FLOAT:
         return pick(BADFMT, BADFMT, BADFMT, BADFMT, BADFMT, vk::Format::eR16G16B16A16Sfloat);
      case latte::SQ_DATA_FORMAT::FMT_32_32_32_32:
         return pick(BADFMT, BADFMT, vk::Format::eR32G32B32Uint, vk::Format::eR32G32B32Sint, BADFMT, BADFMT);
      case latte::SQ_DATA_FORMAT::FMT_32_32_32_32_FLOAT:
         return pick(BADFMT, BADFMT, BADFMT, BADFMT, BADFMT, vk::Format::eR32G32B32Sfloat);
      //case latte::SQ_DATA_FORMAT::FMT_1:
      //case latte::SQ_DATA_FORMAT::FMT_GB_GR:
      //case latte::SQ_DATA_FORMAT::FMT_BG_RG:
      //case latte::SQ_DATA_FORMAT::FMT_32_AS_8:
      //case latte::SQ_DATA_FORMAT::FMT_32_AS_8_8:
      //case latte::SQ_DATA_FORMAT::FMT_5_9_9_9_SHAREDEXP:
      //case latte::SQ_DATA_FORMAT::FMT_8_8_8:
      //case latte::SQ_DATA_FORMAT::FMT_16_16_16:
      //case latte::SQ_DATA_FORMAT::FMT_16_16_16_FLOAT:
      //case latte::SQ_DATA_FORMAT::FMT_32_32_32:
      //case latte::SQ_DATA_FORMAT::FMT_32_32_32_FLOAT:
      case latte::SQ_DATA_FORMAT::FMT_BC1:

         return pick(vk::Format::eBc1RgbaUnormBlock, BADFMT, BADFMT, BADFMT, vk::Format::eBc1RgbaSrgbBlock, BADFMT);
      case latte::SQ_DATA_FORMAT::FMT_BC2:
         return pick(vk::Format::eBc2UnormBlock, BADFMT, BADFMT, BADFMT, vk::Format::eBc2SrgbBlock, BADFMT);
      case latte::SQ_DATA_FORMAT::FMT_BC3:
         return pick(vk::Format::eBc3UnormBlock, BADFMT, BADFMT, BADFMT, vk::Format::eBc3SrgbBlock, BADFMT);
      case latte::SQ_DATA_FORMAT::FMT_BC4:
         return pick(vk::Format::eBc4UnormBlock, vk::Format::eBc4SnormBlock, BADFMT, BADFMT, BADFMT, BADFMT);
      case latte::SQ_DATA_FORMAT::FMT_BC5:
         return pick(vk::Format::eBc5UnormBlock, vk::Format::eBc5SnormBlock, BADFMT, BADFMT, BADFMT, BADFMT);
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
         decaf_abort("Unexpected color buffer format for vulkan");
      }
   }
}

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
