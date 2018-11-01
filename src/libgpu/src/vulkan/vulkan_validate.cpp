#ifdef DECAF_VULKAN
#include "vulkan_driver.h"
#include "vulkan_utils.h"

namespace vulkan
{

auto supportedColorFormats = {
   latte::SurfaceFormat::R8Unorm,
   latte::SurfaceFormat::R8Uint,
   latte::SurfaceFormat::R8Snorm,
   latte::SurfaceFormat::R8Sint,
   latte::SurfaceFormat::R4G4Unorm,
   latte::SurfaceFormat::R16Unorm,
   latte::SurfaceFormat::R16Uint,
   latte::SurfaceFormat::R16Snorm,
   latte::SurfaceFormat::R16Sint,
   latte::SurfaceFormat::R16Float,
   latte::SurfaceFormat::R8G8Unorm,
   latte::SurfaceFormat::R8G8Uint,
   latte::SurfaceFormat::R8G8Snorm,
   latte::SurfaceFormat::R8G8Sint,
   latte::SurfaceFormat::R5G6B5Unorm,
   latte::SurfaceFormat::R5G5B5A1Unorm,
   latte::SurfaceFormat::R4G4B4A4Unorm,
   latte::SurfaceFormat::A1B5G5R5Unorm,
   latte::SurfaceFormat::R32Uint,
   latte::SurfaceFormat::R32Sint,
   latte::SurfaceFormat::R32Float,
   latte::SurfaceFormat::R16G16Unorm,
   latte::SurfaceFormat::R16G16Uint,
   latte::SurfaceFormat::R16G16Snorm,
   latte::SurfaceFormat::R16G16Sint,
   latte::SurfaceFormat::R16G16Float,
   latte::SurfaceFormat::D24UnormS8Uint,
   latte::SurfaceFormat::X24G8Uint,
   latte::SurfaceFormat::R11G11B10Float,
   latte::SurfaceFormat::R10G10B10A2Unorm,
   latte::SurfaceFormat::R10G10B10A2Uint,
   latte::SurfaceFormat::R10G10B10A2Snorm,
   latte::SurfaceFormat::R10G10B10A2Sint,
   latte::SurfaceFormat::R8G8B8A8Unorm,
   latte::SurfaceFormat::R8G8B8A8Uint,
   latte::SurfaceFormat::R8G8B8A8Snorm,
   latte::SurfaceFormat::R8G8B8A8Sint,
   latte::SurfaceFormat::R8G8B8A8Srgb,
   latte::SurfaceFormat::A2B10G10R10Unorm,
   latte::SurfaceFormat::A2B10G10R10Uint,
   latte::SurfaceFormat::D32FloatS8UintX24,
   latte::SurfaceFormat::D32G8UintX24,
   latte::SurfaceFormat::R32G32Uint,
   latte::SurfaceFormat::R32G32Sint,
   latte::SurfaceFormat::R32G32Float,
   latte::SurfaceFormat::R16G16B16A16Unorm,
   latte::SurfaceFormat::R16G16B16A16Uint,
   latte::SurfaceFormat::R16G16B16A16Snorm,
   latte::SurfaceFormat::R16G16B16A16Sint,
   latte::SurfaceFormat::R16G16B16A16Float,
   latte::SurfaceFormat::R32G32B32A32Uint,
   latte::SurfaceFormat::R32G32B32A32Sint,
   latte::SurfaceFormat::R32G32B32A32Float,
   latte::SurfaceFormat::BC1Unorm,
   latte::SurfaceFormat::BC1Srgb,
   latte::SurfaceFormat::BC2Unorm,
   latte::SurfaceFormat::BC2Srgb,
   latte::SurfaceFormat::BC3Unorm,
   latte::SurfaceFormat::BC3Srgb,
   latte::SurfaceFormat::BC4Unorm,
   latte::SurfaceFormat::BC4Snorm,
   latte::SurfaceFormat::BC5Unorm,
   latte::SurfaceFormat::BC5Snorm,
   //latte::SurfaceFormat::NV12,
};

auto supportedDepthFormats = {
   latte::SurfaceFormat::R16Unorm,
   latte::SurfaceFormat::R32Float,
   latte::SurfaceFormat::D24UnormS8Uint,
   latte::SurfaceFormat::X24G8Uint,
   latte::SurfaceFormat::D32FloatS8UintX24,
   latte::SurfaceFormat::D32G8UintX24,
};

void
Driver::validateDevice()
{
   auto checkFormat = [&](latte::SurfaceFormat format, latte::SQ_TILE_TYPE tileType)
   {
      auto hostFormat = getVkSurfaceFormat(format, tileType);
      auto formatUsages = getVkSurfaceFormatUsage(format);

      auto formatProps = mPhysDevice.getFormatProperties(hostFormat);

      if (!(formatProps.optimalTilingFeatures & vk::FormatFeatureFlagBits::eTransferDst)) {
         decaf_abort(fmt::format("Surface format {:03x}[{}]({}) does not support TransferDst feature",
                                 format, tileType, vk::to_string(hostFormat)));
      }
      if (!(formatProps.optimalTilingFeatures & vk::FormatFeatureFlagBits::eTransferSrc)) {
         decaf_abort(fmt::format("Surface format {:03x}[{}]({}) does not support TransferSrc feature",
                                 format, tileType, vk::to_string(hostFormat)));
      }

      if (tileType != latte::SQ_TILE_TYPE::DEPTH) {
         if (formatUsages & SurfaceFormatUsage::TEXTURE) {
            if (!(formatProps.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImage)) {
               decaf_abort(fmt::format("Surface format {:03x}[{}]({}) does not support SampledImage feature",
                                       format, tileType, vk::to_string(hostFormat)));
            }
         }
         if (formatUsages & SurfaceFormatUsage::COLOR) {
            if (!(formatProps.optimalTilingFeatures & vk::FormatFeatureFlagBits::eColorAttachment)) {
               decaf_abort(fmt::format("Surface format {:03x}[{}]({}) does not support ColorAttachment feature",
                                       format, tileType, vk::to_string(hostFormat)));
            }
         }
      } else {
         if (formatUsages & (SurfaceFormatUsage::DEPTH | SurfaceFormatUsage::STENCIL)) {
            if (!(formatProps.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)) {
               decaf_abort(fmt::format("Surface format {:03x}[{}]({}) does not support DepthStencilAttachement feature",
                                       format, tileType, vk::to_string(hostFormat)));
            }
         }
      }
   };

   for (auto format : supportedColorFormats) {
      checkFormat(format, latte::SQ_TILE_TYPE::DEFAULT);
   }

   for (auto format : supportedDepthFormats) {
      checkFormat(format, latte::SQ_TILE_TYPE::DEPTH);
   }
}


} // namespace vulkan

#endif // ifdef DECAF_VULKAN
