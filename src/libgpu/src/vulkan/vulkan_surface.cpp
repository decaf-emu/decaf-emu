#ifdef DECAF_VULKAN
#include "vulkan_driver.h"
#include "vulkan_utils.h"

#include "gpu_tiling.h"
#include "latte/latte_formats.h"
#include <common/rangecombiner.h>

namespace vulkan
{

static inline std::string
_makeSurfaceDescStr(const SurfaceDesc& info)
{
   static const char *DIM_NAMES[] = {
      "1d", "2d", "3d", "cube", "1darr", "2darr", "2daa", "2daaarr" };

   return fmt::format("{:08x}_{:x}_{}_{:x}_{:x}_{:x}_{}x{}x{}x{}#{}",
                      info.calcAlignedBaseAddress(),
                      info.calcSwizzle(),
                      DIM_NAMES[info.dim],
                      info.format,
                      info.tileType,
                      info.tileMode,
                      info.pitch,
                      info.width,
                      info.height,
                      info.depth,
                      info.samples);
}

static inline std::string
_makeSurfaceName(const SurfaceDesc& info)
{
   static uint64_t surfImgIndex = 0;
   return fmt::format("surf_{}:{}",
                      surfImgIndex++,
                      _makeSurfaceDescStr(info));
}

static inline std::string
_makeSurfaceViewName(const SurfaceViewDesc& info)
{
   static const char *COMP_NAMES[] = {
      "x", "y", "z", "w", "0", "1", "_" };

   static uint64_t imageViewIndex = 0;
   return fmt::format("sview_{}:{}:{}{}{}{}:{}-{}",
                      imageViewIndex++,
                      _makeSurfaceDescStr(info.surfaceDesc),
                      COMP_NAMES[info.channels[0]],
                      COMP_NAMES[info.channels[1]],
                      COMP_NAMES[info.channels[2]],
                      COMP_NAMES[info.channels[3]],
                      info.sliceStart,
                      info.sliceEnd);
}

static inline SectionRange
_sliceRangeToSectionRange(const SurfaceSubRange& range)
{
   // Because of the way our memory cache retiler works, we can guarentee
   // that our slices are mapped 1:1 into sections.
   return { range.firstSlice, range.numSlices };
}

static inline uint32_t
_unthickenedSliceSize(const gpu7::tiling::SurfaceInfo &info)
{
   return info.pitch * info.height * info.bpp / 8;
}

static inline std::pair<uint32_t, uint32_t>
_sliceRangeToMemRange(SurfaceObject *surface, const SurfaceSubRange& range)
{
   auto sliceSize = _unthickenedSliceSize(surface->tilingInfo);
   auto slicesOffset = range.firstSlice * sliceSize;
   auto slicesSize = range.numSlices * sliceSize;

   return { slicesOffset, slicesSize };
}

static inline gpu7::tiling::SurfaceDescription
_getTilingSurfaceDesc(const SurfaceDesc &info)
{
   auto swizzle = info.calcSwizzle();
   auto dataFormat = getSurfaceFormatDataFormat(info.format);
   auto bpp = latte::getDataFormatBitsPerElement(dataFormat);

   /*
   AddrTileMode tileMode;
   AddrFormat format;
   uint32_t bpp;
   uint32_t numSamples;
   uint32_t width;
   uint32_t height;
   uint32_t numSlices;
   ADDR_SURFACE_FLAGS flags;
   uint32_t numFrags;
   uint32_t numLevels;
   uint32_t bankSwizzle;
   uint32_t pipeSwizzle;

   */

   gpu7::tiling::SurfaceDescription tilingDesc;
   tilingDesc.tileMode = static_cast<gpu7::tiling::TileMode>(info.tileMode);
   tilingDesc.format = static_cast<AddrFormat>(dataFormat);
   tilingDesc.bpp = bpp;
   tilingDesc.numSamples = 1;
   tilingDesc.width = info.pitch;
   tilingDesc.height = info.height;
   tilingDesc.numSlices = info.depth;
   tilingDesc.flags.depth = (info.tileType == latte::SQ_TILE_TYPE::DEPTH);
   tilingDesc.flags.volume = (info.dim == latte::SQ_TEX_DIM::DIM_3D);
   tilingDesc.numFrags = 0;
   tilingDesc.numLevels = 1;
   tilingDesc.pipeSwizzle = (swizzle >> 8) & 1;
   tilingDesc.bankSwizzle = (swizzle >> 9) & 3;

   /*
   if (dataFormat >= latte::SQ_DATA_FORMAT::FMT_BC1 && dataFormat <= latte::SQ_DATA_FORMAT::FMT_BC5) {
      tilingDesc.width = (tilingDesc.width + 3) / 4;
      tilingDesc.height = (tilingDesc.height + 3) / 4;
   }
   */

   return tilingDesc;
}

MemCacheObject *
Driver::_getSurfaceMemCache(const SurfaceDesc &info, const gpu7::tiling::SurfaceInfo& tilingInfo)
{
   auto realAddr = info.calcAlignedBaseAddress();

   auto sliceSize = _unthickenedSliceSize(tilingInfo);
   auto imageSize = static_cast<uint32_t>(tilingInfo.surfSize);
   auto alignedDepth = tilingInfo.depth;

   // Grab a memory cache object for this image
   return getMemCache(phys_addr(realAddr), alignedDepth, sliceSize);
}

void
Driver::_copySurface(SurfaceObject *dst, SurfaceObject *src, SurfaceSubRange range)
{
   // TODO: Add support for copying 1D Array's here...
   decaf_check(src->desc.dim != latte::SQ_TEX_DIM::DIM_1D_ARRAY);
   decaf_check(dst->desc.dim != latte::SQ_TEX_DIM::DIM_1D_ARRAY);

   // TODO: Add support for copying depth buffers here...
   decaf_check(dst->desc.tileType == src->desc.tileType);
   //decaf_check(dst->desc.tileType == latte::SQ_TILE_TYPE::DEFAULT);

   auto copyWidth = std::min(dst->width, src->width);
   auto copyHeight = std::min(dst->height, src->height);

   auto srcSlices = src->arrayLayers;
   if (src->desc.dim == latte::SQ_TEX_DIM::DIM_3D) {
      decaf_check(src->arrayLayers == 1);
      srcSlices = src->depth;
   }

   auto dstSlices = dst->arrayLayers;
   if (dst->desc.dim == latte::SQ_TEX_DIM::DIM_3D) {
      decaf_check(dst->arrayLayers == 1);
      dstSlices = dst->depth;
   }

   auto copySlices = std::min(srcSlices, dstSlices);

   if (range.firstSlice >= copySlices) {
      // We cannot perform any work, since the requested slice start
      // is beyond the end of one of the surfaces!
      return;
   }

   if (range.firstSlice + range.numSlices > copySlices) {
      // If the requested end is beyond the size of one of the surfaces,
      // lets shrink the range to only cover the available layers.
      range.numSlices = copySlices - range.firstSlice;
   }

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

   vk::ImageCopy copyRegion;
   if (src->desc.dim == latte::SQ_TEX_DIM::DIM_3D) {
      copyRegion.srcOffset = vk::Offset3D { 0, 0, static_cast<int32_t>(range.firstSlice) };
      copyRegion.srcSubresource = { copyAspect, 0, 0, 1 };
      // range.numSlices is expressed by the extent here...
   } else {
      copyRegion.srcOffset = vk::Offset3D { 0, 0, 0 };
      copyRegion.srcSubresource = { copyAspect, 0, range.firstSlice, range.numSlices };
   }
   if (dst->desc.dim == latte::SQ_TEX_DIM::DIM_3D) {
      copyRegion.dstOffset = vk::Offset3D { 0, 0, static_cast<int32_t>(range.firstSlice) };
      copyRegion.dstSubresource = { copyAspect, 0, 0, 1 };
      // range.numSlices is expressed by the extent here...
   } else {
      copyRegion.dstOffset = vk::Offset3D { 0, 0, 0 };
      copyRegion.dstSubresource = { copyAspect, 0, range.firstSlice, range.numSlices };

      // range.numSlices is expressed by the extent here...
   }
   copyRegion.extent = vk::Extent3D { copyWidth, copyHeight, copySlices };

   auto originalSrcUsage = src->activeUsage;

   _barrierSurface(src, ResourceUsage::TransferSrc, vk::ImageLayout::eTransferSrcOptimal, range);
   _barrierSurface(dst, ResourceUsage::TransferDst, vk::ImageLayout::eTransferDstOptimal, range);

   mActiveCommandBuffer.copyImage(
      src->image,
      vk::ImageLayout::eTransferSrcOptimal,
      dst->image,
      vk::ImageLayout::eTransferDstOptimal,
      { copyRegion });

   // We don't know what the source was doing before, so we need to return
   // it back to it's original usage/layout in case its in use.
   if (originalSrcUsage != ResourceUsage::Undefined) {
      auto originalSrcLayout = getResourceUsageMeta(originalSrcUsage).imageLayout;
      _barrierSurface(src, originalSrcUsage, originalSrcLayout, range);
   }
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
   decaf_check(surfaceGroup->surfaces.empty());
   delete surfaceGroup;
}

void
Driver::_addSurfaceGroupSurface(SurfaceGroupObject *surfaceGroup, SurfaceObject *surface)
{
   surfaceGroup->surfaces.push_back(surface);

   auto sliceCount = surface->slices.size();
   while (surfaceGroup->sliceOwners.size() < sliceCount) {
      surfaceGroup->sliceOwners.push_back(nullptr);
   }
}

void
Driver::_removeSurfaceGroupSurface(SurfaceGroupObject *surfaceGroup, SurfaceObject *surface)
{
   for (auto& sliceOwner : surfaceGroup->sliceOwners) {
      if (sliceOwner == surface) {
         sliceOwner = nullptr;
      }
   }

   surfaceGroup->surfaces.remove(surface);
}

void
Driver::_updateSurfaceGroupSlice(SurfaceGroupObject *surfaceGroup, uint32_t sliceId, SurfaceObject *surface)
{
   decaf_check(sliceId < surfaceGroup->sliceOwners.size());
   surfaceGroup->sliceOwners[sliceId] = surface;
}

SurfaceObject *
Driver::_getSurfaceGroupOwner(SurfaceGroupObject *surfaceGroup, uint32_t sliceId, uint64_t minChangeIndex)
{
   decaf_check(sliceId < surfaceGroup->sliceOwners.size());
   auto& sliceOwner = surfaceGroup->sliceOwners[sliceId];
   if (!sliceOwner) {
      return nullptr;
   }

   if (sliceOwner->slices[sliceId].lastChangeIndex < minChangeIndex) {
      return nullptr;
   }

   return sliceOwner;
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
   auto realPitch = 1u;
   auto realWidth = 1u;
   auto realHeight = 1u;
   auto realDepth = 1u;
   auto realArrayLayers = 1u;

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
      realArrayLayers = info.depth;
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

   setVkObjectName(image, _makeSurfaceName(info).c_str());

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

   auto tilingDesc = _getTilingSurfaceDesc(info);
   auto tilingInfo = gpu7::tiling::computeSurfaceInfo(tilingDesc, 0, 0);

   // Grab a reference to the memory cache that backs this surface
   auto memCache = _getSurfaceMemCache(info, tilingInfo);

   // TODO: Maybe join together the getSurfaceMemCache code and this?
   // Generate some meta-data about how we copy in/out
   auto alignedPitch = tilingInfo.pitch;
   auto alignedHeight = tilingInfo.height;
   if (dataFormat >= latte::SQ_DATA_FORMAT::FMT_BC1 && dataFormat <= latte::SQ_DATA_FORMAT::FMT_BC5) {
      alignedPitch *= 4;
      alignedHeight *= 4;
   }

   vk::BufferImageCopy bufferRegion = {};
   bufferRegion.bufferOffset = 0;
   bufferRegion.bufferRowLength = alignedPitch;
   bufferRegion.bufferImageHeight = alignedHeight;

   bufferRegion.imageSubresource.aspectMask = subresRange.aspectMask;
   bufferRegion.imageSubresource.mipLevel = 0;
   bufferRegion.imageSubresource.baseArrayLayer = 0;
   bufferRegion.imageSubresource.layerCount = realArrayLayers;

   bufferRegion.imageOffset = vk::Offset3D { 0, 0, 0 };
   bufferRegion.imageExtent = vk::Extent3D { static_cast<uint32_t>(realWidth),
                                             static_cast<uint32_t>(realHeight),
                                             static_cast<uint32_t>(realDepth) };

   std::vector<SurfaceSlice> slices;

   if (info.dim == latte::SQ_TEX_DIM::DIM_3D) {
      // In the case of a 3D texture, we have to have a slice per DIM.  This
      // is the only way to enable us to render to the surface appropriately.
      decaf_check(realArrayLayers == 1);
      for (auto i = 0u; i < info.depth; ++i) {
         SurfaceSlice slice;
         slice.lastChangeIndex = 0;
         slices.push_back(slice);
      }
   } else {
      for (auto i = 0u; i < realArrayLayers; ++i) {
         SurfaceSlice slice;
         slice.lastChangeIndex = 0;
         slices.push_back(slice);
      }
   }

   auto surfaceGroup = _getSurfaceGroup(info);

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
   surface->tilingDesc = tilingDesc;
   surface->tilingInfo = tilingInfo;
   surface->slices = std::move(slices);
   surface->memCache = memCache;
   surface->subresRange = subresRange;
   surface->bufferRegion = bufferRegion;
   surface->activeUsage = ResourceUsage::Undefined;
   surface->lastUsageIndex = mActiveBatchIndex;
   surface->group = surfaceGroup;

   _addSurfaceGroupSurface(surfaceGroup, surface);

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

   // Remove the surface from the group (it was automatically added)
   _removeSurfaceGroupSurface(surface->group, newSurface);

   // Release the surface on the next frame (an earlier reference to this surface
   // might have bound it to Vulkan, so we need to wait).
   addRetireTask([=](){
      _releaseSurface(newSurface);
   });
}

void
Driver::_readSurfaceData(SurfaceObject *surface, SurfaceSubRange range)
{
   auto& memCache = surface->memCache;
   auto memRange = _sliceRangeToMemRange(surface, range);
   auto sectionRange = _sliceRangeToSectionRange(range);

   auto untiledOffset = memRange.first;
   auto untiledBuffer = memCache->buffer;

   auto retileInfo = gpu7::tiling::vulkan::calculateRetileInfo(surface->tilingDesc, range.firstSlice, range.numSlices);
   if (retileInfo.isTiled) {
      // Lets just double-check everyone is in agreement...
      // TODO: These wont match due to tile thickness needing to be aligned!
      //decaf_check(memRange.first == retileInfo.sliceOffset);

      // Calculate our retiling buffer size.
      auto retileSize = retileInfo.sliceBytes * range.numSlices;

      // Check that we are aligned based on our thickness.  This is critical to
      // ensure that we correctly invalidate the regions touched by the retiler.
      decaf_check(range.firstSlice % retileInfo.microTileThickness == 0);
      decaf_check(range.numSlices % retileInfo.microTileThickness == 0);
      retileSize /= retileInfo.microTileThickness;

      // Grab a staging buffer to write into before the image read
      auto retileStaging = getStagingBuffer(retileSize, StagingBufferType::GpuToGpu);

      // Calculate the real offset into our tiled data, the GPU retiler needs a buffer
      // offset that points directly to the slice.
      auto tiledOffset = retileInfo.sliceOffset;
      auto tiledBuffer = untiledBuffer;

      // Remap the untiled surface to our staging buffer
      untiledOffset = 0;
      untiledBuffer = retileStaging->buffer;

      _barrierMemCache(surface->memCache, ResourceUsage::ComputeSsboRead, sectionRange);

      dispatchGpuUntile(mActiveCommandBuffer, untiledBuffer, untiledOffset, tiledBuffer, tiledOffset, retileInfo);

      _barrierMemCache(surface->memCache, ResourceUsage::TransferSrc, sectionRange);
   } else {
      // We will directly read from the memory cache, since this is a linear surface
      _barrierMemCache(surface->memCache, ResourceUsage::TransferSrc, sectionRange);
   }

   // Actually load the surface

   auto region = surface->bufferRegion;
   region.bufferOffset = untiledOffset;
   if (surface->desc.dim == latte::SQ_TEX_DIM::DIM_3D) {
      region.imageOffset.z = range.firstSlice;
      region.imageExtent.depth = range.numSlices;
   } else {
      region.imageSubresource.baseArrayLayer = range.firstSlice;
      region.imageSubresource.layerCount = range.numSlices;
   }

   _barrierSurface(surface, ResourceUsage::TransferDst, vk::ImageLayout::eTransferDstOptimal, range);

   // TODO: Improve how we handle subresources everywhere.
   bool hasDepth = !!(region.imageSubresource.aspectMask & vk::ImageAspectFlagBits::eDepth);
   bool hasStencil = !!(region.imageSubresource.aspectMask & vk::ImageAspectFlagBits::eStencil);
   if (!(hasDepth & hasStencil)) {
      mActiveCommandBuffer.copyBufferToImage(
         untiledBuffer,
         surface->image,
         vk::ImageLayout::eTransferDstOptimal,
         { region });
   } else {
      auto region2 = region;
      region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eDepth;
      region2.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eStencil;

      mActiveCommandBuffer.copyBufferToImage(
         untiledBuffer,
         surface->image,
         vk::ImageLayout::eTransferDstOptimal,
         { region, region2 });
   }
}

void
Driver::_writeSurfaceData(SurfaceObject *surface, SurfaceSubRange range)
{
   auto& memCache = surface->memCache;
   auto memRange = _sliceRangeToMemRange(surface, range);
   auto sectionRange = _sliceRangeToSectionRange(range);

   auto untiledOffset = memRange.first;
   auto untiledBuffer = memCache->buffer;

   auto retileInfo = gpu7::tiling::vulkan::calculateRetileInfo(surface->tilingDesc, range.firstSlice, range.numSlices);
   if (retileInfo.isTiled) {
      // Calculate the retiling buffer size
      auto retileSize = retileInfo.sliceBytes * range.numSlices;

      // Check that we are aligned based on our thickness.  This is critical to
      // ensure that we correctly invalidate the regions touched by the retiler.
      decaf_check(range.firstSlice % retileInfo.microTileThickness == 0);
      decaf_check(range.numSlices % retileInfo.microTileThickness == 0);
      retileSize /= retileInfo.microTileThickness;

      // Grab our buffer used for retiling
      auto retileStaging = getStagingBuffer(retileSize, StagingBufferType::GpuToGpu);

      untiledOffset = 0;
      untiledBuffer = retileStaging->buffer;
   } else {
      // Write directly to the surface.
      _barrierMemCache(surface->memCache, ResourceUsage::TransferDst, sectionRange);
   }

   auto region = surface->bufferRegion;
   region.bufferOffset = untiledOffset;
   if (surface->desc.dim == latte::SQ_TEX_DIM::DIM_3D) {
      region.imageOffset.z = range.firstSlice;
      region.imageExtent.depth = range.numSlices;
   } else {
      region.imageSubresource.baseArrayLayer = range.firstSlice;
      region.imageSubresource.layerCount = range.numSlices;
   }

   _barrierSurface(surface, ResourceUsage::TransferSrc, vk::ImageLayout::eTransferSrcOptimal, range);

   // TODO: Improve how we handle subresources everywhere.
   bool hasDepth = !!(region.imageSubresource.aspectMask & vk::ImageAspectFlagBits::eDepth);
   bool hasStencil = !!(region.imageSubresource.aspectMask & vk::ImageAspectFlagBits::eStencil);
   if (!(hasDepth & hasStencil)) {
      mActiveCommandBuffer.copyImageToBuffer(
         surface->image,
         vk::ImageLayout::eTransferSrcOptimal,
         untiledBuffer,
         { region });
   } else {
      auto region2 = region;
      region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eDepth;
      region2.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eStencil;

      mActiveCommandBuffer.copyImageToBuffer(
         surface->image,
         vk::ImageLayout::eTransferSrcOptimal,
         untiledBuffer,
         { region, region2 });
   }

   if (retileInfo.isTiled) {
      _barrierMemCache(surface->memCache, ResourceUsage::ComputeSsboWrite, sectionRange);

      auto tiledBuffer = memCache->buffer;
      auto tiledOffset = retileInfo.sliceOffset;

      dispatchGpuTile(mActiveCommandBuffer, untiledBuffer, untiledOffset, tiledBuffer, tiledOffset, retileInfo);
   }
}

void
Driver::_refreshSurface(SurfaceObject *surface, SurfaceSubRange range)
{
   auto sectionRange = _sliceRangeToSectionRange(range);
   auto& memCache = surface->memCache;

   // We manually call refresh here, as in most cases a barrier on the memory
   // and a full data load will be unneccessary.
   surface->memCache->lastUsageIndex = surface->lastUsageIndex;
   _refreshMemCache_Check(memCache, sectionRange);

   auto readCombiner = makeRangeCombiner<SurfaceObject*, uint32_t, uint32_t>(
   [&](SurfaceObject* object, uint32_t start, uint32_t count){
      _refreshMemCache_Update(memCache, { start, count });
      _readSurfaceData(surface, { start, count });
   });

   auto blitCombiner = makeRangeCombiner<SurfaceObject*, uint32_t, uint32_t>(
   [&](SurfaceObject* object, uint32_t start, uint32_t count){
      _copySurface(surface, object, { start, count });
   });

   for (auto i = sectionRange.start; i < sectionRange.start + sectionRange.count; ++i) {
      auto latestChangeIndex = memCache->sections[i].wantedChangeIndex;

      if (surface->slices[i].lastChangeIndex >= latestChangeIndex) {
         continue;
      }

      auto localOwner = _getSurfaceGroupOwner(surface->group, i, latestChangeIndex);
      if (localOwner) {
         decaf_check(!memCache->sections[i].needsUpload);
         blitCombiner.push(localOwner, i, 1);
      } else {
         readCombiner.push(nullptr, i, 1);
      }

      surface->slices[i].lastChangeIndex = latestChangeIndex;
   }

   readCombiner.flush();
   blitCombiner.flush();
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

      _writeSurfaceData(surface, range);

      auto oldLayout = getResourceUsageMeta(oldUsage).imageLayout;
      _barrierSurface(surface, oldUsage, oldLayout, range);
   });

   // Update our last change index to match the data we wrote
   auto sectionRange = _sliceRangeToSectionRange(range);
   for (auto i = sectionRange.start; i < sectionRange.start + sectionRange.count; ++i) {
      surface->slices[i].lastChangeIndex = memCache->sections[i].lastChangeIndex;

      // Mark the surface as the owner in this surface group
      _updateSurfaceGroupSlice(surface->group, i, surface);
   }
}

void
Driver::_barrierSurface(SurfaceObject *surface, ResourceUsage usage, vk::ImageLayout layout, SurfaceSubRange range)
{
   auto srcMeta = getResourceUsageMeta(surface->activeUsage);
   auto dstMeta = getResourceUsageMeta(usage);

   // Lets make sure everyone agrees on what the layout
   // actually needs to be for this transition.
   decaf_check(dstMeta.imageLayout == layout);

   if (surface->activeUsage == usage) {
      return;
   }

   vk::ImageMemoryBarrier imageBarrier;
   imageBarrier.srcAccessMask = srcMeta.accessFlags;
   imageBarrier.dstAccessMask = dstMeta.accessFlags;
   imageBarrier.oldLayout = srcMeta.imageLayout;
   imageBarrier.newLayout = dstMeta.imageLayout;
   imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   imageBarrier.image = surface->image;
   imageBarrier.subresourceRange = surface->subresRange;

   mActiveCommandBuffer.pipelineBarrier(
      srcMeta.stageFlags,
      dstMeta.stageFlags,
      vk::DependencyFlags(),
      {},
      {},
      { imageBarrier });

   surface->activeUsage = usage;
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
   decaf_check(surface->desc.calcAlignedBaseAddress() == info.calcAlignedBaseAddress());
   decaf_check(surface->desc.pitch == info.pitch);
   decaf_check(surface->desc.width == info.width);
   decaf_check(surface->desc.height >= info.height);
   decaf_check(surface->desc.depth >= info.depth);
   decaf_check(surface->desc.samples == info.samples);
   decaf_check(surface->desc.tileType == info.tileType);
   decaf_check(surface->desc.tileMode == info.tileMode);
   decaf_check(surface->desc.format == info.format);

   return surface;
}

void
Driver::transitionSurface(SurfaceObject *surface, ResourceUsage usage, vk::ImageLayout layout, SurfaceSubRange range, bool skipChangeCheck)
{
   // We need to align our invalidation groups along a tickness boundary!
   auto alignedRange = range;
   auto tileThickness = gpu7::tiling::getMicroTileThickness(surface->tilingInfo.tileMode);
   if (tileThickness > 1) {
      auto endSlice = range.firstSlice + range.numSlices;
      alignedRange.firstSlice = align_down(range.firstSlice, tileThickness);
      endSlice = align_up(endSlice, tileThickness);
      alignedRange.numSlices = endSlice - alignedRange.firstSlice;
   }

   surface->lastUsageIndex = mActiveBatchIndex;

   bool forWrite = getResourceUsageMeta(usage).isWrite;

   if (!skipChangeCheck) {
      _refreshSurface(surface, alignedRange);

      if (forWrite) {
         _invalidateSurface(surface, alignedRange);
      }
   }

   _barrierSurface(surface, usage, layout, alignedRange);
}

SurfaceViewObject *
Driver::_allocateSurfaceView(const SurfaceViewDesc& info)
{
   auto adjInfo = info;

   if (adjInfo.surfaceDesc.dim == latte::SQ_TEX_DIM::DIM_3D) {
      // The source GPU allows slice selection on 3D textures.  In order
      // to support this, we will actually have to adjust the sizing of
      // the underlying 3D textures so that the image view can correctly
      // see it (since we cannot view particular slices with this).
      decaf_check(adjInfo.sliceStart == 0);
      decaf_check(adjInfo.sliceEnd == adjInfo.surfaceDesc.depth);
      adjInfo.sliceEnd = 1;
   }

   auto surface = getSurface(adjInfo.surfaceDesc);

   auto subresRange = surface->subresRange;
   subresRange.baseArrayLayer = adjInfo.sliceStart;
   subresRange.layerCount = adjInfo.sliceEnd - adjInfo.sliceStart;

   // We still need to support invalidating specific slices in the underlying
   // image, in case someone renders to a specific slice of the image.
   SurfaceSubRange range;
   range.firstSlice = info.sliceStart;
   range.numSlices = info.sliceEnd - info.sliceStart;

   auto surfaceView = new SurfaceViewObject();
   surfaceView->desc = info;
   surfaceView->surfaceRange = range;
   surfaceView->surface = surface;
   //surfaceView->imageView = nullptr;
   //surfaceView->boundImage = nullptr;
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

   decaf_check(surfaceView->desc->sliceStart == info.sliceStart);
   decaf_check(surfaceView->desc->sliceEnd == info.sliceEnd);

   return surfaceView;
}

void
Driver::transitionSurfaceView(SurfaceViewObject *surfaceView, ResourceUsage usage, vk::ImageLayout layout, bool skipChangeCheck)
{
   transitionSurface(surfaceView->surface, usage, layout, surfaceView->surfaceRange, skipChangeCheck);

   if (surfaceView->boundImage == surfaceView->surface->image) {
      return;
   }

   auto& info = surfaceView->desc;
   auto& surface = surfaceView->surface;

   auto hostFormat = getVkSurfaceFormat(info->surfaceDesc.format, info->surfaceDesc.tileType);

   vk::ImageViewType imageViewType;
   switch (info->surfaceDesc.dim) {
   case latte::SQ_TEX_DIM::DIM_1D:
      imageViewType = vk::ImageViewType::e1D;
      break;
   case latte::SQ_TEX_DIM::DIM_2D:
   case latte::SQ_TEX_DIM::DIM_2D_MSAA:
      imageViewType = vk::ImageViewType::e2D;
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
   case latte::SQ_TEX_DIM::DIM_3D:
      imageViewType = vk::ImageViewType::e3D;
      break;
   default:
      decaf_abort(fmt::format("Failed to pick vulkan image view type for dim {}", info->surfaceDesc.dim));
   }

   auto hostComponentMap = vk::ComponentMapping();
   hostComponentMap.r = getVkComponentSwizzle(info->channels[0]);
   hostComponentMap.g = getVkComponentSwizzle(info->channels[1]);
   hostComponentMap.b = getVkComponentSwizzle(info->channels[2]);
   hostComponentMap.a = getVkComponentSwizzle(info->channels[3]);

   vk::ImageViewCreateInfo imageViewDesc;
   imageViewDesc.image = surface->image;
   imageViewDesc.viewType = imageViewType;
   imageViewDesc.format = hostFormat;
   imageViewDesc.components = hostComponentMap;
   imageViewDesc.subresourceRange = surfaceView->subresRange;
   auto imageView = mDevice.createImageView(imageViewDesc);

   setVkObjectName(imageView, _makeSurfaceViewName(*info).c_str());

   if (surfaceView->imageView) {
      auto oldImageView = surfaceView->imageView;
      addRetireTask([=](){
         mDevice.destroyImageView(oldImageView);
      });
      surfaceView->imageView = vk::ImageView();
   }

   surfaceView->imageView = imageView;
   surfaceView->boundImage = surface->image;
}


} // namespace vulkan

#endif // ifdef DECAF_VULKAN
