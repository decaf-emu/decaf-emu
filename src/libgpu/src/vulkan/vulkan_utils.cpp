#ifdef DECAF_VULKAN
#include "vulkan_utils.h"

#include "common/decaf_assert.h"
#include "common/log.h"

namespace vulkan
{

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
   case latte::SQ_DATA_FORMAT::FMT_32_FLOAT:
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
      flags |= DataFormatUsage::FORMAT_HAS_COLOR;
      flags |= DataFormatUsage::FORMAT_ALLOW_RENDER_TARGET;
      break;
   case latte::SQ_DATA_FORMAT::FMT_BC1:
   case latte::SQ_DATA_FORMAT::FMT_BC2:
   case latte::SQ_DATA_FORMAT::FMT_BC3:
   case latte::SQ_DATA_FORMAT::FMT_BC4:
   case latte::SQ_DATA_FORMAT::FMT_BC5:
      flags |= DataFormatUsage::FORMAT_HAS_COLOR;
      break;
   case latte::SQ_DATA_FORMAT::FMT_8_24:
   case latte::SQ_DATA_FORMAT::FMT_8_24_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_24_8:
   case latte::SQ_DATA_FORMAT::FMT_24_8_FLOAT:
   case latte::SQ_DATA_FORMAT::FMT_X24_8_32_FLOAT:
      flags |= DataFormatUsage::FORMAT_HAS_DEPTH;
      flags |= DataFormatUsage::FORMAT_HAS_STENCIL;
      break;
   default:
      decaf_abort("Unexpected texture format");
   }

   return DataFormatUsage(flags);
}

std::pair<vk::ImageType, uint32_t>
getSurfaceTypeInfo(latte::SQ_TEX_DIM dim, uint32_t height, uint32_t depth)
{
   switch (dim) {
   case latte::SQ_TEX_DIM::DIM_1D:
      return { vk::ImageType::e1D, 1 };
   case latte::SQ_TEX_DIM::DIM_2D:
      return { vk::ImageType::e2D, 1 };
   case latte::SQ_TEX_DIM::DIM_3D:
      return { vk::ImageType::e3D, 1 };
   case latte::SQ_TEX_DIM::DIM_CUBEMAP:
      decaf_check(depth % 6 == 0);
      return { vk::ImageType::e2D, depth / 6 };
   case latte::SQ_TEX_DIM::DIM_1D_ARRAY:
      return { vk::ImageType::e1D, height };
   case latte::SQ_TEX_DIM::DIM_2D_ARRAY:
      return { vk::ImageType::e2D, depth };
   case latte::SQ_TEX_DIM::DIM_2D_MSAA:
      return { vk::ImageType::e2D, 1 };
   case latte::SQ_TEX_DIM::DIM_2D_ARRAY_MSAA:
      return { vk::ImageType::e2D, depth };
   }

   decaf_abort(fmt::format("Failed to pick dx dim for latte dim {}", dim));
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
                uint32_t degamma)
{
   static const vk::Format BADFMT = vk::Format::eUndefined;
   auto pick = [=](vk::Format unorm, vk::Format snorm, vk::Format uint, vk::Format sint, vk::Format srgb, vk::Format scaled)
   {
      auto pickedFormat = getSurfaceFormat(numFormat, formatComp, degamma, unorm, snorm, uint, sint, srgb, scaled);
      if (pickedFormat == BADFMT) {
         decaf_abort(fmt::format("Failed to pick dx format for latte format: {} {} {} {}", format, numFormat, formatComp, degamma));
      }
      return pickedFormat;
   };

   bool isSigned = formatComp == latte::SQ_FORMAT_COMP::SIGNED;

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
      return pick(vk::Format::eD24UnormS8Uint, BADFMT, BADFMT, BADFMT, BADFMT, BADFMT);
   //case latte::SQ_DATA_FORMAT::FMT_24_8_FLOAT:
   //case latte::SQ_DATA_FORMAT::FMT_10_11_11:
   case latte::SQ_DATA_FORMAT::FMT_10_11_11_FLOAT:
   // TODO: This is not strictly correct, but may work for now... (Reversed bit order)
      return pick(BADFMT, BADFMT, BADFMT, BADFMT, BADFMT, vk::Format::eB10G11R11UfloatPack32);
   //case latte::SQ_DATA_FORMAT::FMT_11_11_10:
   case latte::SQ_DATA_FORMAT::FMT_11_11_10_FLOAT:
   // TODO: This is not strictly correct, but may work for now... (Reversed bit order)
      return pick(BADFMT, BADFMT, BADFMT, BADFMT, BADFMT, vk::Format::eB10G11R11UfloatPack32);
   case latte::SQ_DATA_FORMAT::FMT_2_10_10_10:
   // TODO: This is not strictly correct, but may work for now... (Reversed bit order)
      return pick(vk::Format::eA2B10G10R10UnormPack32, vk::Format::eA2B10G10R10SnormPack32, vk::Format::eA2B10G10R10UintPack32, vk::Format::eA2B10G10R10UnormPack32, BADFMT, BADFMT);
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
   }

   return pick(BADFMT, BADFMT, BADFMT, BADFMT, BADFMT, BADFMT);
}

} // namespace vulkan

#endif // DECAF_VULKAN
