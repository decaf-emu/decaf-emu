#ifdef DECAF_VULKAN
#include "vulkan_driver.h"
#include "vulkan_utils.h"
#include "vulkan_rangecombiner.h"

#include "gpu_tiling.h"
#include "latte/latte_formats.h"

namespace vulkan
{

static inline SectionRange
_sliceRangeToSectionRange(const SurfaceSubRange& range)
{
   // Because of the way our memory cache retiler works, we can guarentee
   // that our slices are mapped 1:1 into sections.
   return { range.firstSlice, range.numSlices };
}

static inline std::pair<uint32_t, uint32_t>
_sliceRangeToMemRange(SurfaceObject *surface, const SurfaceSubRange& range)
{
   auto& memCache = surface->memCache;
   decaf_check(memCache->mutator.mode == MemCacheMutator::Mode::Retile);
   auto& retile = memCache->mutator.retile;

   auto sliceSize = retile.pitch * retile.height * retile.bpp / 8;

   auto slicesOffset = range.firstSlice * sliceSize;
   auto slicesSize = range.numSlices * sliceSize;

   return { slicesOffset, slicesSize };
}

MemCacheObject *
Driver::_getSurfaceMemCache(const SurfaceDesc &info)
{
   auto realAddr = info.calcAlignedBaseAddress();
   auto swizzle = info.calcSwizzle();
   auto tileMode = info.tileMode;
   auto dataFormat = getSurfaceFormatDataFormat(info.format);
   auto bpp = latte::getDataFormatBitsPerElement(dataFormat);
   auto isDepthBuffer = (info.tileType == latte::SQ_TILE_TYPE::DEPTH);
   auto aa = uint32_t(0);

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

   gpu::alignTiling(tileMode, swizzle,
                    texelPitch, texelWidth, texelHeight, texelDepth,
                    aa, isDepthBuffer, bpp);

   auto sliceSize = texelPitch * texelHeight * bpp / 8;
   auto imageSize = sliceSize * texelDepth;

   std::vector<uint32_t> sliceSizes;
   for (auto i = 0u; i < texelDepth; ++i) {
      sliceSizes.push_back(sliceSize);
   }

   // Generate a mutator and grab the memory
   MemCacheMutator mutator;
   mutator.mode = MemCacheMutator::Mode::Retile;
   mutator.retile.tileMode = tileMode;
   mutator.retile.swizzle = swizzle;
   mutator.retile.pitch = texelPitch;
   mutator.retile.width = texelWidth;
   mutator.retile.height = texelHeight;
   mutator.retile.depth = texelDepth;
   mutator.retile.aa = aa;
   mutator.retile.isDepth = isDepthBuffer;
   mutator.retile.bpp = bpp;
   return getMemCache(phys_addr(realAddr), imageSize, sliceSizes, mutator);
}

void
Driver::_copySurface(SurfaceObject *dst, SurfaceObject *src, SurfaceSubRange range)
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

   _barrierSurface(dst, ResourceUsage::TransferDst, vk::ImageLayout::eTransferDstOptimal, { 0, copyLayers });
   _barrierSurface(src, ResourceUsage::TransferSrc, vk::ImageLayout::eTransferSrcOptimal, { 0, copyLayers });

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
   auto surfaceGroup = new SurfaceGroupObject();
   surfaceGroup->desc = info;
   return surfaceGroup;
}

void
Driver::_releaseSurfaceGroup(SurfaceGroupObject *surfaceGroup)
{
   // TODO: Add support for releasing surface data...
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
   case latte::SQ_TEX_DIM::DIM_2D_MSAA:
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
   case latte::SQ_TEX_DIM::DIM_2D_ARRAY_MSAA:
      imageType = vk::ImageType::e2D;
      realPitch = info.pitch;
      realWidth = info.width;
      realHeight = info.height;
      realDepth = 1;
      realArrayLayers = info.depth;
      break;
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

   static uint64_t surfImgIndex = 0;
   setVkObjectName(image, fmt::format("surfimg_{}_{:08x}", surfImgIndex++, info.baseAddress).c_str());

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

   // Grab a reference to the memory cache that backs this surface
   auto memCache = _getSurfaceMemCache(info);

   // TODO: Maybe join together the getSurfaceMemCache code and this?
   // Generate some meta-data about how we copy in/out
   auto alignedPitch = realPitch;
   auto alignedHeight = realHeight;
   if (memCache->mutator.mode == MemCacheMutator::Mode::Retile) {
      alignedPitch = memCache->mutator.retile.pitch;
      alignedHeight = memCache->mutator.retile.height;

      if (dataFormat >= latte::SQ_DATA_FORMAT::FMT_BC1 && dataFormat <= latte::SQ_DATA_FORMAT::FMT_BC5) {
         alignedPitch *= 4;
         alignedHeight *= 4;
      }
   }

   vk::BufferImageCopy bufferRegion = {};
   bufferRegion.bufferOffset = 0;
   bufferRegion.bufferRowLength = alignedPitch;
   bufferRegion.bufferImageHeight = alignedHeight;

   bufferRegion.imageSubresource.aspectMask = subresRange.aspectMask;
   bufferRegion.imageSubresource.mipLevel = 0;
   bufferRegion.imageSubresource.baseArrayLayer = 0;
   bufferRegion.imageSubresource.layerCount = realArrayLayers;

   bufferRegion.imageOffset = { 0, 0, 0 };
   bufferRegion.imageExtent = { static_cast<uint32_t>(realWidth),
                           static_cast<uint32_t>(realHeight),
                           static_cast<uint32_t>(realDepth) };

   std::vector<SurfaceSlice> slices;
   for (auto i = 0; i < realArrayLayers; ++i) {
      SurfaceSlice slice;
      slice.lastChangeIndex = 0;
      slices.push_back(slice);
   }

   // Return our freshly minted surface data object
   auto surface = new SurfaceObject();
   surface->desc = info;
   surface->image = image;
   surface->imageMem = imageMem;
   surface->pitch = realPitch;
   surface->width = realWidth;
   surface->height = realHeight;
   surface->depth = realDepth;
   surface->arrayLayers = realArrayLayers;
   surface->slices = std::move(slices);
   surface->memCache = memCache;
   surface->subresRange = subresRange;
   surface->bufferRegion = bufferRegion;
   surface->activeLayout = vk::ImageLayout::eUndefined;
   surface->activeUsage = ResourceUsage::Undefined;
   surface->lastUsageIndex = mActiveBatchIndex;
   surface->group = _getSurfaceGroup(info);

   return surface;
}

void
Driver::_releaseSurface(SurfaceObject *surface)
{
   // TODO: Add support for releasing surface data...

   delete surface;
}

void
Driver::_upgradeSurface(SurfaceObject *surface, const SurfaceDesc &info)
{
   // Allocate the new surface
   auto newSurface = _allocateSurface(info);

   // Verify that we are not making any errors
   // TODO: Reenable this check with support for array upgrades.
   //decaf_check(newSurface->desc.dim == surface->desc.dim);
   decaf_check(newSurface->desc.tileType == surface->desc.tileType);
   decaf_check(newSurface->desc.tileMode == surface->desc.tileMode);
   decaf_check(newSurface->desc.format == surface->desc.format);
   decaf_check(newSurface->width == surface->width);
   decaf_check(newSurface->height == surface->height);
   decaf_check(newSurface->depth == surface->depth);
   decaf_check(newSurface->arrayLayers > surface->arrayLayers);

   _copySurface(newSurface, surface, { 0, surface->arrayLayers });

   newSurface->lastUsageIndex = surface->lastUsageIndex;
   for (auto i = 0u; i < surface->slices.size(); ++i) {
      newSurface->slices[i] = surface->slices[i];
   }

   // Switch out the surfaces from the map
   auto switchIter = mSurfaces.find(info.hash());
   decaf_check(switchIter->second == surface);
   std::swap(*switchIter->second, *newSurface);

   // Release the surface on the next frame (an earlier reference to this surface
   // might have bound it to Vulkan, so we need to wait).
   addRetireTask([=](){
      _releaseSurface(newSurface);
   });
}

void
Driver::_readSurfaceData(SurfaceObject *surface, SurfaceSubRange range)
{
   if (surface->desc.tileType == latte::SQ_TILE_TYPE::DEPTH) {
      // TODO: Implement uploading of depth buffer data (you know... just in case?)
      return;
   }

   if (surface->desc.format == latte::SurfaceFormat::R11G11B10Float) {
      // TODO: Handle format conversion of R11G11B10 formats...
      return;
   }

   auto& memCache = surface->memCache;
   auto memRange = _sliceRangeToMemRange(surface, range);
   auto sectionRange = _sliceRangeToSectionRange(range);

   auto region = surface->bufferRegion;
   region.bufferOffset = memRange.first;
   region.imageSubresource.baseArrayLayer = range.firstSlice;
   region.imageSubresource.layerCount = range.numSlices;

   _barrierMemCache(surface->memCache, ResourceUsage::TransferSrc, sectionRange);
   _barrierSurface(surface, ResourceUsage::TransferDst, vk::ImageLayout::eTransferDstOptimal, range);

   mActiveCommandBuffer.copyBufferToImage(
      memCache->buffer,
      surface->image,
      vk::ImageLayout::eTransferDstOptimal,
      { region });
}

void
Driver::_writeSurfaceData(SurfaceObject *surface, SurfaceSubRange range)
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

   auto& memCache = surface->memCache;
   auto memRange = _sliceRangeToMemRange(surface, range);
   auto sectionRange = _sliceRangeToSectionRange(range);

   auto region = surface->bufferRegion;
   region.bufferOffset = memRange.first;
   region.imageSubresource.baseArrayLayer = range.firstSlice;
   region.imageSubresource.layerCount = range.numSlices;

   _barrierMemCache(surface->memCache, ResourceUsage::TransferDst, sectionRange);
   _barrierSurface(surface, ResourceUsage::TransferSrc, vk::ImageLayout::eTransferSrcOptimal, range);

   mActiveCommandBuffer.copyImageToBuffer(
      surface->image,
      vk::ImageLayout::eTransferSrcOptimal,
      memCache->buffer,
      { region });
}

void
Driver::_refreshSurface(SurfaceObject *surface, SurfaceSubRange range)
{
   auto sectionRange = _sliceRangeToSectionRange(range);
   auto& memCache = surface->memCache;

   // We manually call refresh here, as in most cases a barrier on the memory
   // and a full data load will be unneccessary.
   surface->memCache->lastUsageIndex = surface->lastUsageIndex;
   _refreshMemCache(memCache, sectionRange);

   RangeCombiner<SurfaceObject*, uint32_t, uint32_t> readCombiner(
   [&](SurfaceObject* object, uint32_t start, uint32_t count){
      _readSurfaceData(surface, { start, count });
   });

   for (auto i = sectionRange.start; i < sectionRange.start + sectionRange.count; ++i) {
      auto latestChangeIndex = memCache->sections[i].wantedChangeIndex;

      if (surface->slices[i].lastChangeIndex >= latestChangeIndex) {
         continue;
      }

      readCombiner.push(nullptr, i, 1);

      surface->slices[i].lastChangeIndex = latestChangeIndex;
   }

   readCombiner.flush();
}

void
Driver::_invalidateSurface(SurfaceObject *surface, SurfaceSubRange range)
{
   auto& memCache = surface->memCache;

   auto memRange = _sliceRangeToMemRange(surface, range);

   // Mark the memory cache as delayed invalidated, and perform the write
   // if the memory cache ends up requesting it (sometimes its cancelled).
   invalidateMemCacheDelayed(memCache, memRange.first, memRange.second, [=](){
      // We need to be careful to restore the surface layout after we are
      // done, as this async download could happen mid-draw.
      auto oldUsage = surface->activeUsage;
      auto oldLayout = surface->activeLayout;

      _writeSurfaceData(surface, range);

      _barrierSurface(surface, oldUsage, oldLayout, range);
   });

   // Update our last change index to match the data we wrote
   auto sectionRange = _sliceRangeToSectionRange(range);
   for (auto i = sectionRange.start; i < sectionRange.start + sectionRange.count; ++i) {
      surface->slices[i].lastChangeIndex = memCache->sections[i].lastChangeIndex;
   }
}

void
Driver::_barrierSurface(SurfaceObject *surface, ResourceUsage usage, vk::ImageLayout layout, SurfaceSubRange range)
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

   surface->activeUsage = usage;

   if (surface->activeLayout == layout) {
      // Nothing to do, we are already the correct layout
      return;
   }

   vk::ImageMemoryBarrier imageBarrier;
   imageBarrier.srcAccessMask = vk::AccessFlags();
   imageBarrier.dstAccessMask = vk::AccessFlags();
   imageBarrier.oldLayout = surface->activeLayout;
   imageBarrier.newLayout = layout;
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

   surface->activeLayout = layout;
}

SurfaceObject *
Driver::getSurface(const SurfaceDesc& info)
{
   auto& surface = mSurfaces[info.hash()];
   if (!surface) {
      surface = _allocateSurface(info);
   }

   if (info.pitch > surface->desc.pitch ||
       info.width > surface->desc.width ||
       info.height > surface->desc.height ||
       info.depth > surface->desc.depth) {
      _upgradeSurface(surface, info);
   }

   // Check we got the surface we wanted, not that we check height,depth
   // as a greater-equal to easily handle the array cases.  We also skip
   // DIM check, since it can go from 2D to 2D_ARRAY.
   decaf_check(surface->desc.baseAddress == info.baseAddress);
   decaf_check(surface->desc.pitch == info.pitch);
   decaf_check(surface->desc.width == info.width);
   decaf_check(surface->desc.height >= info.height);
   decaf_check(surface->desc.depth >= info.depth);
   decaf_check(surface->desc.samples == info.samples);
   //decaf_check(surface->desc.dim == info.dim);
   decaf_check(surface->desc.tileType == info.tileType);
   decaf_check(surface->desc.tileMode == info.tileMode);
   decaf_check(surface->desc.format == info.format);

   return surface;
}

void
Driver::transitionSurface(SurfaceObject *surface, ResourceUsage usage, vk::ImageLayout layout, SurfaceSubRange range)
{
   if (surface->desc.dim == latte::SQ_TEX_DIM::DIM_3D) {
      decaf_check(range.firstSlice % surface->desc.depth == 0);
      decaf_check(range.numSlices % surface->desc.depth == 0);
      range.firstSlice /= surface->desc.depth;
      range.numSlices /= surface->desc.depth;
   }

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

   surface->lastUsageIndex = mActiveBatchIndex;

   if (!forWrite) {
      _refreshSurface(surface, range);
   } else {
      _invalidateSurface(surface, range);
   }

   _barrierSurface(surface, usage, layout, range);
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
   case latte::SQ_TEX_DIM::DIM_2D_MSAA:
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
   case latte::SQ_TEX_DIM::DIM_2D_ARRAY_MSAA:
      imageViewType = vk::ImageViewType::e2DArray;
      break;
   default:
      decaf_abort(fmt::format("Failed to pick vulkan image view type for dim {}", info.surfaceDesc.dim));
   }

   auto hostComponentMap = vk::ComponentMapping();
   hostComponentMap.r = getVkComponentSwizzle(info.channels[0]);
   hostComponentMap.g = getVkComponentSwizzle(info.channels[1]);
   hostComponentMap.b = getVkComponentSwizzle(info.channels[2]);
   hostComponentMap.a = getVkComponentSwizzle(info.channels[3]);

   auto realSliceStart = info.sliceStart;
   auto realSliceCount = info.sliceEnd - info.sliceStart;

   // 3D textures behave identically to 2D_ARRAY except that they reduce
   // the number of array elements for each mip level.  Latte selects them
   // the same as a 3D texture, but we need to translate that.
   if (info.surfaceDesc.dim == latte::SQ_TEX_DIM::DIM_3D) {
      decaf_check(realSliceStart % info.surfaceDesc.depth == 0);
      decaf_check(realSliceCount % info.surfaceDesc.depth == 0);
      realSliceStart /= info.surfaceDesc.depth;
      realSliceCount /= info.surfaceDesc.depth;
   }

   auto subresRange = surface->subresRange;
   subresRange.baseArrayLayer = realSliceStart;
   subresRange.layerCount = realSliceCount;

   vk::ImageViewCreateInfo imageViewDesc;
   imageViewDesc.image = surface->image;
   imageViewDesc.viewType = imageViewType;
   imageViewDesc.format = hostFormat;
   imageViewDesc.components = hostComponentMap;
   imageViewDesc.subresourceRange = subresRange;
   auto imageView = mDevice.createImageView(imageViewDesc);

   auto compName = [](latte::SQ_SEL comp) {
      static const char *names[] = { "x", "y", "z", "w", "0", "1", "_" };
      return names[comp];
   };
   static uint64_t imageViewIndex = 0;
   setVkObjectName(imageView,
                   fmt::format(
                      "surfview_{}_{:08x}:{}{}{}{}",
                      imageViewIndex++,
                      info.surfaceDesc.baseAddress,
                      compName(info.channels[0]),
                      compName(info.channels[1]),
                      compName(info.channels[2]),
                      compName(info.channels[3])).c_str());

   SurfaceSubRange range;
   range.firstSlice = info.sliceStart;
   range.numSlices = info.sliceEnd - info.sliceStart;

   auto surfaceView = new SurfaceViewObject();
   surfaceView->desc = info;
   surfaceView->surfaceRange = range;
   surfaceView->surface = surface;
   surfaceView->imageView = imageView;
   surfaceView->subresRange = subresRange;
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

   decaf_check(surfaceView->desc.sliceStart == info.sliceStart);
   decaf_check(surfaceView->desc.sliceEnd == info.sliceEnd);

   return surfaceView;
}

void
Driver::transitionSurfaceView(SurfaceViewObject *surfaceView, ResourceUsage usage, vk::ImageLayout layout)
{
   transitionSurface(surfaceView->surface, usage, layout, surfaceView->surfaceRange);
}


} // namespace vulkan

#endif // ifdef DECAF_VULKAN
