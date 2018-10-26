#ifdef DECAF_VULKAN
#include "vulkan_driver.h"
#include "vulkan_utils.h"

#include "gpu_tiling.h"
#include "latte/latte_formats.h"

namespace vulkan
{

MemCacheObject *
Driver::_getSurfaceMemCache(const SurfaceDesc &info)
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

   if (info.dim == latte::SQ_TEX_DIM::DIM_CUBEMAP) {
      texelDepth *= 6;
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

void
Driver::_copySurface(SurfaceObject *dst, SurfaceObject *src)
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

   _barrierSurface(dst, ResourceUsage::TransferDst, vk::ImageLayout::eTransferDstOptimal);
   _barrierSurface(src, ResourceUsage::TransferSrc, vk::ImageLayout::eTransferSrcOptimal);

   mActiveCommandBuffer.blitImage(
      src->image,
      vk::ImageLayout::eTransferSrcOptimal,
      dst->image,
      vk::ImageLayout::eTransferDstOptimal,
      { blitRegion },
      vk::Filter::eNearest);
}

SurfaceGroupObject *
Driver::_allocateSurfaceGroup(const SurfaceDesc& info)
{
   // We don't actually allocate any memory stuff here.  This allows us
   // to centralize our memcache handling and what not in the one place.

   auto surface = new SurfaceGroupObject();
   surface->desc = info;

   surface->cacheMem = nullptr;
   surface->masterImage = nullptr;
   surface->activeImage = nullptr;

   surface->lastUsageIndex = 0;
   surface->hash = DataHash {};

   return surface;
}

void
Driver::_releaseSurfaceGroup(SurfaceGroupObject *surface)
{
   // TODO: Add support for releasing surface data...
}

void
Driver::_readSurfaceData(SurfaceGroupObject *surface)
{
   if (surface->desc.tileType == latte::SQ_TILE_TYPE::DEPTH) {
      // TODO: Implement uploading of depth buffer data (you know... just in case?)
      return;
   }

   if (surface->desc.format == latte::SurfaceFormat::R11G11B10Float) {
      // TODO: Handle format conversion of R11G11B10 formats...
      return;
   }

   // Attempting to read the surface data when the active isn't the master
   // would cause the data to be immediately overwritten when the active changes.
   decaf_check(surface->activeImage == surface->masterImage);

   auto& masterImage = surface->masterImage;

   vk::BufferImageCopy region = {};
   region.bufferOffset = 0;
   region.bufferRowLength = masterImage->pitch;
   region.bufferImageHeight = masterImage->height;

   region.imageSubresource.aspectMask = masterImage->subresRange.aspectMask;
   region.imageSubresource.mipLevel = 0;
   region.imageSubresource.baseArrayLayer = 0;
   region.imageSubresource.layerCount = masterImage->arrayLayers;

   region.imageOffset = { 0, 0, 0 };
   region.imageExtent = { static_cast<uint32_t>(masterImage->width),
                           static_cast<uint32_t>(masterImage->height),
                           static_cast<uint32_t>(masterImage->depth) };

   transitionMemCache(surface->cacheMem, ResourceUsage::TransferSrc);
   _barrierSurface(masterImage, ResourceUsage::TransferDst, vk::ImageLayout::eTransferDstOptimal);

   mActiveCommandBuffer.copyBufferToImage(
      surface->cacheMem->buffer,
      masterImage->image,
      vk::ImageLayout::eTransferDstOptimal,
      { region });
}

void
Driver::_writeSurfaceData(SurfaceGroupObject *surface)
{
   if (surface->desc.tileType == latte::SQ_TILE_MODE::LINEAR_ALIGNED) {
      // If this is a linear aligned texture, we will assume that it is a swap
      // buffer for now and don't invalidate it to the CPU.
      return;
   }

   if (surface->desc.tileType == latte::SQ_TILE_TYPE::DEPTH) {
      // TODO: Implement uploading of depth buffer data (you know... just in case?)
      return;
   }

   if (surface->desc.format == latte::SurfaceFormat::R11G11B10Float) {
      // TODO: Handle format conversion of R11G11B10 formats...
      return;
   }

   auto& masterImage = surface->masterImage;

   // We need to switch back to the master image before we can perform the write...
   auto& activeImage = surface->activeImage;
   if (activeImage != masterImage) {
      _copySurface(masterImage, activeImage);
      activeImage = masterImage;
   }

   // Perform the actual copy
   vk::BufferImageCopy region = {};
   region.bufferOffset = 0;
   region.bufferRowLength = masterImage->pitch;
   region.bufferImageHeight = masterImage->height;

   region.imageSubresource.aspectMask = masterImage->subresRange.aspectMask;
   region.imageSubresource.mipLevel = 0;
   region.imageSubresource.baseArrayLayer = 0;
   region.imageSubresource.layerCount = masterImage->arrayLayers;

   region.imageOffset = { 0, 0, 0 };
   region.imageExtent = { static_cast<uint32_t>(masterImage->width),
                           static_cast<uint32_t>(masterImage->height),
                           static_cast<uint32_t>(masterImage->depth) };

   _barrierSurface(masterImage, ResourceUsage::TransferSrc, vk::ImageLayout::eTransferSrcOptimal);

   mActiveCommandBuffer.copyImageToBuffer(
      masterImage->image,
      vk::ImageLayout::eTransferSrcOptimal,
      surface->cacheMem->buffer,
      { region });
}

void
Driver::_activateGroupSurface(SurfaceGroupObject *surfaceGroup, SurfaceObject *newSurface, ResourceUsage usage)
{
   bool forWrite;
   switch (usage) {
   case ResourceUsage::ColorAttachment:
   case ResourceUsage::DepthStencilAttachment:
   case ResourceUsage::StreamOutBuffer:
   case ResourceUsage::TransferDst:
      forWrite = true;
      break;
   case ResourceUsage::Texture:
   case ResourceUsage::TransferSrc:
      forWrite = false;
      break;
   default:
      decaf_abort("Unexpected surface resource usage");
   }

   if (newSurface != surfaceGroup->activeImage) {
      auto& masterImage = surfaceGroup->masterImage;
      auto& activeImage = surfaceGroup->activeImage;

      // If the active surface is not the master surface, we first need to copy
      // back to the master as the master is the source of truth.
      if (masterImage && activeImage != masterImage) {
         // Perform the actual copy first
         _copySurface(masterImage, activeImage);

         // Mark the new active image for safety
         activeImage = masterImage;
      }

      // If there is not yet a master image, or the new image is bigger
      // than the current master we need to promote a new master.
      if (!masterImage ||
          newSurface->width > masterImage->width ||
          newSurface->height > masterImage->height ||
          newSurface->depth > masterImage->depth ||
          newSurface->arrayLayers > masterImage->arrayLayers) {
         auto newMasterDesc = newSurface->desc;
         if (masterImage) {
            newMasterDesc.pitch = std::max(newMasterDesc.pitch, masterImage->desc.pitch);
            newMasterDesc.width = std::max(newMasterDesc.width, masterImage->desc.width);
            newMasterDesc.height = std::max(newMasterDesc.height, masterImage->desc.height);
            newMasterDesc.depth = std::max(newMasterDesc.depth, masterImage->desc.depth);
         }

         auto newMasterImage = newSurface;
         if (newMasterDesc.hash() != newSurface->desc.hash()) {
            // If the new master needs to be even bigger than the new image, lets
            // set up our new master image first.
            newMasterImage = _allocateSurface(newMasterDesc);
         }

         auto oldMasterImage = surfaceGroup->masterImage;
         auto oldCacheMem = surfaceGroup->cacheMem;

         // Allocate new cached memory encompasing the new master
         auto cacheMem = _getSurfaceMemCache(newMasterDesc);
         cacheMem->refCount++;
         surfaceGroup->cacheMem = cacheMem;

         // Upload the CPU data to the new surface
         surfaceGroup->masterImage = newMasterImage;
         surfaceGroup->activeImage = newMasterImage;
         _readSurfaceData(surfaceGroup);

         // Copy the surface data from the old master into the new image
         if (oldMasterImage) {
            _copySurface(masterImage, oldMasterImage);
         }

         // Remove the reference to the old cached memory
         if (oldCacheMem) {
            oldCacheMem->refCount--;
         }
      }

      // If we are not the active surface yet, we can do the last copy
      // to get us to that point.
      if (activeImage != newSurface) {
         _copySurface(newSurface, activeImage);
         activeImage = newSurface;
      }

      // Make sure everything went according to plan
      decaf_check(activeImage == newSurface);
   }

   // If this usage is a write usage, we need to do some work to mark the
   // underlying memory as having been modified by this surface.  If this
   // is for a read, we need to verify that the memory has not changed.
   if (!forWrite) {
      auto& memCache = surfaceGroup->cacheMem;

      // TODO: This can cause a previous write to this surface to be invalidated
      // back to the cache and then immediately read in, as the memory cache does
      // not know that the last writer was us!

      // Transition to being a reader of the memory cache
      transitionMemCache(memCache, ResourceUsage::TransferSrc);

      // Mark us as having been used this frame.  Note that we cannot predicate
      // checking the hashes on this as there could have been writes by other
      // stuff inside this particular pm4 context.
      surfaceGroup->lastUsageIndex = mActivePm4BufferIndex;

      // If the underlying hash has changed, we need to refresh ourselves.
      if (surfaceGroup->hash != memCache->dataHash) {
         _readSurfaceData(surfaceGroup);
      }

      // Mark ourselves as having the latest data
      surfaceGroup->hash = memCache->dataHash;
   } else {
      // Mark this surface as needing to be read-back at the next transition
      // or at the end of the active Pm4Context.
      markMemCacheDirty(surfaceGroup->cacheMem, [=](bool cancelled){
         if (!cancelled) {
            _writeSurfaceData(surfaceGroup);
         }
      });
      surfaceGroup->hash = surfaceGroup->cacheMem->dataHash;
   }
}

SurfaceGroupObject *
Driver::_getSurfaceGroup(const SurfaceDesc& info)
{
   auto &surface = mSurfaceGroups[info.hash(true)];
   if (!surface) {
      surface = _allocateSurfaceGroup(info);
   }

   return surface;
}

SurfaceObject *
Driver::_allocateSurface(const SurfaceDesc& info)
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

   setVkObjectName(image, fmt::format("surfimg_{:08x}", info.baseAddress).c_str());

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
   auto surfaceData = new SurfaceObject();
   surfaceData->desc = info;
   surfaceData->image = image;
   surfaceData->imageMem = imageMem;
   surfaceData->pitch = realPitch;
   surfaceData->width = realWidth;
   surfaceData->height = realHeight;
   surfaceData->depth = realDepth;
   surfaceData->arrayLayers = realArrayLayers;
   surfaceData->subresRange = subresRange;
   surfaceData->activeLayout = vk::ImageLayout::eUndefined;
   surfaceData->group = _getSurfaceGroup(info);

   return surfaceData;
}

void
Driver::_releaseSurface(SurfaceObject *surfaceData)
{
   // TODO: Add support for releasing surface data...
}

SurfaceObject *
Driver::getSurface(const SurfaceDesc& info)
{
   auto& foundImage = mSurfaces[info.hash()];
   if (!foundImage) {
      foundImage = _allocateSurface(info);
   }

   return foundImage;
}

void
Driver::_barrierSurface(SurfaceObject *surfaceData, ResourceUsage usage, vk::ImageLayout layout)
{
   switch (usage) {
   case ResourceUsage::ColorAttachment:
      decaf_check(layout == vk::ImageLayout::eColorAttachmentOptimal);
      break;
   case ResourceUsage::DepthStencilAttachment:
      decaf_check(layout == vk::ImageLayout::eDepthStencilAttachmentOptimal);
      break;
   case ResourceUsage::Texture:
      decaf_check(layout == vk::ImageLayout::eShaderReadOnlyOptimal);
      break;
   case ResourceUsage::TransferSrc:
      decaf_check(layout == vk::ImageLayout::eTransferSrcOptimal);
      break;
   case ResourceUsage::TransferDst:
      decaf_check(layout == vk::ImageLayout::eTransferDstOptimal);
      break;
   default:
      decaf_abort("Unexpected surface resource usage");
   }

   if (surfaceData->activeLayout == layout) {
      // Nothing to do, we are already the correct layout
      return;
   }

   vk::ImageMemoryBarrier imageBarrier;
   imageBarrier.srcAccessMask = vk::AccessFlags();
   imageBarrier.dstAccessMask = vk::AccessFlags();
   imageBarrier.oldLayout = surfaceData->activeLayout;
   imageBarrier.newLayout = layout;
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

   surfaceData->activeLayout = layout;
}

void
Driver::transitionSurface(SurfaceObject *surfaceData, ResourceUsage usage, vk::ImageLayout layout)
{
   // Activate this specific surface in the group, this will perform any
   // neccessary downloads and/or image transfers needed.
   _activateGroupSurface(surfaceData->group, surfaceData, usage);

   // Barrier the surface so that we know what its going to be used for.
   _barrierSurface(surfaceData, usage, layout);
}

SurfaceViewObject *
Driver::_allocateSurfaceView(const SurfaceViewDesc& info)
{
   auto surface = getSurface(info.surfaceDesc);

   auto hostFormat = getVkSurfaceFormat(info.surfaceDesc.format, info.surfaceDesc.tileType);

   vk::ImageViewType imageViewType;
   switch (info.surfaceDesc.dim) {
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
      decaf_abort(fmt::format("Failed to pick vulkan image view type for dim {}", info.surfaceDesc.dim));
   }

   auto hostComponentMap = vk::ComponentMapping();
   hostComponentMap.r = getVkComponentSwizzle(info.channels[0]);
   hostComponentMap.g = getVkComponentSwizzle(info.channels[1]);
   hostComponentMap.b = getVkComponentSwizzle(info.channels[2]);
   hostComponentMap.a = getVkComponentSwizzle(info.channels[3]);

   vk::ImageViewCreateInfo imageViewDesc;
   imageViewDesc.image = surface->image;
   imageViewDesc.viewType = imageViewType;
   imageViewDesc.format = hostFormat;
   imageViewDesc.components = hostComponentMap;
   imageViewDesc.subresourceRange = surface->subresRange;
   auto imageView = mDevice.createImageView(imageViewDesc);

   auto compName = [](latte::SQ_SEL comp) {
      static const char *names[] = { "x", "y", "z", "w", "0", "1", "_" };
      return names[comp];
   };
   setVkObjectName(imageView,
                   fmt::format(
                      "surfview_{:08x}:{}{}{}{}",
                      info.surfaceDesc.baseAddress,
                      compName(info.channels[0]),
                      compName(info.channels[1]),
                      compName(info.channels[2]),
                      compName(info.channels[3])).c_str());

   auto surfaceView = new SurfaceViewObject();
   surfaceView->desc = info;
   surfaceView->surface = surface;
   surfaceView->imageView = imageView;
   return surfaceView;
}

void
Driver::_releaseSurfaceView(SurfaceViewObject *surfaceView)
{
   // TODO: Add support for releasing surfaces...
}

SurfaceViewObject *
Driver::getSurfaceView(const SurfaceViewDesc& info)
{
   auto &surfaceView = mSurfaceViews[info.hash()];
   if (!surfaceView) {
      surfaceView = _allocateSurfaceView(info);
   }

   return surfaceView;
}

void
Driver::transitionSurfaceView(SurfaceViewObject *surfaceView, ResourceUsage usage, vk::ImageLayout layout)
{
   transitionSurface(surfaceView->surface, usage, layout);
}


} // namespace vulkan

#endif // ifdef DECAF_VULKAN
