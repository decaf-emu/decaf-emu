#ifdef DECAF_VULKAN
#include "vulkan_driver.h"

#include "gpu_tiling.h"
#include <common/rangecombiner.h>

namespace vulkan
{

// void(MemSegment&)
template<typename FunctorType>
static inline void
forEachMemSegment(MemSegmentRef begin, uint32_t size, FunctorType functor)
{
   auto segment = begin.get();
   for (auto sizeLeft = size; sizeLeft > 0;) {
      functor(*segment);
      sizeLeft -= segment->size;
      segment = segment->nextSegment;
   }
}

// void(MemCacheSection&, MemSegment&)
template<typename FunctorType>
static inline void
forEachSectionSegment(MemCacheObject *cache, SectionRange range, FunctorType functor)
{
   auto rangeStart = range.start;
   auto rangeEnd = range.start + range.count;

   auto firstSection = cache->sections[rangeStart];
   auto segment = firstSection.firstSegment.get();
   for (auto i = rangeStart; i < rangeEnd; ++i) {
      auto& section = cache->sections[i];

      for (auto sizeLeft = cache->sectionSize; sizeLeft > 0;) {
         functor(section, *segment);

         sizeLeft -= segment->size;
         segment = segment->nextSegment;
      }
   }
}

// void(MemSegment&)
template<typename FunctorType>
static inline void
forEachMemSegment(MemCacheObject *cache, SectionRange range, FunctorType functor)
{
   auto& firstSection = cache->sections[range.start];
   auto begin = firstSection.firstSegment;
   auto size = range.count * cache->sectionSize;
   forEachMemSegment(begin, size, functor);
}

MemCacheObject *
Driver::_allocMemCache(phys_addr address, uint32_t numSections, uint32_t sectionSize)
{
   uint32_t totalSize = 0;

   std::vector<MemCacheSection> sections;
   for (auto i = 0u; i < numSections; ++i) {
      auto firstSegment = mMemTracker.get(address + totalSize, sectionSize);

      MemCacheSection section;
      section.lastChangeIndex = 0;
      section.firstSegment = firstSegment;
      section.needsUpload = false;
      section.wantedChangeIndex = 0;
      sections.push_back(section);

      totalSize += sectionSize;
   }

   vk::BufferCreateInfo bufferDesc;
   bufferDesc.size = totalSize;
   bufferDesc.usage =
      vk::BufferUsageFlagBits::eVertexBuffer |
      vk::BufferUsageFlagBits::eUniformBuffer |
      vk::BufferUsageFlagBits::eTransformFeedbackBufferEXT |
      vk::BufferUsageFlagBits::eStorageBuffer |
      vk::BufferUsageFlagBits::eTransferDst |
      vk::BufferUsageFlagBits::eTransferSrc;
   bufferDesc.sharingMode = vk::SharingMode::eExclusive;
   bufferDesc.queueFamilyIndexCount = 0;
   bufferDesc.pQueueFamilyIndices = nullptr;

   VmaAllocationCreateInfo allocInfo = {};
   allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

   VkBuffer buffer;
   VmaAllocation allocation;
   CHECK_VK_RESULT(
      vmaCreateBuffer(mAllocator,
                      reinterpret_cast<VkBufferCreateInfo*>(&bufferDesc),
                      &allocInfo,
                      &buffer,
                      &allocation,
                      nullptr));

   static uint64_t memCacheIndex = 0;
   setVkObjectName(buffer, fmt::format("mcch_{}_{:08x}_{}", memCacheIndex++, address.getAddress(), totalSize).c_str());

   auto cache = new MemCacheObject();
   cache->address = address;
   cache->size = totalSize;
   cache->numSections = numSections;
   cache->sectionSize = sectionSize;
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
Driver::_uploadMemCache(MemCacheObject *cache, SectionRange range)
{
   uint8_t *cacheBasePtr = phys_cast<uint8_t*>(cache->address).getRawPointer();

   auto offsetStart = cache->sectionSize * range.start;
   auto rangeSize = cache->sectionSize * range.count;

   uint8_t *uploadData = cacheBasePtr + offsetStart;

   // Upload the data to the CPU
   auto stagingBuffer = getStagingBuffer(rangeSize, StagingBufferType::CpuToGpu);
   copyToStagingBuffer(stagingBuffer, 0, uploadData, rangeSize);

   // Transition the buffers appropriately
   transitionStagingBuffer(stagingBuffer, ResourceUsage::TransferSrc);
   _barrierMemCache(cache, ResourceUsage::TransferDst, range);

   // Copy the data out of the staging buffer into the memory cache.
   vk::BufferCopy copyDesc;
   copyDesc.srcOffset = 0;
   copyDesc.dstOffset = offsetStart;
   copyDesc.size = rangeSize;
   mActiveCommandBuffer.copyBuffer(stagingBuffer->buffer, cache->buffer, { copyDesc });
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

   // Lets start doing the actual download!
   auto offsetStart = cache->sectionSize * range.start;
   auto rangeSize = cache->sectionSize * range.count;

   // Create a staging buffer to use for the readback
   auto stagingBuffer = getStagingBuffer(rangeSize, StagingBufferType::GpuToCpu);

   // Transition the buffers appropriately
   transitionStagingBuffer(stagingBuffer, ResourceUsage::TransferDst);
   _barrierMemCache(cache, ResourceUsage::TransferSrc, range);

   // Copy the data into our staging buffer from the cache object
   vk::BufferCopy copyDesc;
   copyDesc.srcOffset = offsetStart;
   copyDesc.dstOffset = 0;
   copyDesc.size = rangeSize;
   mActiveCommandBuffer.copyBuffer(cache->buffer, stagingBuffer->buffer, { copyDesc });

   // We have to pre-transition the buffer to being host-read, as it would be otherwise
   // illegal to be doing the transition during the retire function below.
   transitionStagingBuffer(stagingBuffer, ResourceUsage::HostRead);

   // TODO: This can be optimized... A lot...

   // Move the data onto the CPU on a per-section basis
   for (auto i = range.start; i < range.start + range.count; ++i) {
      auto& section = cache->sections[i];
      auto changeIndex = section.lastChangeIndex;

      addRetireTask([=](){
         void *data = phys_cast<void*>(cache->address + i * cache->sectionSize).getRawPointer();

         auto stagingOffset = (i - range.start) * cache->sectionSize;

         // Copy the data out of the staging area into memory
         copyFromStagingBuffer(stagingBuffer, stagingOffset, data, cache->sectionSize);

         // We need to calculate new data hashes for the relevant segments that
         // are affected by this image and are not still being GPU written.
         forEachMemSegment(section.firstSegment, cache->sectionSize, [&](MemSegment& segment){
            // For safety purposes, lets confirm that this write was intended.
            decaf_check(segment.lastChangeIndex >= section.lastChangeIndex);

            // Only bother recalculating the hashing if we are not already waiting
            // for more writes to this segment...
            if (segment.lastChangeIndex == changeIndex) {
               mMemTracker.markSegmentGpuDone(&segment);
            }
         });
      });
   }
}

void
Driver::_refreshMemCache_Check(MemCacheObject *cache, SectionRange range)
{
   forEachSectionSegment(cache, range, [&](MemCacheSection& section, MemSegment& segment){
      // Refresh the segment to make sure we have up-to-date information
      mMemTracker.refreshSegment(&segment);

      // Update the wanted index
      if (segment.lastChangeIndex > section.wantedChangeIndex) {
         section.wantedChangeIndex = segment.lastChangeIndex;
      }

      // Check if we already have this data, if we do, there is nothing to do.
      if (section.lastChangeIndex >= segment.lastChangeIndex) {
         return;
      }

      // Check if we have no last-owner first, obviously an upload is needed in that
      // case.  Additionally, we are required to perform an upload if the last owner
      // has rearranged the data such that it might not make sense for us anymore.
      if (!segment.lastChangeOwner) {
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

   auto uploadCombiner = makeRangeCombiner<void*, uint32_t, uint32_t>(
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

   auto copyCombiner = makeRangeCombiner<MemCacheObject*, phys_addr, uint32_t>(
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

      forEachMemSegment(section.firstSegment, cache->sectionSize, [&](MemSegment& segment){
         // Note that we have to do this delayed write check before we exit
         // early as we are not actually 'up to date' until the write occurs.
         auto& lastChangeOwner = segment.lastChangeOwner;
         if (lastChangeOwner && lastChangeOwner->delayedWriteFunc) {
            if (lastChangeOwner->delayedWriteRange.intersects(range)) {
               lastChangeOwner->delayedWriteFunc();
               lastChangeOwner->delayedWriteFunc = nullptr;
               lastChangeOwner->delayedWriteRange = { 0, 0 };
            }
         }

         // Check to see if we already have the latest data from this segment available
         // to us in our buffer already (to avoid needing to copy).
         if (section.lastChangeIndex >= segment.lastChangeIndex) {
            return;
         }

         // Lets make sure if there was no owner, that we uploaded this previously,
         // and that we take ownership and update the last change index.
         if (!segment.lastChangeOwner) {
            decaf_check(section.needsUpload);
            segment.lastChangeOwner = cache;
            return;
         }

         // Push this copy to our list of copies we want to do.
         copyCombiner.push(segment.lastChangeOwner, segment.address, segment.size);

         // If the segment was not GPU written, lets also take ownership since it will
         // increase the chances of condensing buffer copies for future transfers.
         if (!segment.gpuWritten) {
            segment.lastChangeOwner = cache;
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
Driver::_invalidateMemCache(MemCacheObject *cache, SectionRange range, const DelayedMemWriteFunc& delayedWriteFunc)
{
   auto changeIndex = mMemTracker.newChangeIndex();

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

   forEachMemSegment(cache, range, [&](MemSegment& segment){
      segment.lastChangeIndex = changeIndex;
      segment.lastChangeOwner = cache;
      segment.gpuWritten = true;
   });

   if (delayedWriteFunc) {
      // If there is a delayed write, we assume that this must have been a surface transition
      // that was happening, and we don't want to copy these all back, so lets not mark it
      // for download later...
      return;
   }

   mDirtyMemCaches.push_back({ changeIndex, cache, range });
}

void
Driver::_barrierMemCache(MemCacheObject *cache, ResourceUsage usage, SectionRange range)
{
   auto offsetStart = range.start * cache->sectionSize;;
   auto memSize = range.count * cache->sectionSize;

   if (cache->activeUsage == usage) {
      return;
   }

   auto srcMeta = getResourceUsageMeta(cache->activeUsage);
   auto dstMeta = getResourceUsageMeta(usage);

   vk::BufferMemoryBarrier bufferBarrier;
   bufferBarrier.srcAccessMask = srcMeta.accessFlags;
   bufferBarrier.dstAccessMask = dstMeta.accessFlags;
   bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   bufferBarrier.buffer = cache->buffer;
   bufferBarrier.offset = offsetStart;
   bufferBarrier.size = memSize;

   mActiveCommandBuffer.pipelineBarrier(
      srcMeta.stageFlags,
      dstMeta.stageFlags,
      vk::DependencyFlags(),
      {},
      { bufferBarrier },
      {});

   cache->activeUsage = usage;
}

SectionRange
Driver::_sectionsFromOffsets(MemCacheObject *cache, uint32_t begin, uint32_t end)
{
   decaf_check(begin % cache->sectionSize == 0);
   decaf_check(end % cache->sectionSize == 0);

   SectionRange range;
   range.start = begin / cache->sectionSize;
   range.count = (end - begin) / cache->sectionSize;

   return range;
}

MemCacheObject *
Driver::getMemCache(phys_addr address, uint32_t numSections, uint32_t sectionSize)
{
   // Note: We cast here first to make sure the types are in the appropriate
   // types, otherwise the bit operations may not behave as expected.
   uint64_t lookupAddr = address.getAddress();
   uint64_t lookupSize = numSections * sectionSize;
   uint64_t lookupKey = (lookupSize << 32) | lookupAddr;

   // TODO: We should implement the ability to 'grow' a MemCacheObject to
   // contain more sections on top of the ones that already exist in the
   // object.  This will improve memory usage, and help when new surfaces
   // appear which just add slices.

   auto& cacheRef = mMemCaches[lookupKey];

   auto cache = cacheRef;
   while (cache) {
      if (cache->sectionSize == sectionSize) {
         break;
      }
      cache = cache->nextObject;
   }

   if (!cache) {
      // If there is not yet a cache object, we need to create it.
      cache = _allocMemCache(address, numSections, sectionSize);

      // Be warned about the fact that `cache` is a reference object,
      // and we are putting the new object at the head of the list.
      cache->nextObject = cacheRef;
      cacheRef = cache;
   }

   decaf_check(cache->address == address);
   decaf_check(cache->numSections == numSections);
   decaf_check(cache->sectionSize == sectionSize);

   return cache;
}

void
Driver::invalidateMemCacheDelayed(MemCacheObject *cache, uint32_t offset, uint32_t size, const DelayedMemWriteFunc& delayedWriteHandler)
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
   auto forWrite = getResourceUsageMeta(usage).isWrite;

   // Update the last usage here
   cache->lastUsageIndex = mActiveBatchIndex;

   // If this is a write-usage, we need to register this object to be
   // invalidated later when the batch is completed.  Otherwise we
   // need to read the data for usage.  Note that its safe to do the
   // invalidation before the actual write occurs since no transfers
   // occur until the end of the batch (or when it changes again).
   _refreshMemCache(cache, range);

   if (forWrite) {
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
   auto cache = getMemCache(baseAddress, 1, size);
   return static_cast<DataBufferObject *>(cache);
}

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
