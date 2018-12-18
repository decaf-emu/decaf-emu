#pragma once
#ifdef DECAF_VULKAN

#include <cstdint>
#include <libcpu/be2_struct.h>
#include <libcpu/memtrack.h>
#include <map>
#include <forward_list>

namespace vulkan
{

/*
This memory tracker keeps track of a range of phys_addr memory for changes.
It attempts to make iteration as fast as possible by trying to keep the
segments linearly arranged in memory, while still allowing fast insertions.
It does this by using a dynamic list to hold the segments initially, but
then quickly optimizes them into a linear vector.

Important Semantics:
 - SegmentRef's are stable
 - Pointers to segments ARE NOT stable.
 - Don't forget to call optimize periodically.
*/

template<typename _DataOwnerType>
class MemoryTracker
{
public:
   struct Segment {
      // We keep a linked list of segments connected to eachother for
      // faster iteration through all the segments.  It is intentionally
      // at the top to increase the chance of being on the cacheline of
      // the previous segment.
      Segment *nextSegment = nullptr;

      // Meta-data about what this segment represents
      phys_addr address = {};
      uint32_t size = 0;

      // Stores the last memory tracking state for this segment.
      cpu::MemtrackState dataState;

      // Tracks the last CPU check of this segment, to avoid checking the
      // memory multiple times in a single batch.
      uint64_t lastCheckIndex = 0;

      // Records if there is a pending GPU write for this data.  This is to ensure
      // that we do not overwrite a pending GPU write with random CPU data.
      bool gpuWritten = false;

      // The last change for this segment
      uint64_t lastChangeIndex = 0;

      // Represents the object owning the most up to date version of the data.
      _DataOwnerType lastChangeOwner = {};
   };

protected:
   typedef std::map<phys_addr, Segment*> SegmentMap;
   typedef typename SegmentMap::iterator SegmentMapIter;

public:
   class SegmentRef
   {
      friend MemoryTracker;

   public:
      SegmentRef()
      {
      }

      Segment * get() const
      {
         return mIterator->second;
      }

      Segment * operator->() const
      {
         return get();
      }

   protected:
      SegmentRef(SegmentMapIter iterator)
         : mIterator(iterator)
      {
      }

      SegmentMapIter mIterator;

   };

   void nextBatch()
   {
      mCurrentBatchIndex++;
   }

   uint64_t newChangeIndex()
   {
      return ++mChangeCounter;
   }

   SegmentRef get(phys_addr address, uint32_t size)
   {
      auto iter = _getSegment(address, size);
      _ensureSegments(iter, size);
      return iter;
   }

   void markSegmentGpuDone(Segment *segment)
   {
      segment->dataState = cpu::getMemoryState(segment->address, segment->size);
      segment->gpuWritten = false;
   }

   void refreshSegment(Segment *segment)
   {
      _refreshSegment(segment);
   }

   void optimize()
   {
      // TODO: Maybe avoid optimizing for low dynamic segment counts.

      if (mLookupMap.empty() || mDynamicSegments.empty()) {
         // If the map is empty, or there are no dynamic segments,
         // there is no need to optimize the map.
         return;
      }

      // Reserve the new linear segments
      std::vector<Segment> newLinearSegments;
      newLinearSegments.reserve(mLookupMap.size());

      // Move all our segments
      auto iter = mLookupMap.begin();
      newLinearSegments.push_back(std::move(*iter->second));
      auto lastSegment = &newLinearSegments.back();
      iter->second = lastSegment;
      ++iter;

      for ( ; iter != mLookupMap.end(); ++iter) {
         newLinearSegments.push_back(std::move(*iter->second));
         auto newSegment = &newLinearSegments.back();

         iter->second = newSegment;

         lastSegment->nextSegment = newSegment;

         lastSegment = newSegment;
      }
      lastSegment->nextSegment = nullptr;

      // Move the new linear map into place and clear the dynamic segments
      // that are no longer being used.
      mLinearSegments = std::move(newLinearSegments);
      mDynamicSegments.clear();
   }

protected:
   SegmentMapIter
   _insertSegment(SegmentMapIter position, phys_addr address, uint32_t size)
   {
      mDynamicSegments.push_front({});
      Segment &segment = mDynamicSegments.front();
      segment.address = address;
      segment.size = size;

      auto newIter = mLookupMap.insert(position, { segment.address, &segment });

      auto prevIter = newIter;
      prevIter--;
      if (prevIter != mLookupMap.end()) {
         prevIter->second->nextSegment = &segment;
      }

      auto nextIter = newIter;
      nextIter++;
      if (nextIter != mLookupMap.end()) {
         segment.nextSegment = nextIter->second;
      } else {
         segment.nextSegment = nullptr;
      }

      return newIter;
   }

   SegmentMapIter
   _splitSegment(SegmentMapIter iter, uint32_t newSize)
   {
      auto& oldSegment = iter->second;
      decaf_check(oldSegment->size > newSize);

      // Save the old info so we can do the final hash check
      auto oldSize = oldSegment->size;
      auto oldState = oldSegment->dataState;

      // Create the new segment, save it to the map and resize the
      // old segment to not overlap the new one.
      auto newIter = _insertSegment(iter, oldSegment->address + newSize, oldSegment->size - newSize);
      auto &newSegment = newIter->second;
      oldSegment->size = newSize;

      // Copy over some state from the old Segment
      newSegment->lastCheckIndex = oldSegment->lastCheckIndex;
      newSegment->gpuWritten = oldSegment->gpuWritten;
      newSegment->lastChangeIndex = oldSegment->lastChangeIndex;
      newSegment->lastChangeOwner = oldSegment->lastChangeOwner;

      // If the old segment was written by the GPU, there is no need to
      // do any of the hashing work, it will be done during readback.
      if (oldSegment->gpuWritten) {
         newSegment->dataState = {};
         return newIter;
      }

      // Lets calculate the new hashes for the segments after they have been
      // split to ensure we don't do unneeded uploading after a split.  We check
      // that the new hashes reflect the same data that previous existed in the
      // segment following this.
      oldSegment->dataState = cpu::getMemoryState(oldSegment->address, oldSegment->size);
      newSegment->dataState = cpu::getMemoryState(newSegment->address, newSegment->size);

      // If the segment was last checked during this batch, there is no need to do
      // any additional work to figure out if the data changed.
      if (oldSegment->lastCheckIndex >= mCurrentBatchIndex) {
         return newIter;
      }

      // Now check that the data hasn't changed since we did the last hashing.
      auto newFullState = cpu::getMemoryState(oldSegment->address, oldSize);
      if (newFullState != oldState) {
         auto changeIndex = newChangeIndex();

         oldSegment->lastCheckIndex = mCurrentBatchIndex;
         oldSegment->lastChangeIndex = changeIndex;
         oldSegment->lastChangeOwner = nullptr;

         newSegment->lastCheckIndex = mCurrentBatchIndex;
         newSegment->lastChangeIndex = changeIndex;
         newSegment->lastChangeOwner = nullptr;
      }

      return newIter;
   }

   SegmentMapIter
   _getSegment(phys_addr address, uint32_t maxSize)
   {
      auto iter = mLookupMap.lower_bound(address);

      if (iter != mLookupMap.end()) {
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
      if (iter != mLookupMap.begin()) {
         --iter;

         auto& foundSegment = iter->second;

         // If the previous segment covers this new segments range, we
         // need to split it and return that.
         if (address < foundSegment->address + foundSegment->size) {
            auto newSize = static_cast<uint32_t>(address - foundSegment->address);
            return _splitSegment(iter, newSize);
         }
      }

      // maxSize being 0 indicates that we need to ensure there is a split
      // point in the map, but we don't need to generate anything for it.
      if (maxSize == 0) {
         return mLookupMap.end();
      }

      // Allocate a new segment for this at the end
      return _insertSegment(mLookupMap.end(), address, maxSize);
   }

   void
   _ensureSegments(SegmentMapIter firstSegment, uint32_t size)
   {
      auto curAddress = firstSegment->second->address;
      auto sizeLeft = size;

      // We ensure there is a split point at the end for us to hit.
      // TODO: Do this below as it will be slightly faster
      _getSegment(curAddress + size, 0);

      auto iter = firstSegment;
      while (sizeLeft > 0) {
         if (iter == mLookupMap.end()) {
            _insertSegment(iter, curAddress, sizeLeft);
            break;
         }

         if (iter->second->address != curAddress) {
            auto gapSize = static_cast<uint32_t>(iter->second->address - curAddress);
            auto newSize = std::min(gapSize, sizeLeft);

            iter = _insertSegment(iter, curAddress, newSize);
         }

         auto& segment = iter->second;

         decaf_check(segment->address == curAddress);
         decaf_check(segment->size <= sizeLeft);

         curAddress += segment->size;
         sizeLeft -= segment->size;

         iter++;
      }
   }

   void
   _refreshSegment(Segment *segment)
   {
      // If this segment was last written by the GPU, then we are guarenteed
      // to already have the most up to date data in a GPU buffer somewhere.
      if (segment->gpuWritten) {
         return;
      }

      // If this segment was already checked during this batch, there is no
      // need to go check it again..
      if (segment->lastCheckIndex >= mCurrentBatchIndex) {
         return;
      }

      // Rehash all our data
      auto dataState = cpu::getMemoryState(segment->address, segment->size);

      // If we already have a hash, and the hash already matches, we can
      // simply mark it as checked without any more downloads.
      if (segment->lastCheckIndex > 0 && segment->dataState == dataState) {
         segment->lastCheckIndex = mCurrentBatchIndex;
         return;
      }

      // The data has changed, lets update our internal hashes and create
      // a new memory change event to represent this.
      auto changeIndex = newChangeIndex();

      segment->dataState = dataState;
      segment->lastCheckIndex = mCurrentBatchIndex;
      segment->lastChangeIndex = changeIndex;
      segment->lastChangeOwner = nullptr;
   }

   uint64_t mCurrentBatchIndex = 0;
   uint64_t mChangeCounter = 0;
   SegmentMap mLookupMap;
   std::vector<Segment> mLinearSegments;
   std::forward_list<Segment> mDynamicSegments;

};

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
