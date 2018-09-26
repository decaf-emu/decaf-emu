#pragma optimize("", off)

#ifdef DECAF_VULKAN

#include "vulkan_driver.h"
#include "vulkan_utils.h"

#include "gpu_tiling.h"
#include "latte/latte_formats.h"

namespace vulkan
{

SurfaceObject *
Driver::allocateSurface(const SurfaceDesc& info)
{
   auto formatFlags = getDataFormatUsageFlags(info.format);
   auto hostFormat = getSurfaceFormat(info.format, info.numFormat, info.formatComp, info.degamma, info.isDepthBuffer);

   vk::ImageUsageFlags usageFlags;
   usageFlags |= vk::ImageUsageFlagBits::eTransferDst;
   usageFlags |= vk::ImageUsageFlagBits::eTransferSrc;
   usageFlags |= vk::ImageUsageFlagBits::eSampled;

   if (formatFlags & DataFormatUsage::FORMAT_ALLOW_RENDER_TARGET) {
      if (!info.isDepthBuffer) {
         if (formatFlags & DataFormatUsage::FORMAT_MAYBE_COLOR) {
            usageFlags |= vk::ImageUsageFlagBits::eColorAttachment;
         }
      }
      if (info.isDepthBuffer) {
         if (formatFlags & (DataFormatUsage::FORMAT_MAYBE_DEPTH | DataFormatUsage::FORMAT_MAYBE_STENCIL)) {
            usageFlags |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
         }
      }
   }

   vk::ImageType imageType;
   vk::ImageViewType imageViewType;
   auto realWidth = 1;
   auto realHeight = 1;
   auto realDepth = 1;
   auto realArrayLayers = 1;
   
   switch (info.dim) {
   case latte::SQ_TEX_DIM::DIM_1D:
      imageType = vk::ImageType::e1D;
      realWidth = info.width;
      realHeight = 1;
      realDepth = 1;
      realArrayLayers = 1;
      imageViewType = vk::ImageViewType::e1D;
      break;
   case latte::SQ_TEX_DIM::DIM_2D:
      imageType = vk::ImageType::e2D;
      realWidth = info.width;
      realHeight = info.height;
      realDepth = 1;
      realArrayLayers = 1;
      imageViewType = vk::ImageViewType::e2D;
      break;
   case latte::SQ_TEX_DIM::DIM_3D:
      imageType = vk::ImageType::e3D;
      realWidth = info.width;
      realHeight = info.height;
      realDepth = info.depth;
      realArrayLayers = 1;
      imageViewType = vk::ImageViewType::e3D;
      break;
   case latte::SQ_TEX_DIM::DIM_CUBEMAP:
      imageType = vk::ImageType::e2D;
      realWidth = info.width;
      realHeight = info.height;
      realDepth = 1;
      realArrayLayers = info.depth;
      imageViewType = vk::ImageViewType::eCube;
      break;
   case latte::SQ_TEX_DIM::DIM_1D_ARRAY:
      imageType = vk::ImageType::e1D;
      realWidth = info.width;
      realHeight = 1;
      realDepth = 1;
      realArrayLayers = info.height;
      imageViewType = vk::ImageViewType::e1DArray;
      break;
   case latte::SQ_TEX_DIM::DIM_2D_ARRAY:
      imageType = vk::ImageType::e2D;
      realWidth = info.width;
      realHeight = info.height;
      realDepth = 1;
      realArrayLayers = info.depth;
      imageViewType = vk::ImageViewType::e2DArray;
      break;
   case latte::SQ_TEX_DIM::DIM_2D_MSAA:
   case latte::SQ_TEX_DIM::DIM_2D_ARRAY_MSAA:
      decaf_abort("We do not currently support multi-sampling");
   default:
      decaf_abort(fmt::format("Failed to pick vulkan dim for latte dim {}", info.dim));
   }

   vk::ImageCreateInfo createImageDesc;
   createImageDesc.imageType = imageType;
   createImageDesc.format = hostFormat;
   createImageDesc.extent = vk::Extent3D(realWidth, realHeight, realDepth);
   createImageDesc.mipLevels = 1;
   createImageDesc.arrayLayers = realArrayLayers;
   createImageDesc.samples = vk::SampleCountFlagBits::e1;
   createImageDesc.tiling = vk::ImageTiling::eOptimal;
   createImageDesc.usage = usageFlags;
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
   // TODO: We should probably improve how we build the list of
   // aspect mask values that a particular format has.
   if (!info.isDepthBuffer) {
      if (formatFlags & DataFormatUsage::FORMAT_MAYBE_COLOR) {
         subresRange.aspectMask |= vk::ImageAspectFlagBits::eColor;
      }
   }
   if (info.isDepthBuffer) {
      if (formatFlags & DataFormatUsage::FORMAT_MAYBE_DEPTH) {
         subresRange.aspectMask |= vk::ImageAspectFlagBits::eDepth;
      }
      if (formatFlags & DataFormatUsage::FORMAT_MAYBE_STENCIL) {
         subresRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
      }
   }
   subresRange.baseMipLevel = 0;
   subresRange.levelCount = 1;
   subresRange.baseArrayLayer = 0;
   subresRange.layerCount = realArrayLayers;

   // TODO: I think we can use component mapping to fixup the out of order
   // formats that exist in the formats table.  This requires cooperation
   // between the format picker and this component mapping though.
   // TODO: We should support reading the component mapping from the registers
   // as well, as some games remap registers to different places.
   auto hostComponentMap = vk::ComponentMapping();

   vk::ImageViewCreateInfo imageViewDesc;
   imageViewDesc.image = image;
   imageViewDesc.viewType = imageViewType;
   imageViewDesc.format = hostFormat;
   imageViewDesc.components = hostComponentMap;
   imageViewDesc.subresourceRange = subresRange;
   auto imageView = mDevice.createImageView(imageViewDesc);

   auto surface = new SurfaceObject();
   surface->desc = info;
   surface->image = image;
   surface->imageView = imageView;
   surface->subresRange = subresRange;
   surface->activeLayout = vk::ImageLayout::eUndefined;
   return surface;
}

void
Driver::releaseSurface(SurfaceObject *surface)
{
   // TODO: Add support for releasing a surface...
}

void
Driver::uploadSurface(SurfaceObject *surface)
{
   if (surface->desc.isDepthBuffer) {
      // TODO: Implement uploading of depth buffer data (you know... just in case?)
      return;
   }

   auto imagePtr = phys_cast<uint8_t*>(surface->desc.baseAddress).getRawPointer();
   auto dim = surface->desc.dim;
   auto format = surface->desc.format;
   auto bpp = latte::getDataFormatBitsPerElement(surface->desc.format);
   auto srcWidth = surface->desc.width;
   auto srcHeight = surface->desc.height;
   auto srcPitch = surface->desc.pitch;
   auto srcDepth = surface->desc.depth;
   auto tileMode = surface->desc.tileMode;
   auto swizzle = surface->desc.swizzle;
   auto isDepthBuffer = surface->desc.isDepthBuffer;

   auto texelWidth = srcWidth;
   auto texelHeight = srcHeight;
   auto texelPitch = srcPitch;
   auto texelDepth = srcDepth;
   if (format >= latte::SQ_DATA_FORMAT::FMT_BC1 && format <= latte::SQ_DATA_FORMAT::FMT_BC5) {
      texelWidth = (texelWidth + 3) / 4;
      texelHeight = (texelHeight + 3) / 4;
      texelPitch = texelPitch / 4;
   }

   if (dim == latte::SQ_TEX_DIM::DIM_CUBEMAP) {
      //uploadDepth *= 6;
   }

   auto srcImageSize = texelPitch * texelHeight * texelDepth * bpp / 8;
   auto dstImageSize = texelPitch * texelHeight * texelDepth * bpp / 8;

   auto newHash = DataHash {}.write(imagePtr, srcImageSize);
   if (surface->dataHash != newHash) {
      surface->dataHash = newHash;

      std::vector<uint8_t> untiledImage;
      untiledImage.resize(dstImageSize);

      // Untile
      gpu::convertFromTiled(
         untiledImage.data(),
         texelPitch,
         imagePtr,
         tileMode,
         swizzle,
         texelPitch,
         texelWidth,
         texelHeight,
         texelDepth,
         0,
         isDepthBuffer,
         bpp);

      auto uploadBuffer = getStagingBuffer(dstImageSize);
      auto uploadPtr = mapStagingBuffer(uploadBuffer, false);
      memcpy(uploadPtr, untiledImage.data(), dstImageSize);
      unmapStagingBuffer(uploadBuffer, true);

      vk::BufferImageCopy region = {};
      region.bufferOffset = 0;
      region.bufferRowLength = srcPitch;
      region.bufferImageHeight = srcHeight;

      region.imageSubresource.aspectMask = surface->subresRange.aspectMask;
      region.imageSubresource.mipLevel = surface->subresRange.baseMipLevel;
      region.imageSubresource.baseArrayLayer = surface->subresRange.baseArrayLayer;
      region.imageSubresource.layerCount = surface->subresRange.layerCount;

      region.imageOffset = { 0, 0, 0 };
      region.imageExtent = { srcWidth, srcHeight, srcDepth };

      transitionSurface(surface, vk::ImageLayout::eTransferDstOptimal);

      mActiveCommandBuffer.copyBufferToImage(
         uploadBuffer->buffer,
         surface->image,
         vk::ImageLayout::eTransferDstOptimal,
         { region });
   }
}

void
Driver::downloadSurface(SurfaceObject *surface)
{
   decaf_abort("We do not support surface downloads currently");
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
Driver::getSurface(const SurfaceDesc& infoIn, bool discardData)
{
   // We have to copy the surface info as we adjust the baseAddress
   // based on the alignment requirements of the tile mode.
   SurfaceDesc info = infoIn;

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

   if (!discardData) {
      uploadSurface(surface);
   }

   return surface;
}

} // namespace vulkan

#endif // DECAF_VULKAN
