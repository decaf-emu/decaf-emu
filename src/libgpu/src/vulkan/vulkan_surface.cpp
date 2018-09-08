#ifdef DECAF_VULKAN
#include "vulkan_driver.h"
#include "vulkan_utils.h"
#include "latte/latte_formats.h"

namespace vulkan
{

SurfaceObject *
Driver::allocateSurface(const SurfaceInfo& info)
{
   auto surfaceInfo = getSurfaceTypeInfo(info.dim, info.height, info.depth);
   auto formatFlags = getDataFormatUsageFlags(info.format);

   auto hostFormat = getSurfaceFormat(info.format, info.numFormat, info.formatComp, info.degamma);

   vk::ImageCreateInfo createImageDesc;
   createImageDesc.imageType = surfaceInfo.first;
   createImageDesc.format = hostFormat;
   createImageDesc.extent = vk::Extent3D(info.width, info.height, info.depth);
   createImageDesc.mipLevels = 1;
   createImageDesc.arrayLayers = surfaceInfo.second;
   createImageDesc.samples = vk::SampleCountFlagBits::e1; // TODO: use `samples`
   createImageDesc.tiling = vk::ImageTiling::eOptimal;
   createImageDesc.usage =
      vk::ImageUsageFlagBits::eSampled |
      vk::ImageUsageFlagBits::eColorAttachment |
      vk::ImageUsageFlagBits::eTransferDst |
      vk::ImageUsageFlagBits::eTransferSrc;
   createImageDesc.sharingMode = vk::SharingMode::eExclusive;
   createImageDesc.initialLayout = vk::ImageLayout::eUndefined;
   auto image = mDevice.createImage(createImageDesc);

   auto imageMemReqs = mDevice.getImageMemoryRequirements(image);

   vk::MemoryAllocateInfo allocDesc;
   allocDesc.allocationSize = imageMemReqs.size;
   allocDesc.memoryTypeIndex = findMemoryType(imageMemReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
   auto imageMem = mDevice.allocateMemory(allocDesc);

   mDevice.bindImageMemory(image, imageMem, 0);

   vk::ImageSubresourceRange subresRange;
   if (formatFlags & DataFormatUsage::FORMAT_HAS_COLOR) {
      subresRange.aspectMask |= vk::ImageAspectFlagBits::eColor;
   }
   if (formatFlags & DataFormatUsage::FORMAT_HAS_DEPTH) {
      subresRange.aspectMask |= vk::ImageAspectFlagBits::eDepth;
   }
   if (formatFlags & DataFormatUsage::FORMAT_HAS_STENCIL) {
      subresRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
   }
   subresRange.baseMipLevel = 0;
   subresRange.levelCount = 1;
   subresRange.baseArrayLayer = 0;
   subresRange.layerCount = 1;

   // TODO: This is clearly the wrong way to handle this...
   vk::ImageViewType viewType;
   switch (surfaceInfo.first) {
   case vk::ImageType::e1D:
      decaf_check(surfaceInfo.second <= 1);
      viewType = vk::ImageViewType::e1D;
      break;
   case vk::ImageType::e2D:
      decaf_check(surfaceInfo.second <= 1);
      viewType = vk::ImageViewType::e2D;
      break;
   case vk::ImageType::e3D:
      decaf_check(surfaceInfo.second <= 1);
      viewType = vk::ImageViewType::e3D;
      break;
   default:
      decaf_abort("Unsupported texture view type.");
   }

   // TODO: I think we can use component mapping to fixup the out
   // of order formats that exist in the formats table.
   auto hostComponentMap = vk::ComponentMapping();

   vk::ImageViewCreateInfo imageViewDesc;
   imageViewDesc.image = image;
   imageViewDesc.viewType = viewType;
   imageViewDesc.format = hostFormat;
   imageViewDesc.components = hostComponentMap;
   imageViewDesc.subresourceRange = subresRange;
   auto imageView = mDevice.createImageView(imageViewDesc);

   auto surface = new SurfaceObject();
   surface->info = info;
   surface->image = image;
   surface->imageView = imageView;
   surface->subresRange = subresRange;
   surface->activeLayout = vk::ImageLayout::eUndefined;
   return surface;
}

void
Driver::releaseSurface(SurfaceObject *surface)
{
   // TODO: Add support for releasing a surface
}

void
Driver::uploadSurface(SurfaceObject *surface)
{
   // TODO: Implement surface uploads...
}

void
Driver::downloadSurface(SurfaceObject *surface)
{
   // TODO: Implement surface downloads...
}

void
Driver::transitionSurface(SurfaceObject *surface, vk::ImageLayout newLayout)
{
   if (surface->activeLayout == newLayout) {
      // Nothing to do, we are already the correct layout
      return;
   }

   vk::ImageMemoryBarrier imageBarrier;
   imageBarrier.srcAccessMask = vk::AccessFlags();
   imageBarrier.dstAccessMask = vk::AccessFlags();
   imageBarrier.oldLayout = surface->activeLayout;
   imageBarrier.newLayout = newLayout;
   imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   imageBarrier.image = surface->image;
   imageBarrier.subresourceRange = surface->subresRange;

   mActiveCommandBuffer.pipelineBarrier(
      vk::PipelineStageFlagBits::eAllGraphics,
      vk::PipelineStageFlagBits::eAllGraphics,
      vk::DependencyFlags(),
      {},
      {},
      { imageBarrier });

   surface->activeLayout = newLayout;
}

SurfaceObject *
Driver::getSurface(const SurfaceInfo& infoIn, bool discardData)
{
   // A hack for now...
   SurfaceInfo info = infoIn;

   decaf_check(info.baseAddress);
   decaf_check(info.width);
   decaf_check(info.height);
   decaf_check(info.depth);
   decaf_check(info.width <= 8192);
   decaf_check(info.height <= 8192);

   // Grab the swizzle from this...
   auto swizzle = static_cast<uint32_t>(info.baseAddress) & 0xFFF;

   // Align the base address according to the GPU logic
   if (info.tileMode >= latte::SQ_TILE_MODE::TILED_2D_THIN1) {
      info.baseAddress &= ~(0x800 - 1);
   } else {
      info.baseAddress &= ~(0x100 - 1);
   }

   auto surfaceKey = static_cast<uint64_t>(info.baseAddress) << 32;
   surfaceKey ^= info.format << 22 ^ info.numFormat << 28 ^ info.formatComp << 30;

   auto dimDimensions = latte::getTexDimDimensions(info.dim);
   if (dimDimensions == 1) {
      surfaceKey ^= info.pitch;
   } else if (dimDimensions == 2) {
      surfaceKey ^= info.pitch ^ (info.height << 10);
   } else if (dimDimensions == 3) {
      surfaceKey ^= info.pitch ^ (info.height << 10) ^ (info.depth << 20);
   } else {
      decaf_abort("Unexpected dim dimensions.");
   }

   auto &surface = mSurfaces[surfaceKey];
   if (!surface) {
      surface = allocateSurface(info);
      mSurfaces[surfaceKey] = surface;

      if (!discardData) {
         uploadSurface(surface);
      }
      
      return surface;
   }

   // TODO: Add a hashing check here...
   if (!discardData) {
      uploadSurface(surface);
   }

   return surface;
}

} // namespace vulkan

#endif // DECAF_VULKAN
