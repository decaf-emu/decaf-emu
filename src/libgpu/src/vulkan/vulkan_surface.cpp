#ifdef DECAF_VULKAN
#include "vulkan_driver.h"
#include "vulkan_utils.h"

#include "gpu_tiling.h"
#include "latte/latte_formats.h"

namespace vulkan
{

MemCacheObject *
Driver::getSurfaceMemCache(const SurfaceDataDesc &info)
{
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
   }

   auto imageSize = texelPitch * texelHeight * texelDepth * bpp / 8;

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
   return getMemCache(phys_addr(realAddr), imageSize, mutator);
}

SurfaceSubDataObject *
Driver::allocateSurfaceSubData(const SurfaceDataDesc& info)
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

   // Return our freshly minted surface data object
   auto surfaceSubData = new SurfaceSubDataObject();
   surfaceSubData->desc = info;
   surfaceSubData->image = image;
   surfaceSubData->imageMem = imageMem;
   surfaceSubData->pitch = realPitch;
   surfaceSubData->width = realWidth;
   surfaceSubData->height = realHeight;
   surfaceSubData->depth = realDepth;
   surfaceSubData->arrayLayers = realArrayLayers;
   surfaceSubData->subresRange = subresRange;
   surfaceSubData->activeLayout = vk::ImageLayout::eUndefined;
   return surfaceSubData;
}

void
Driver::releaseSurfaceSubData(SurfaceSubDataObject *surfaceSubData)
{
   // TODO: Add support for releasing surface data...
}

void
Driver::copySurfaceSubData(SurfaceSubDataObject *dst, SurfaceSubDataObject *src)
{
   // TODO: Add support for copying depth buffers here...
   decaf_check(dst->desc.tileType == src->desc.tileType);
   //decaf_check(dst->desc.tileType == latte::SQ_TILE_TYPE::DEFAULT);

   auto copyWidth = std::min(dst->width, src->width);
   auto copyHeight = std::min(dst->height, src->height);
   auto copyDepth = std::min(dst->depth, src->depth);
   auto copyLayers = std::min(dst->arrayLayers, src->arrayLayers);
   auto copyAspect = vk::ImageAspectFlags();

   auto formatUsage = getVkSurfaceFormatUsage(src->desc.format);
   if (src->desc.tileType != latte::SQ_TILE_TYPE::DEPTH) {
      if (formatUsage & (SurfaceFormatUsage::TEXTURE | SurfaceFormatUsage::COLOR)) {
         copyAspect |= vk::ImageAspectFlagBits::eColor;
      }
   } else {
      if (formatUsage & SurfaceFormatUsage::DEPTH) {
         copyAspect |= vk::ImageAspectFlagBits::eDepth;
      }
      if (formatUsage & SurfaceFormatUsage::STENCIL) {
         copyAspect |= vk::ImageAspectFlagBits::eStencil;
      }
   }

   vk::ImageBlit blitRegion(
      vk::ImageSubresourceLayers(copyAspect, 0, 0, copyLayers),
      { vk::Offset3D(0, 0, 0), vk::Offset3D(copyWidth, copyHeight, copyDepth) },
      vk::ImageSubresourceLayers(copyAspect, 0, 0, copyLayers),
      { vk::Offset3D(0, 0, 0), vk::Offset3D(copyWidth, copyHeight, copyDepth) });

   transitionSurfaceSubData(dst, vk::ImageLayout::eTransferDstOptimal);
   transitionSurfaceSubData(src, vk::ImageLayout::eTransferSrcOptimal);

   mActiveCommandBuffer.blitImage(
      src->image,
      vk::ImageLayout::eTransferSrcOptimal,
      dst->image,
      vk::ImageLayout::eTransferDstOptimal,
      { blitRegion },
      vk::Filter::eNearest);
}

void
Driver::transitionSurfaceSubData(SurfaceSubDataObject *surfaceSubData, vk::ImageLayout newLayout)
{
   if (surfaceSubData->activeLayout == newLayout) {
      // Nothing to do, we are already the correct layout
      return;
   }

   vk::ImageMemoryBarrier imageBarrier;
   imageBarrier.srcAccessMask = vk::AccessFlags();
   imageBarrier.dstAccessMask = vk::AccessFlags();
   imageBarrier.oldLayout = surfaceSubData->activeLayout;
   imageBarrier.newLayout = newLayout;
   imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   imageBarrier.image = surfaceSubData->image;
   imageBarrier.subresourceRange = surfaceSubData->subresRange;

   mActiveCommandBuffer.pipelineBarrier(
      vk::PipelineStageFlagBits::eAllGraphics,
      vk::PipelineStageFlagBits::eAllGraphics,
      vk::DependencyFlags(),
      {},
      {},
      { imageBarrier });

   surfaceSubData->activeLayout = newLayout;
}

SurfaceDataObject *
Driver::allocateSurfaceData(const SurfaceDataDesc& info)
{
   auto cacheMem = getSurfaceMemCache(info);
   auto surfaceSubData = allocateSurfaceSubData(info);

   // Lock the cached memory from being evicted.
   cacheMem->extnRefCount++;

   auto surfaceData = new SurfaceDataObject();
   surfaceData->desc = info;

   surfaceData->cacheMem = cacheMem;

   surfaceData->masterImage = surfaceSubData;
   surfaceData->activeImage = surfaceSubData;
   surfaceData->images[info.hash()] = surfaceSubData;

   surfaceData->lastUsageIndex = 0;
   surfaceData->hash = DataHash {};

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

   auto& activeImage = surfaceData->activeImage;
   auto& masterImage = surfaceData->masterImage;

   if (activeImage != masterImage) {
      // If the active image isn't the master surface, we need to
      // switch to it first to do the full upload...
      makeSurfaceSubDataActive(surfaceData, surfaceData->masterImage);

      // Lets make sure that worked.
      decaf_check(activeImage == masterImage);
   }

   vk::BufferImageCopy region = {};
   region.bufferOffset = 0;
   region.bufferRowLength = masterImage->pitch;
   region.bufferImageHeight = 0;

   region.imageSubresource.aspectMask = masterImage->subresRange.aspectMask;
   region.imageSubresource.mipLevel = 0;
   region.imageSubresource.baseArrayLayer = 0;
   region.imageSubresource.layerCount = masterImage->arrayLayers;

   region.imageOffset = { 0, 0, 0 };
   region.imageExtent = { static_cast<uint32_t>(masterImage->width),
                           static_cast<uint32_t>(masterImage->height),
                           static_cast<uint32_t>(masterImage->depth) };

   transitionSurfaceSubData(masterImage, vk::ImageLayout::eTransferDstOptimal);

   mActiveCommandBuffer.copyBufferToImage(
      surfaceData->cacheMem->buffer,
      masterImage->image,
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
Driver::makeSurfaceSubDataActive(SurfaceDataObject *surface, SurfaceSubDataObject *newImage)
{
   if (newImage == surface->activeImage) {
      // We are already the active surface, we don't need to do
      // any additional work at this point.
      return;
   }

   auto& masterImage = surface->masterImage;
   auto& activeImage = surface->activeImage;

   if (activeImage != masterImage) {
      // If the active surface is not the master surface, we first need to copy
      // back to the master as the master is the source of truth.
      copySurfaceSubData(masterImage, activeImage);

      // Mark the new active image for safety
      activeImage = masterImage;
   }

   if (newImage->width > masterImage->width ||
       newImage->height > masterImage->height ||
       newImage->depth > masterImage->depth ||
       newImage->arrayLayers > masterImage->arrayLayers) {
      // Quick check to make sure we aren't incorrectly sized in one
      // dimension somehow (this should never happen due to desc matching).
      auto newMasterDesc = masterImage->desc;
      newMasterDesc.pitch = std::max(newMasterDesc.pitch, newImage->desc.pitch);
      newMasterDesc.width = std::max(newMasterDesc.width, newImage->desc.width);
      newMasterDesc.height = std::max(newMasterDesc.height, newImage->desc.height);
      newMasterDesc.depth = std::max(newMasterDesc.depth, newImage->desc.depth);

      auto newMasterImage = newImage;
      if (newMasterDesc.hash() != newImage->desc.hash()) {
         // If the new master needs to be even bigger than the new image, lets
         // set up our new master image first.
         newMasterImage = allocateSurfaceSubData(newMasterDesc);
         surface->images[newMasterDesc.hash()] = newMasterImage;
      }

      auto oldMasterImage = surface->masterImage;
      auto oldCacheMem = surface->cacheMem;

      // Allocate new cached memory encompasing the new master
      auto cacheMem = getSurfaceMemCache(newMasterDesc);
      cacheMem->extnRefCount++;
      surface->cacheMem = cacheMem;

      // Upload the CPU data to the new surface
      surface->masterImage = newMasterImage;
      surface->activeImage = newMasterImage;
      uploadSurfaceData(surface);

      // Copy the surface data from the old master into the new image
      copySurfaceSubData(masterImage, oldMasterImage);

      // Remove the reference to the old cached memory
      oldCacheMem->extnRefCount--;
   }

   if (activeImage != newImage) {
      // If we are not the active surface yet, we can do the last copy
      // to get us to that point.
      copySurfaceSubData(newImage, activeImage);
      activeImage = newImage;
   }

   // Make sure everything went according to plan
   decaf_check(activeImage == newImage);
}

SurfaceSubDataObject *
Driver::getSurfaceSubData(SurfaceDataObject *surface, const SurfaceDataDesc& info)
{
   auto infoHash = info.hash();

   if (surface->activeImage->desc.hash() == infoHash) {
      // The current surface iamge is already the active one, we don't need
      // to do anything special to handle this case.
      return surface->activeImage;
   }

   auto& foundImage = surface->images[infoHash];
   if (!foundImage) {
      // Allocate a new surface image and implicitly place it into the
      // map (as this is a reference).  Making this subdata active will
      // elect this new surface to being master if needed.
      foundImage = allocateSurfaceSubData(info);
   } else {
      // This case should be caught above, but for safety sake we
      // are going to double-check since copies will fail otherwise.
      decaf_check(foundImage != surface->activeImage);
   }

   // We implicit select the surface if it was fetched
   makeSurfaceSubDataActive(surface, foundImage);

   return foundImage;
}

SurfaceDataObject *
Driver::getSurfaceData(const SurfaceDataDesc& info)
{
   auto &surfaceData = mSurfaceDatas[info.hash(true)];
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
   auto surfaceSubData = getSurfaceSubData(surfaceData, info.dataDesc);

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
   imageViewDesc.image = surfaceSubData->image;
   imageViewDesc.viewType = imageViewType;
   imageViewDesc.format = hostFormat;
   imageViewDesc.components = hostComponentMap;
   imageViewDesc.subresourceRange = surfaceSubData->subresRange;
   auto imageView = mDevice.createImageView(imageViewDesc);

   auto surface = new SurfaceObject();
   surface->desc = info;
   surface->masterData = surfaceData;
   surface->data = surfaceSubData;
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
   }

   if (!discardData) {
      uploadSurfaceData(surface->masterData);
   }

   makeSurfaceSubDataActive(surface->masterData, surface->data);

   return surface;
}

void
Driver::transitionSurface(SurfaceObject *surface, vk::ImageLayout newLayout)
{
   transitionSurfaceSubData(surface->data, newLayout);
}


} // namespace vulkan

#endif // ifdef DECAF_VULKAN
