#pragma once
#include <atomic>
#include <common/align.h>
#include <common/decaf_assert.h>
#include <common/platform_memory.h>
#include <mutex>

namespace cpu
{
namespace jit
{

class VMemRuntime
{
public:
   VMemRuntime(size_t initialSize, size_t sizeLimit)
   {
      // Find a good base address
      mRootAddress = 0;
      for (auto n = 2; n < 32; ++n) {
         auto base = 0x100000000 * n;

         if (platform::reserveMemory(base, sizeLimit)) {
            mRootAddress = base;
            break;
         }
      }

      decaf_assert(mRootAddress, "Failed to map memory for JIT");

      if (!platform::commitMemory(mRootAddress, initialSize, platform::ProtectFlags::ReadWriteExecute)) {
         decaf_abort("Failed to commit memory for JIT");
      }

      mSizeLimit = sizeLimit;
      mCommittedSize = initialSize;
      mIncreaseSize = initialSize;
      mCurAddress = mRootAddress;
   }

   ~VMemRuntime()
   {
      platform::freeMemory(mRootAddress, mSizeLimit);
   }

   uintptr_t getRootAddress() const
   {
      return mRootAddress;
   }

   void * allocate(size_t size, size_t alignment) noexcept
   {
      // Calculate how much we need to allocate to guarentee we can
      //  align the pointer and still have sufficient room for our data.
      size_t alignedSize = align_up(size + (alignment - 1), alignment);

      // Try to grab some space
      uintptr_t baseCurAddress = mCurAddress.fetch_add(alignedSize);
      uintptr_t alignedAddress = align_up(baseCurAddress, alignment);
      size_t curCommited = mCommittedSize.load();

      // Check that we did not overrun the end of the commited area
      if (alignedAddress + size > curCommited) {
         // Lock the runtime while we adjusting committed region stuff
         std::unique_lock<std::mutex> lock(mMutex);

         // Loop committing new sections until we have enough commited space to cover
         //  our new allocation.  We don't bother doing commissions for other threads
         //  since they are already being forced to lock the mutex anyways.
         auto committedSize = mCommittedSize.load();
         while (mRootAddress + committedSize < alignedAddress + size) {
            if (!platform::commitMemory(mRootAddress + committedSize, mIncreaseSize, platform::ProtectFlags::ReadWriteExecute)) {
               // We failed to commit more memory for some reason
               return nullptr;
            }
            committedSize += mIncreaseSize;
         }

         mCommittedSize.store(committedSize);
      }

      return reinterpret_cast<void*>(alignedAddress);
   }

   bool add(void** dst, size_t codeSize, size_t alignment) noexcept
   {
      if (codeSize == 0) {
         *dst = nullptr;
         return false;
      }

      // Lets allocate some memory for the JIT block, allocate only
      //  fails if we have run out of memory, so make sure to indicate
      //  when that happens.
      auto allocPtr = allocate(codeSize, alignment);
      if (!allocPtr) {
         *dst = nullptr;
         return false;
      }

      *dst = allocPtr;

      return true;
   }

   void release(void* p) noexcept
   {
      // We do not release memory
   }

   std::mutex mMutex;
   uintptr_t mRootAddress;
   size_t mIncreaseSize;
   std::atomic<uintptr_t> mCurAddress;
   std::atomic<size_t> mCommittedSize;
   size_t mSizeLimit;

};

} // namespace jit
} // namespace cpu
