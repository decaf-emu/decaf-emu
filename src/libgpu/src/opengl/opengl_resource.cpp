#ifdef DECAF_GL
#include "opengl_resource.h"
#include <common/decaf_assert.h>

namespace opengl
{

ResourceMemoryMap::ResourceMemoryMap()
   : mCounter(0)
{
}

void
ResourceMemoryMap::addResource(Resource *resource)
{
   std::unique_lock<std::mutex> lock(mMutex);

   if (mKnownResources.find(resource) != mKnownResources.end()) {
      return;
   }

   // mCounter serves to differentiate multiple resources with the same
   //  start or end address.
   auto counter = mCounter++;
   auto keyStart = static_cast<uint64_t>(resource->cpuMemStart) << 32 | counter;
   auto keyEnd = static_cast<uint64_t>(resource->cpuMemEnd - 1) << 32 | counter;
   mKnownResources[resource] = counter;

   // We insert entries for both the start (first byte) and end (last byte)
   //  of the resource.  Then in markDirty(), we can simply set the dirty
   //  flag on every resource with at least one key (region endpoint) in
   //  the dirty region.
   decaf_check(mMemoryMap.find(keyStart) == mMemoryMap.end());
   mMemoryMap[keyStart] = resource;
   decaf_check(mMemoryMap.find(keyEnd) == mMemoryMap.end());
   mMemoryMap[keyEnd] = resource;
}

void
ResourceMemoryMap::removeResource(Resource *resource)
{
   std::unique_lock<std::mutex> lock(mMutex);

   auto knownIter = mKnownResources.find(resource);
   decaf_check(knownIter != mKnownResources.end());
   auto counter = knownIter->second;
   mKnownResources.erase(knownIter);
   auto keyStart = static_cast<uint64_t>(resource->cpuMemStart) << 32 | counter;
   auto keyEnd = static_cast<uint64_t>(resource->cpuMemEnd - 1) << 32 | counter;

   decaf_check(mMemoryMap.find(keyStart) != mMemoryMap.end());
   mMemoryMap.erase(keyStart);
   decaf_check(mMemoryMap.find(keyEnd) != mMemoryMap.end());
   mMemoryMap.erase(keyEnd);
}

} // namespace opengl

#endif // ifdef DECAF_GL
