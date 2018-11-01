#ifdef DECAF_VULKAN
#include "vulkan_driver.h"
#include "gpu_tiling.h"

namespace vulkan
{

MemCacheSegment *
Driver::_allocateMemSegment(phys_addr address, uint32_t size)
{
   auto segment = new MemCacheSegment();
   segment->address = address;
   segment->size = size;
   segment->dataHash = DataHash {};
   segment->lastCheckIndex = 0;
   segment->gpuWritten = false;
   segment->lastChangeIndex = 0;
   segment->lastChangeOwner = nullptr;
   return segment;
}

MemSegmentMap::iterator
Driver::_splitMemSegment(MemSegmentMap::iterator iter, uint32_t newSize)
{
   auto& oldSegment = iter->second;
   decaf_check(oldSegment->size > newSize);

   // Save the old info so we can do the final hash check
   auto oldSize = oldSegment->size;
   auto oldHash = oldSegment->dataHash;

   // Create the new segment, save it to the map and resize the
   // old segment to not overlap the new one.
   auto newSegment = _allocateMemSegment(oldSegment->address + newSize, oldSegment->size - newSize);
   auto newIter = mMemSegmentMap.insert(iter, { newSegment->address, newSegment });
   oldSegment->size = newSize;

   // Copy over some state from the old Segment
   newSegment->lastCheckIndex = oldSegment->lastCheckIndex;
   newSegment->gpuWritten = oldSegment->gpuWritten;
   newSegment->lastChangeIndex = oldSegment->lastChangeIndex;
   newSegment->lastChangeOwner = oldSegment->lastChangeOwner;

   // If the old segment was written by the GPU, there is no need to
   // do any of the hashing work, it will be done during readback.
   if (oldSegment->gpuWritten) {
      newSegment->dataHash = DataHash {};
      return newIter;
   }

   // Lets calculate the new hashes for the segments after they have been
   // split to ensure we don't do unneeded uploading after a split.  We check
   // that the new hashes reflect the same data that previous existed in the
   // segment following this.
   auto oldSegPtr = phys_cast<void*>(oldSegment->address).getRawPointer();
   oldSegment->dataHash = DataHash {}.write(oldSegPtr, oldSegment->size);

   auto newSegPtr = phys_cast<void*>(oldSegment->address).getRawPointer();
   newSegment->dataHash = DataHash {}.write(newSegPtr, newSegment->size);

   // If the segment was last checked during this batch, there is no need to do
   // any additional work to figure out if the data changed.
   if (oldSegment->lastCheckIndex >= mActiveBatchIndex) {
      return newIter;
   }

   // Now check that the data hasn't changed since we did the last hashing.
   auto newFullCpuHash = DataHash {}.write(oldSegPtr, oldSize);
   if (newFullCpuHash != oldHash) {
      auto changeIndex = ++mMemChangeCounter;

      oldSegment->lastCheckIndex = mActiveBatchIndex;
      oldSegment->lastChangeIndex = changeIndex;
      oldSegment->lastChangeOwner = nullptr;

      newSegment->lastCheckIndex = mActiveBatchIndex;
      newSegment->lastChangeIndex = changeIndex;
      newSegment->lastChangeOwner = nullptr;
   }

   return newIter;
}

MemSegmentMap::iterator
Driver::_getMemSegment(phys_addr address, uint32_t maxSize)
{
   auto iter = mMemSegmentMap.lower_bound(address);

   if (iter != mMemSegmentMap.end()) {
      // Check if we found an exact match.  If so, return that.
      if (iter->second->address == address) {
         return iter;
      }

      // Otherwise we need to bound our maxSize not to tramble this.
      auto gapSize = static_cast<uint32_t>(iter->second->address - address);
      maxSize = std::min(maxSize, gapSize);
   }

   // Check that we are not at the beginning, if we are and it wasn't an
   // exact match, we know we need to make a new segment before this one.
   if (iter != mMemSegmentMap.begin()) {
      --iter;

      auto& foundSegment = iter->second;

      // If the previous segment covers this new segments range, we
      // need to split it and return that.
      if (address < foundSegment->address + foundSegment->size) {
         auto newSize = static_cast<uint32_t>(address - foundSegment->address);
         return _splitMemSegment(iter, newSize);
      }
   }

   // maxSize being 0 indicates that we need to ensure there is a split
   // point in the map, but we don't need to generate anything for it.
   if (maxSize == 0) {
      return mMemSegmentMap.end();
   }

   // Allocate a new segment for this
   auto newSegment = _allocateMemSegment(address, maxSize);
   return mMemSegmentMap.insert({ address, newSegment }).first;
}

void
Driver::_ensureMemSegments(MemSegmentMap::iterator firstSegment, uint32_t size)
{
   auto curAddress = firstSegment->second->address;
   auto sizeLeft = size;

   // We ensure there is a split point at the end for us to hit.
   // TODO: Do this below as it will be slightly faster
   _getMemSegment(curAddress + size, 0);

   auto iter = firstSegment;
   while (sizeLeft > 0) {
      if (iter == mMemSegmentMap.end()) {
         auto newSegment = _allocateMemSegment(curAddress, sizeLeft);
         mMemSegmentMap.insert(iter, { newSegment->address, newSegment });
         break;
      }

      auto segment = iter->second;

      if (segment->address != curAddress) {
         auto gapSize = static_cast<uint32_t>(segment->address - curAddress);
         auto newSize = std::min(gapSize, sizeLeft);

         auto newSegment = _allocateMemSegment(curAddress, newSize);
         iter = mMemSegmentMap.insert(iter, { newSegment->address, newSegment });

         // Refresh our segment reference
         segment = iter->second;
      }

      decaf_check(segment->address == curAddress);
      decaf_check(segment->size <= sizeLeft);

      curAddress += segment->size;
      sizeLeft -= segment->size;

      iter++;
   }
}

void
Driver::_refreshMemSegment(MemCacheSegment *segment)
{
   // If this segment was last written by the GPU, then we are guarenteed
   // to already have the most up to date data in a GPU buffer somewhere.
   if (segment->gpuWritten) {
      return;
   }

   // If this segment was already checked during this batch, there is no
   // need to go check it again..
   if (segment->lastCheckIndex >= mActiveBatchIndex) {
      return;
   }

   // Rehash all our data
   auto dataPtr = phys_cast<void*>(segment->address).getRawPointer();
   auto dataSize = segment->size;
   auto dataHash = DataHash {}.write(dataPtr, dataSize);

   // If we already have a hash, and the hash already matches, we can
   // simply mark it as checked without any more downloads.
   if (segment->lastCheckIndex > 0 && segment->dataHash == dataHash) {
      segment->lastCheckIndex = mActiveBatchIndex;
      return;
   }

   // The data has changed, lets update our internal hashes and create
   // a new memory change event to represent this.
   auto changeIndex = ++mMemChangeCounter;

   segment->dataHash = dataHash;
   segment->lastCheckIndex = mActiveBatchIndex;
   segment->lastChangeIndex = changeIndex;
   segment->lastChangeOwner = nullptr;
}

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
