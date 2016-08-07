#include "common/align.h"
#include "common/decaf_assert.h"
#include "common/platform_memory.h"
#include <asmjit/asmjit.h>
#include <atomic>
#include <mutex>

namespace cpu
{
namespace jit
{

class VMemRuntime : public asmjit::HostRuntime
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

      _sizeLimit = sizeLimit;
      mCommittedSize = initialSize;
      mIncreaseSize = initialSize;
      mCurAddress = mRootAddress;
   }

   ~VMemRuntime()
   {
      platform::freeMemory(mRootAddress, _sizeLimit);
   }

   asmjit::Ptr getRootAddress() const
   {
      return mRootAddress;
   }

   void * allocate(size_t size, size_t alignment = 4) noexcept
   {
      // Calculate how much we need to allocate to guarentee we can
      //  align the pointer and still have sufficient room for our data.
      size_t alignedSize = align_up(size + (alignment - 1), alignment);

      // Try to grab some space
      asmjit::Ptr baseCurAddress = mCurAddress.fetch_add(alignedSize);
      asmjit::Ptr alignedAddress = align_up(baseCurAddress, alignment);
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

   ASMJIT_API asmjit::Error add(void** dst, asmjit::Assembler* assembler) noexcept override
   {
      size_t codeSize = assembler->getCodeSize();
      if (codeSize == 0) {
         *dst = nullptr;
         return asmjit::kErrorNoCodeGenerated;
      }

      // Lets allocate some memory for the JIT block, allocate only
      //  fails if we have run out of memory, so make sure to indicate
      //  when that happens.
      auto allocPtr = allocate(codeSize, 8);
      if (!allocPtr) {
         *dst = nullptr;
         return asmjit::kErrorCodeTooLarge;
      }

      // Lets relocate the code to the memory block
      size_t relocSize = assembler->relocCode(allocPtr);
      if (relocSize == 0) {
         return asmjit::kErrorInvalidState;
      }

      flush(allocPtr, codeSize);
      *dst = allocPtr;

      return asmjit::kErrorOk;
   }

   ASMJIT_API asmjit::Error release(void* p) noexcept override
   {
      // We do not release memory
      return asmjit::kErrorOk;
   }

   std::mutex mMutex;
   asmjit::Ptr mRootAddress;
   size_t mIncreaseSize;
   std::atomic<asmjit::Ptr> mCurAddress;
   std::atomic<size_t> mCommittedSize;

};

} // namespace jit
} // namespace cpu
