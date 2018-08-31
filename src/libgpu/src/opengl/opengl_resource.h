#pragma once
#ifdef DECAF_GL
#include <libcpu/be2_struct.h>
#include <map>
#include <mutex>
#include <unordered_map>

namespace opengl
{

struct Resource
{
   //! The start of the CPU memory region this occupies
   phys_addr cpuMemStart;

   //! The end of the CPU memory region this occupies (plus 1)
   phys_addr cpuMemEnd;

   //! Hash of the memory contents, for detecting changes
   uint64_t cpuMemHash[2] = { 0, 0 };

   //! True if a DCFlush has been received for the memory region
   bool dirtyMemory = true;

   //! The type of resource (poor man's RTTI for surfaceSync())
   enum Type {
      DATA_BUFFER,
      SHADER,
      SURFACE,
   } type;

   Resource(Type type_) : type(type_) { }
};

// Manages data ranges associated with resources for efficient querying by
//  address.  addResource() and removeResource() lock the object for the
//  duration of the call.
class ResourceMemoryMap
{
public:
   class Iterator
   {
   public:
      Iterator(const ResourceMemoryMap &map, phys_addr start, size_t size)
         : mMap(map),
           mKeyStart(static_cast<uint64_t>(start) << 32),
           mKeyEnd(static_cast<uint64_t>(start + size) << 32),
           mMapIter(mMap.mMemoryMap.lower_bound(mKeyStart))
      {
      }

      Resource *next()
      {
         if (mMapIter != mMap.mMemoryMap.end() && mMapIter->first < mKeyEnd) {
            return (mMapIter++)->second;
         } else {
            return nullptr;
         }
      }

   private:
      const ResourceMemoryMap &mMap;
      uint64_t mKeyStart;
      uint64_t mKeyEnd;
      std::map<uint64_t, Resource *>::const_iterator mMapIter;
   };

   ResourceMemoryMap();

   void
   addResource(Resource *resource);

   void
   removeResource(Resource *resource);

   std::mutex &
   getMutex()
   {
      return mMutex;
   }

   Iterator
   getIterator(phys_addr start, size_t size)
   {
      return Iterator(*this, start, size);
   }

private:
   std::mutex mMutex;
   std::unordered_map<Resource *, uint32_t> mKnownResources;
   std::map<uint64_t, Resource *> mMemoryMap;
   uint32_t mCounter;
};

} // namespace opengl

#endif // ifdef DECAF_GL
