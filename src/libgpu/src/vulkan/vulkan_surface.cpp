#ifdef DECAF_VULKAN
#include "vulkan_driver.h"
#include "vulkan_utils.h"

#include "gpu_tiling.h"
#include "latte/latte_formats.h"

namespace vulkan
{

SurfaceDataObject *
Driver::allocateSurfaceData(const SurfaceDataDesc& info)
{
   decaf_check(info.baseAddress);
   decaf_check(info.width);
   decaf_check(info.height);
   decaf_check(info.depth);
   decaf_check(info.width <= 8192);
   decaf_check(info.height <= 8192);

   auto hostFormat = getVkSurfaceFormat(info.format, info.tileType);
   auto formatUsage = getVkSurfaceFormatUsage(info.format);

   vk::ImageAspectFlags aspectFlags;
   if (info.tileType != latte::SQ_TILE_TYPE::DEPTH) {
      if (formatUsage & (SurfaceFormatUsage::TEXTURE | SurfaceFormatUsage::COLOR)) {
         aspectFlags |= vk::ImageAspectFlagBits::eColor;
      }
   } else {
      if (formatUsage & SurfaceFormatUsage::DEPTH) {
         aspectFlags |= vk::ImageAspectFlagBits::eDepth;
      }
      if (formatUsage & SurfaceFormatUsage::STENCIL) {
         aspectFlags |= vk::ImageAspectFlagBits::eStencil;
      }
   }

   vk::ImageUsageFlags usageFlags;
   usageFlags |= vk::ImageUsageFlagBits::eTransferDst;
   usageFlags |= vk::ImageUsageFlagBits::eTransferSrc;
   if (formatUsage & SurfaceFormatUsage::TEXTURE) {
      usageFlags |= vk::ImageUsageFlagBits::eSampled;
   }
   if (info.tileType != latte::SQ_TILE_TYPE::DEPTH) {
      if (formatUsage & SurfaceFormatUsage::COLOR) {
         usageFlags |= vk::ImageUsageFlagBits::eColorAttachment;
      }
   } else {
      if (formatUsage & (SurfaceFormatUsage::DEPTH | SurfaceFormatUsage::STENCIL)) {
         usageFlags |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
      }
   }

   vk::ImageType imageType;
   auto realPitch = 1;
   auto realWidth = 1;
   auto realHeight = 1;
   auto realDepth = 1;
   auto realArrayLayers = 1;

   switch (info.dim) {
   case latte::SQ_TEX_DIM::DIM_1D:
      imageType = vk::ImageType::e1D;
      realPitch = info.pitch;
      realWidth = info.width;
      realHeight = 1;
      realDepth = 1;
      realArrayLayers = 1;
      break;
   case latte::SQ_TEX_DIM::DIM_2D:
      imageType = vk::ImageType::e2D;
      realPitch = info.pitch;
      realWidth = info.width;
      realHeight = info.height;
      realDepth = 1;
      realArrayLayers = 1;
      break;
   case latte::SQ_TEX_DIM::DIM_3D:
      imageType = vk::ImageType::e3D;
      realPitch = info.pitch;
      realWidth = info.width;
      realHeight = info.height;
      realDepth = info.depth;
      realArrayLayers = 1;
      break;
   case latte::SQ_TEX_DIM::DIM_CUBEMAP:
      imageType = vk::ImageType::e2D;
      realPitch = info.pitch;
      realWidth = info.width;
      realHeight = info.height;
      realDepth = 1;
      realArrayLayers = info.depth * 6;
      break;
   case latte::SQ_TEX_DIM::DIM_1D_ARRAY:
      imageType = vk::ImageType::e1D;
      realPitch = info.pitch;
      realWidth = info.width;
      realHeight = 1;
      realDepth = 1;
      realArrayLayers = info.height;
      break;
   case latte::SQ_TEX_DIM::DIM_2D_ARRAY:
      imageType = vk::ImageType::e2D;
      realPitch = info.pitch;
      realWidth = info.width;
      realHeight = info.height;
      realDepth = 1;
      realArrayLayers = info.depth;
      break;
   case latte::SQ_TEX_DIM::DIM_2D_MSAA:
   case latte::SQ_TEX_DIM::DIM_2D_ARRAY_MSAA:
      decaf_abort("We do not currently support multi-sampling");
   default:
      decaf_abort(fmt::format("Failed to pick vulkan dim for latte dim {}", info.dim));
   }

   auto realAddr = info.calcAlignedBaseAddress();
   auto swizzle = info.calcSwizzle();
   auto dataFormat = getSurfaceFormatDataFormat(info.format);
   auto bpp = latte::getDataFormatBitsPerElement(dataFormat);
   auto isDepthBuffer = (info.tileType == latte::SQ_TILE_TYPE::DEPTH);

   auto texelPitch = info.pitch;
   auto texelWidth = info.width;
   auto texelHeight = info.height;
   auto texelDepth = info.depth;

   if (dataFormat >= latte::SQ_DATA_FORMAT::FMT_BC1 && dataFormat <= latte::SQ_DATA_FORMAT::FMT_BC5) {
      // Block compressed textures are tiled/untiled in terms of blocks
      texelPitch = texelPitch / 4;
      texelWidth = (texelWidth + 3) / 4;
      texelHeight = (texelHeight + 3) / 4;

      // We need to make sure to round the sizes up appropriately
      realWidth = align_up(realWidth, 4);
      realHeight = align_up(realHeight, 4);
   }

   auto imageSize = texelPitch * texelHeight * texelDepth * bpp / 8;

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
   subresRange.aspectMask = aspectFlags;
   subresRange.baseMipLevel = 0;
   subresRange.levelCount = 1;
   subresRange.baseArrayLayer = 0;
   subresRange.layerCount = realArrayLayers;

   // Generate a mutator and grab the memory
   MemCacheMutator mutator;
   mutator.mode = MemCacheMutator::Mode::Retile;
   mutator.retile.tileMode = info.tileMode;
   mutator.retile.swizzle = swizzle;
   mutator.retile.pitch = texelPitch;
   mutator.retile.height = texelHeight;
   mutator.retile.depth = texelDepth;
   mutator.retile.aa = 0;
   mutator.retile.isDepth = isDepthBuffer;
   mutator.retile.bpp = bpp;
   auto cacheMem = getMemCache(phys_addr(realAddr), imageSize, mutator);

   // Record the external usage of this cache so its not evicted.
   cacheMem->extnRefCount++;

   // Return our freshly minted surface data object
   auto surfaceData = new SurfaceDataObject();
   surfaceData->desc = info;
   surfaceData->image = image;
   surfaceData->imageMem = imageMem;
   surfaceData->cacheMem = cacheMem;
   surfaceData->subresRange = subresRange;
   surfaceData->pitch = realPitch;
   surfaceData->width = realWidth;
   surfaceData->height = realHeight;
   surfaceData->depth = realDepth;
   surfaceData->arrayLayers = realArrayLayers;
   surfaceData->activeLayout = vk::ImageLayout::eUndefined;
   return surfaceData;
}

void
Driver::releaseSurfaceData(SurfaceDataObject *surfaceData)
{
   // TODO: Add support for releasing surface data...
}

void
Driver::uploadSurfaceData(SurfaceDataObject *surfaceData)
{
   if (surfaceData->desc.tileType == latte::SQ_TILE_TYPE::DEPTH) {
      // TODO: Implement uploading of depth buffer data (you know... just in case?)
      return;
   }

   if (surfaceData->desc.format == latte::SurfaceFormat::R11G11B10Float) {
      // TODO: Handle format conversion of R11G11B10 formats...
      return;
   }

   refreshMemCache(surfaceData->cacheMem);
   if (surfaceData->hash == surfaceData->cacheMem->dataHash) {
      // The underlying data has not changed, no need to do anything.
      surfaceData->lastUsageIndex = mActivePm4BufferIndex;
      return;
   }

   vk::BufferImageCopy region = {};
   region.bufferOffset = 0;
   region.bufferRowLength = surfaceData->pitch;
   region.bufferImageHeight = 0;

   region.imageSubresource.aspectMask = surfaceData->subresRange.aspectMask;
   region.imageSubresource.mipLevel = 0;
   region.imageSubresource.baseArrayLayer = 0;
   region.imageSubresource.layerCount = surfaceData->arrayLayers;

   region.imageOffset = { 0, 0, 0 };
   region.imageExtent = { static_cast<uint32_t>(surfaceData->width),
                           static_cast<uint32_t>(surfaceData->height),
                           static_cast<uint32_t>(surfaceData->depth) };

   transitionSurfaceData(surfaceData, vk::ImageLayout::eTransferDstOptimal);

   mActiveCommandBuffer.copyBufferToImage(
      surfaceData->cacheMem->buffer,
      surfaceData->image,
      vk::ImageLayout::eTransferDstOptimal,
      { region });

   surfaceData->hash = surfaceData->cacheMem->dataHash;
   surfaceData->lastUsageIndex = mActivePm4BufferIndex;
}

void
Driver::downloadSurfaceData(SurfaceDataObject *surfaceData)
{
   decaf_abort("We do not support surface downloads currently");
   // TODO: Implement surface downloads...
}

void
Driver::transitionSurfaceData(SurfaceDataObject *surfaceData, vk::ImageLayout newLayout)
{
   if (surfaceData->activeLayout == newLayout) {
      // Nothing to do, we are already the correct layout
      return;
   }

   vk::ImageMemoryBarrier imageBarrier;
   imageBarrier.srcAccessMask = vk::AccessFlags();
   imageBarrier.dstAccessMask = vk::AccessFlags();
   imageBarrier.oldLayout = surfaceData->activeLayout;
   imageBarrier.newLayout = newLayout;
   imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   imageBarrier.image = surfaceData->image;
   imageBarrier.subresourceRange = surfaceData->subresRange;

   mActiveCommandBuffer.pipelineBarrier(
      vk::PipelineStageFlagBits::eAllGraphics,
      vk::PipelineStageFlagBits::eAllGraphics,
      vk::DependencyFlags(),
      {},
      {},
      { imageBarrier });

   surfaceData->activeLayout = newLayout;
}

SurfaceDataObject *
Driver::getSurfaceData(const SurfaceDataDesc& info)
{
   auto &surfaceData = mSurfaceDatas[info.hash()];
   if (!surfaceData) {
      surfaceData = allocateSurfaceData(info);

      // This needs to be a second line as the above one will do the
      // assignment into the map reference...
      return surfaceData;
   }

   return surfaceData;
}

SurfaceObject *
Driver::allocateSurface(const SurfaceDesc& info)
{
   auto surfaceData = getSurfaceData(info.dataDesc);

   auto hostFormat = getVkSurfaceFormat(info.dataDesc.format, info.dataDesc.tileType);

   vk::ImageViewType imageViewType;
   switch (info.dataDesc.dim) {
   case latte::SQ_TEX_DIM::DIM_1D:
      imageViewType = vk::ImageViewType::e1D;
      break;
   case latte::SQ_TEX_DIM::DIM_2D:
      imageViewType = vk::ImageViewType::e2D;
      break;
   case latte::SQ_TEX_DIM::DIM_3D:
      imageViewType = vk::ImageViewType::e3D;
      break;
   case latte::SQ_TEX_DIM::DIM_CUBEMAP:
      imageViewType = vk::ImageViewType::e2DArray;
      break;
   case latte::SQ_TEX_DIM::DIM_1D_ARRAY:
      imageViewType = vk::ImageViewType::e1DArray;
      break;
   case latte::SQ_TEX_DIM::DIM_2D_ARRAY:
      imageViewType = vk::ImageViewType::e2DArray;
      break;
   case latte::SQ_TEX_DIM::DIM_2D_MSAA:
   case latte::SQ_TEX_DIM::DIM_2D_ARRAY_MSAA:
      decaf_abort("We do not currently support multi-sampling");
   default:
      decaf_abort(fmt::format("Failed to pick vulkan image view type for dim {}", info.dataDesc.dim));
   }

   auto hostComponentMap = vk::ComponentMapping();
   hostComponentMap.r = getVkComponentSwizzle(info.channels[0]);
   hostComponentMap.g = getVkComponentSwizzle(info.channels[1]);
   hostComponentMap.b = getVkComponentSwizzle(info.channels[2]);
   hostComponentMap.a = getVkComponentSwizzle(info.channels[3]);

   vk::ImageViewCreateInfo imageViewDesc;
   imageViewDesc.image = surfaceData->image;
   imageViewDesc.viewType = imageViewType;
   imageViewDesc.format = hostFormat;
   imageViewDesc.components = hostComponentMap;
   imageViewDesc.subresourceRange = surfaceData->subresRange;
   auto imageView = mDevice.createImageView(imageViewDesc);

   auto surface = new SurfaceObject();
   surface->desc = info;
   surface->data = surfaceData;
   surface->imageView = imageView;
   return surface;
}

void
Driver::releaseSurface(SurfaceObject *surface)
{
   // TODO: Add support for releasing surfaces...
}

SurfaceObject *
Driver::getSurface(const SurfaceDesc& info, bool discardData)
{
   auto &surface = mSurfaces[info.hash()];
   if (!surface) {
      surface = allocateSurface(info);

      if (!discardData) {
         uploadSurfaceData(surface->data);
      }

      return surface;
   }

   if (!discardData) {
      uploadSurfaceData(surface->data);
   }

   return surface;
}

void
Driver::transitionSurface(SurfaceObject *surface, vk::ImageLayout newLayout)
{
   transitionSurfaceData(surface->data, newLayout);
}


} // namespace vulkan

#endif // ifdef DECAF_VULKAN
