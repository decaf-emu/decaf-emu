#ifdef DECAF_VULKAN
#include "vulkan_driver.h"
#include "vulkan_rangecombiner.h"
#include "gpu_tiling.h"

namespace vulkan
{

static inline void
forEachMemSegment(MemSegmentMap::iterator begin, uint32_t size, std::function<void(MemCacheSegment*)> functor)
{
   auto iter = begin;
   for (auto sizeLeft = size; sizeLeft > 0;) {
      auto& segment = iter->second;
      functor(segment);

      decaf_check(segment->size <= sizeLeft);
      sizeLeft -= segment->size;

      iter++;
   }
}

static inline void
forEachSectionSegment(MemCacheObject *cache, SectionRange range, std::function<void(MemCacheSection&, MemCacheSegment*)> functor)
{
   auto rangeStart = range.start;
   auto rangeEnd = range.start + range.count;
   for (auto i = rangeStart; i < rangeEnd; ++i) {
      auto& section = cache->sections[i];
      forEachMemSegment(section.firstSegment, section.size, [&](MemCacheSegment *segment){
         functor(section, segment);
      });
   }
}

static inline void
forEachMemSegment(MemCacheObject *cache, SectionRange range, std::function<void(MemCacheSegment*)> functor)
{
   auto& firstSection = cache->sections[range.start];
   auto& lastSection = cache->sections[range.start + range.count - 1];

   auto begin = firstSection.firstSegment;
   auto size = lastSection.offset + lastSection.size - firstSection.offset;
   forEachMemSegment(begin, size, functor);
}

MemCacheObject *
Driver::_allocMemCache(phys_addr address, const std::vector<uint32_t>& sectionSizes, const MemCacheMutator& mutator)
{
   if (mutator.mode == MemCacheMutator::Mode::None) {
      // Raw mutators allow any section sizes
   } else if (mutator.mode == MemCacheMutator::Mode::Retile) {
      // In the case of a retiling mutator, we need to verify that the
      // section sizes perfectly match with slices.

      auto& retile = mutator.retile;
      auto sliceSize = retile.pitch * retile.height * retile.bpp / 8;

      for (auto i = 0; i < sectionSizes.size(); ++i) {
         decaf_check(sectionSizes[i] == sliceSize);
      }
   } else {
      decaf_abort("Unexpected mutator type");
   }


   uint32_t totalSize = 0;

   std::vector<MemCacheSection> sections;
   for (auto i = 0; i < sectionSizes.size(); ++i) {
      auto sectionSize = sectionSizes[i];

      auto firstSegment = _getMemSegment(address + totalSize, sectionSize);
      _ensureMemSegments(firstSegment, sectionSize);

      MemCacheSection section;
      section.offset = totalSize;
      section.size = sectionSize;
      section.lastChangeIndex = 0;
      section.firstSegment = firstSegment;
      section.needsUpload = false;
      section.wantedChangeIndex = 0;
      sections.push_back(section);

      totalSize += section.size;
   }

   vk::BufferCreateInfo bufferDesc;
   bufferDesc.size = totalSize;
   bufferDesc.usage =
      vk::BufferUsageFlagBits::eVertexBuffer |
      vk::BufferUsageFlagBits::eUniformBuffer |
      vk::BufferUsageFlagBits::eTransferDst |
      vk::BufferUsageFlagBits::eTransferSrc;
   bufferDesc.sharingMode = vk::SharingMode::eExclusive;
   bufferDesc.queueFamilyIndexCount = 0;
   bufferDesc.pQueueFamilyIndices = nullptr;

   VmaAllocationCreateInfo allocInfo = {};
   allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

   VkBuffer buffer;
   VmaAllocation allocation;
   vmaCreateBuffer(mAllocator,
                   reinterpret_cast<VkBufferCreateInfo*>(&bufferDesc),
                   &allocInfo,
                   &buffer,
                   &allocation,
                   nullptr);

   static uint64_t memCacheIndex = 0;
   setVkObjectName(buffer, fmt::format("memcache_{}_{:08x}_{}", memCacheIndex++, address.getAddress(), totalSize).c_str());

   auto cache = new MemCacheObject();
   cache->address = address;
   cache->size = totalSize;
   cache->mutator = mutator;
   cache->allocation = allocation;
   cache->buffer = buffer;
   cache->sections = std::move(sections);
   cache->delayedWriteFunc = nullptr;
   cache->delayedWriteRange = {};
   cache->lastUsageIndex = mActiveBatchIndex;
   cache->refCount = 0;
   return cache;
}

void
Driver::_uploadMemCacheRaw(MemCacheObject *cache, SectionRange range)
{
   uint8_t *cacheBasePtr = phys_cast<uint8_t*>(cache->address).getRawPointer();

   auto& firstSection = cache->sections[range.start];
   auto& lastSection = cache->sections[range.start + range.count - 1];
   auto uploadOffset = firstSection.offset;
   auto uploadSize = lastSection.offset + lastSection.size - firstSection.offset;
   uint8_t *uploadData = cacheBasePtr + uploadOffset;

   auto stagingBuffer = getStagingBuffer(uploadSize);
   void *mappedPtr = mapStagingBuffer(stagingBuffer, false);
   memcpy(mappedPtr, uploadData, uploadSize);
   unmapStagingBuffer(stagingBuffer, true);

   // Copy the data out of the staging buffer into the memory cache.
   vk::BufferCopy copyDesc;
   copyDesc.srcOffset = 0;
   copyDesc.dstOffset = uploadOffset;
   copyDesc.size = uploadSize;
   mActiveCommandBuffer.copyBuffer(stagingBuffer->buffer, cache->buffer, { copyDesc });
}

void
Driver::_uploadMemCacheRetile(MemCacheObject *cache, SectionRange range)
{
   auto& retile = cache->mutator.retile;
   auto sliceSize = retile.pitch * retile.height * retile.bpp / 8;
   auto imageSize = sliceSize * retile.depth;

   auto& firstSection = cache->sections[range.start];
   auto& lastSection = cache->sections[range.start + range.count - 1];
   auto startOffset = firstSection.offset;
   auto endOffset = lastSection.offset + lastSection.size;

   decaf_check(startOffset % sliceSize == 0);
   decaf_check(endOffset % sliceSize == 0);
   auto startSlice = startOffset / sliceSize;
   auto endSlice = endOffset / sliceSize;
   auto sliceCount = endSlice - startSlice;

   void *data = phys_cast<void*>(cache->address).getRawPointer();
   auto dataBytesPtr = reinterpret_cast<uint8_t*>(data);

   auto& untiledImage = mScratchRetiling;
   untiledImage.resize(imageSize);

   gpu::convertFromTiled(
      untiledImage.data(),
      retile.pitch,
      dataBytesPtr,
      retile.tileMode,
      retile.swizzle,
      retile.pitch,
      retile.pitch,
      retile.height,
      retile.depth,
      retile.aa,
      retile.isDepth,
      retile.bpp,
      startSlice,
      endSlice);

   auto uploadOffset = startSlice * sliceSize;
   auto uploadData = untiledImage.data() + uploadOffset;
   auto uploadSize = endOffset - startOffset;

   auto stagingBuffer = getStagingBuffer(uploadSize);
   void *mappedPtr = mapStagingBuffer(stagingBuffer, false);
   memcpy(mappedPtr, uploadData, uploadSize);
   unmapStagingBuffer(stagingBuffer, true);

   // Copy the data out of the staging buffer into the memory cache.
   vk::BufferCopy copyDesc;
   copyDesc.srcOffset = 0;
   copyDesc.dstOffset = uploadOffset;
   copyDesc.size = uploadSize;
   mActiveCommandBuffer.copyBuffer(stagingBuffer->buffer, cache->buffer, { copyDesc });
}

void
Driver::_uploadMemCache(MemCacheObject *cache, SectionRange range)
{
   if (cache->mutator.mode == MemCacheMutator::Mode::None) {
      _uploadMemCacheRaw(cache, range);
   } else if (cache->mutator.mode == MemCacheMutator::Mode::Retile) {
      _uploadMemCacheRetile(cache, range);
   } else {
      decaf_abort("Unsupported memory cache mutator mode");
   }
}

void
Driver::_downloadMemCacheRaw(MemCacheObject *cache, SectionRange range)
{
   auto& firstSection = cache->sections[range.start];
   auto& lastSection = cache->sections[range.start + range.count - 1];
   auto offsetStart = firstSection.offset;
   auto offsetEnd = lastSection.offset + lastSection.size;
   auto rangeSize = offsetEnd - offsetStart;

   // Create a staging buffer to use for the readback
   auto stagingBuffer = getStagingBuffer(rangeSize);

   // Copy the data into our staging buffer from the cache object
   vk::BufferCopy copyDesc;
   copyDesc.srcOffset = offsetStart;
   copyDesc.dstOffset = 0;
   copyDesc.size = rangeSize;
   mActiveCommandBuffer.copyBuffer(cache->buffer, stagingBuffer->buffer, { copyDesc });

   // Move the data onto the CPU on a per-section basis
   for (auto i = range.start; i < range.start + range.count; ++i) {
      auto& section = cache->sections[i];
      auto changeIndex = section.lastChangeIndex;

      addRetireTask([=](){
         void *data = phys_cast<void*>(cache->address + section.offset).getRawPointer();

         auto stagingOffset = section.offset - offsetStart;

         // Map our staging buffer so we can copy out of it.
         void *mappedPtr = mapStagingBuffer(stagingBuffer, true);
         auto stagedData = reinterpret_cast<uint8_t*>(mappedPtr) + stagingOffset;
         memcpy(data, stagedData, section.size);
         unmapStagingBuffer(stagingBuffer, false);

         // We need to calculate new data hashes for the relevant segments that
         // are affected by this image and are not still being GPU written.
         forEachMemSegment(section.firstSegment, section.size, [&](MemCacheSegment* segment){
            // For safety purposes, lets confirm that this write was intended.
            decaf_check(segment->lastChangeIndex >= section.lastChangeIndex);

            // Only bother recalculating the hashing if we are not already waiting
            // for more writes to this segment...
            if (segment->lastChangeIndex == changeIndex) {
               auto dataPtr = phys_cast<void*>(segment->address).getRawPointer();
               auto dataSize = segment->size;
               segment->dataHash = DataHash {}.write(dataPtr, dataSize);
               segment->gpuWritten = false;
            }
         });
      });
   }
}

void
Driver::_downloadMemCacheRetile(MemCacheObject *cache, SectionRange range)
{
   // We do not currently implement retiled downloads

   /*
   Note that in the upload code, we set width to pitch, so that we untile the whole
   pitch in all cases.  This ensures that if we copy memory from this block, we also
   copy the data from the pitch.  When we write back to the CPU, we only write the
   exact width that is being used by this particular buffer.

   gpu::convertFromTiled(
      untiledImage.data(),
      retile.pitch,
      dataBytesPtr,
      retile.tileMode,
      retile.swizzle,
      retile.pitch,
      retile.width,
      retile.height,
      retile.depth,
      retile.aa,
      retile.isDepth,
      retile.bpp,
      startSlice,
      endSlice);
   */
}

void
Driver::_downloadMemCache(MemCacheObject *cache, SectionRange range)
{
   // If we have a pending delayed write that overlaps, we are going to need to
   // process it here before we can actually write to the CPU.
   if (cache->delayedWriteFunc && cache->delayedWriteRange.intersects(range)) {
      cache->delayedWriteFunc();
      cache->delayedWriteFunc = nullptr;
   }

   if (cache->mutator.mode == MemCacheMutator::Mode::None) {
      _downloadMemCacheRaw(cache, range);
   } else if (cache->mutator.mode == MemCacheMutator::Mode::Retile) {
      _downloadMemCacheRetile(cache, range);
   } else {
      decaf_abort("Unsupported memory cache mutator mode");
   }
}

void
Driver::_refreshMemCache_Check(MemCacheObject *cache, SectionRange range)
{
   forEachSectionSegment(cache, range, [&](MemCacheSection& section, MemCacheSegment* segment){
      // Refresh the segment to make sure we have up-to-date information
      _refreshMemSegment(segment);

      // Update the wanted index
      if (segment->lastChangeIndex > section.wantedChangeIndex) {
         section.wantedChangeIndex = segment->lastChangeIndex;
      }

      // Check if we already have this data, if we do, there is nothing to do.
      if (section.lastChangeIndex >= segment->lastChangeIndex) {
         return;
      }

      if (!segment->lastChangeOwner) {
         section.needsUpload = true;
      }
   });
}

void
Driver::_refreshMemCache_Update(MemCacheObject *cache, SectionRange range)
{
   // This is an optimization to enable us to do bigger copies and uploads in
   // the case of a fragmented set of underlying segments which point to the
   // same thing contiguously.

   RangeCombiner<void*, uint32_t, uint32_t> uploadCombiner(
   [&](void*, uint32_t start, uint32_t count){
      _uploadMemCache(cache, { start, count });
   });

   for (auto i = range.start; i < range.start + range.count; ++i) {
      auto& section = cache->sections[i];

      if (section.needsUpload) {
         uploadCombiner.push(nullptr, i, 1);
      }
   }

   uploadCombiner.flush();

   RangeCombiner<MemCacheObject*, phys_addr, uint32_t> copyCombiner(
   [&](MemCacheObject *object, phys_addr address, uint32_t size){
      vk::BufferCopy copyDesc;
      copyDesc.srcOffset = static_cast<uint32_t>(address - object->address);
      copyDesc.dstOffset = static_cast<uint32_t>(address - cache->address);
      copyDesc.size = size;
      mActiveCommandBuffer.copyBuffer(object->buffer, cache->buffer, { copyDesc });
   });

   // We have to do this independantly, as our section updates need to
   // happen only after all segments have been written...
   for (auto i = range.start; i < range.start + range.count; ++i) {
      auto& section = cache->sections[i];

      forEachMemSegment(section.firstSegment, section.size, [&](MemCacheSegment* segment){
         // Check to see if we already have the latest data from this segment available
         // to us in our buffer already (to avoid needing to copy).
         if (section.lastChangeIndex >= segment->lastChangeIndex) {
            return;
         }

         // Lets make sure if there was no owner, that we uploaded this previously,
         // and that we take ownership and update the last change index.
         if (!segment->lastChangeOwner) {
            decaf_check(section.needsUpload);
            segment->lastChangeOwner = cache;
            return;
         }

         // TODO: We should probably check if the delayed write actually
         // overlaps with the data that we want to have access to.  If it
         // does not, we have no need to force it to be flushed.
         auto& lastChangeOwner = segment->lastChangeOwner;
         if (lastChangeOwner->delayedWriteFunc) {
            lastChangeOwner->delayedWriteFunc();
            lastChangeOwner->delayedWriteFunc = nullptr;
            lastChangeOwner->delayedWriteRange = { 0, 0 };
         }

         // Push this copy to our list of copies we want to do.
         copyCombiner.push(segment->lastChangeOwner, segment->address, segment->size);

         // If the segment was not GPU written, lets also take ownership since it will
         // increase the chances of condensing buffer copies for future transfers.
         if (!segment->gpuWritten) {
            segment->lastChangeOwner = cache;
         }
      });

      // Mark the section as having been updated.
      section.lastChangeIndex = section.wantedChangeIndex;

      // Mark it as no longer needing to be uploaded
      section.needsUpload = false;
   }

   copyCombiner.flush();
}

void
Driver::_refreshMemCache(MemCacheObject *cache, SectionRange range)
{
   _refreshMemCache_Check(cache, range);
   _refreshMemCache_Update(cache, range);
}

void
Driver::_invalidateMemCache(MemCacheObject *cache, SectionRange range, DelayedMemWriteFunc delayedWriteFunc)
{
   auto changeIndex = ++mMemChangeCounter;

   // If there is already a delayed write and its not covered by
   // this particular invalidation, we will have to execute it.
   if (cache->delayedWriteFunc) {
      if (!range.covers(cache->delayedWriteRange)) {
         cache->delayedWriteFunc();
      }

      cache->delayedWriteFunc = nullptr;
      cache->delayedWriteRange = {};
   }

   if (delayedWriteFunc) {
      cache->delayedWriteFunc = delayedWriteFunc;
      cache->delayedWriteRange = range;
   }

   for (auto i = range.start; i < range.start + range.count; ++i) {
      cache->sections[i].lastChangeIndex = changeIndex;
   }

   auto& startSection = cache->sections[range.start];
   auto& lastSection = cache->sections[range.start + range.count - 1];
   auto firstSegment = startSection.firstSegment;
   auto totalSegSize = lastSection.offset + lastSection.size - startSection.offset;

   forEachMemSegment(cache, range, [&](MemCacheSegment* segment){
      segment->lastChangeIndex = changeIndex;
      segment->lastChangeOwner = cache;
      segment->gpuWritten = true;
   });

   mDirtyMemCaches.push_back({ changeIndex, cache, range });
}

void
Driver::_barrierMemCache(MemCacheObject *cache, ResourceUsage usage, SectionRange range)
{
   auto& firstSection = cache->sections[range.start];
   auto& lastSection = cache->sections[range.start + range.count - 1];
   auto offsetStart = firstSection.offset;
   auto offsetEnd = lastSection.offset + lastSection.size;
   auto memRange = offsetEnd - offsetStart;

   vk::BufferMemoryBarrier bufferBarrier;
   bufferBarrier.srcAccessMask = vk::AccessFlags();
   bufferBarrier.dstAccessMask = vk::AccessFlags();
   bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   bufferBarrier.buffer = cache->buffer;
   bufferBarrier.offset = offsetStart;
   bufferBarrier.size = memRange;

   mActiveCommandBuffer.pipelineBarrier(
      vk::PipelineStageFlagBits::eAllGraphics,
      vk::PipelineStageFlagBits::eAllGraphics,
      vk::DependencyFlags(),
      {},
      { bufferBarrier },
      {});
}

SectionRange
Driver::_sectionsFromOffsets(MemCacheObject *cache, uint32_t begin, uint32_t end)
{
   // Calculate which sections actually apply to this...
   uint32_t beginSection = 0;
   uint32_t endSection = 0;
   for (auto i = 0; i < cache->sections.size(); ++i) {
      auto& section = cache->sections[i];

      if (section.offset == begin) {
         beginSection = i;
      }
      if (section.offset + section.size == end) {
         endSection = i + 1;
      }
   }
   decaf_check(endSection != 0);
   decaf_check(beginSection != endSection);

   SectionRange range;
   range.start = beginSection;
   range.count = endSection - beginSection;

   return range;
}

MemCacheObject *
Driver::getMemCache(phys_addr address, uint32_t size, const std::vector<uint32_t>& sectionSizes, const MemCacheMutator& mutator)
{
   struct MemCacheDesc
   {
      uint32_t address = 0;
      uint32_t size = 0;
      std::array<uint32_t, 64> sectionSizes = { 0 };
      MemCacheMutator mutator;
   } _dataHash;
   memset(&_dataHash, 0xFF, sizeof(_dataHash));

   _dataHash.address = address.getAddress();
   _dataHash.size = size;
   decaf_check(sectionSizes.size() < _dataHash.sectionSizes.size());
   for (auto i = 0u; i < sectionSizes.size(); ++i) {
      _dataHash.sectionSizes[i] = sectionSizes[i];
   }
   _dataHash.mutator = mutator;

   auto cacheKey = DataHash {}.write(_dataHash);

   auto& cache = mMemCaches[cacheKey];
   if (!cache) {
      // If there is not yet a cache object, we need to create it.
      cache = _allocMemCache(address, sectionSizes, mutator);
   }

   decaf_check(cache->address == address);
   decaf_check(cache->size == size);
   for (auto i = 0u; i < sectionSizes.size(); ++i) {
      cache->sections[i].size = sectionSizes[i];
   }
   decaf_check(cache->mutator == mutator);

   return cache;
}

void
Driver::invalidateMemCacheDelayed(MemCacheObject *cache, uint32_t offset, uint32_t size, DelayedMemWriteFunc delayedWriteHandler)
{
   // Calculate which sections actually apply to this...
   auto range = _sectionsFromOffsets(cache, offset, offset + size);

   // Perform the invalidation
   _invalidateMemCache(cache, range, delayedWriteHandler);
}

void
Driver::transitionMemCache(MemCacheObject *cache, ResourceUsage usage, uint32_t offset, uint32_t size)
{
   // If no size was specified, it means the whole buffer
   if (size == 0) {
      size = cache->size - offset;
   }

   // Calculate which sections actually apply to this...
   auto range = _sectionsFromOffsets(cache, offset, offset + size);

   // Check if this is for reading or writing
   bool forWrite;
   switch (usage) {
   case ResourceUsage::UniformBuffer:
   case ResourceUsage::AttributeBuffer:
   case ResourceUsage::StreamOutCounterRead:
   case ResourceUsage::TransferSrc:
      forWrite = false;
      break;
   case ResourceUsage::StreamOutBuffer:
   case ResourceUsage::StreamOutCounterWrite:
   case ResourceUsage::TransferDst:
      forWrite = true;
      break;
   default:
      decaf_abort("Unexpected surface resource usage");
   }

   // Update the last usage here
   cache->lastUsageIndex = mActiveBatchIndex;

   // If this is a write-usage, we need to register this object to be
   // invalidated later when the batch is completed.  Otherwise we
   // need to read the data for usage.  Note that its safe to do the
   // invalidation before the actual write occurs since no transfers
   // occur until the end of the batch (or when it changes again).
   if (!forWrite) {
      _refreshMemCache(cache, range);
   } else {
      _invalidateMemCache(cache, range, nullptr);
   }

   _barrierMemCache(cache, usage, range);
}

void
Driver::downloadPendingMemCache()
{
   for (auto& dirtyRecord : mDirtyMemCaches) {
      auto& cache = dirtyRecord.cache;

      // If none of the sections we are targetting still have our change
      // index, there is no need to do any of the work.
      bool needsDownload = false;
      for (auto i = 0u; i < dirtyRecord.sections.count; ++i) {
         auto sectionIndex = dirtyRecord.sections.start + i;
         if (cache->sections[sectionIndex].lastChangeIndex <= dirtyRecord.changeIndex) {
            needsDownload = true;
            break;
         }
      }

      // Download the memory cache back to the CPU.  Note that we have to
      // keep this object marked as being written by the GPU until its
      // actually downloaded and we can rehash it.
      if (needsDownload) {
         _downloadMemCache(cache, dirtyRecord.sections);
      }
   }

   mDirtyMemCaches.clear();
}

DataBufferObject *
Driver::getDataMemCache(phys_addr baseAddress, uint32_t size)
{
   auto cache = getMemCache(baseAddress, size, { size });
   return static_cast<DataBufferObject *>(cache);
}

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
