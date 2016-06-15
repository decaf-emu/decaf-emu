#include <asmjit/asmjit.h>
#include <mutex>
#include <stdexcept>
#include "common/align.h"
#include "common/platform_memory.h"

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

      if (!mRootAddress) {
         throw std::logic_error("Failed to map memory for JIT");
      }

      if (!platform::commitMemory(mRootAddress, initialSize, platform::ProtectFlags::ReadWriteExecute)) {
         throw std::logic_error("Failed to commit memory for JIT");
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
      // Lock the runtime while we adjust the memory stuff
      std::unique_lock<std::mutex> lock(mMutex);

      mCurAddress = align_up(mCurAddress, alignment);
      void *ptr = reinterpret_cast<void*>(mCurAddress);
      mCurAddress += size;
      return ptr;
   }

   ASMJIT_API virtual asmjit::Error add(void** dst, asmjit::Assembler* assembler) noexcept
   {
      size_t codeSize = assembler->getCodeSize();
      if (codeSize == 0) {
         *dst = nullptr;
         return asmjit::kErrorNoCodeGenerated;
      }

      // Lock the runtime while we adjust the memory stuff
      std::unique_lock<std::mutex> lock(mMutex);

      // Lets make sure our code-addresses are aligned to 8-bytes.
      // There is no particular reason for this except that they are
      // very slightly faster to retreive during execution and having
      // some space between blocks is helpful when debugging.
      mCurAddress = align_up(mCurAddress, 8);

      // If we don't have any room left, we need to error
      auto cursorPos = mCurAddress - mRootAddress;
      if (cursorPos + codeSize > _sizeLimit) {
         *dst = nullptr;
         return asmjit::kErrorCodeTooLarge;
      }

      // If this code block will push us past our commited region
      // we need to commit more memory to the JIT code block.
      while (cursorPos + codeSize > mCommittedSize) {
         if (!platform::commitMemory(mRootAddress + mCommittedSize, mIncreaseSize, platform::ProtectFlags::ReadWriteExecute)) {
            *dst = nullptr;
            return asmjit::kErrorCodeTooLarge;
         }
         mCommittedSize += mIncreaseSize;
      }

      // Lets relocate the code to the memory block
      auto allocPtr = reinterpret_cast<void*>(mCurAddress);
      size_t relocSize = assembler->relocCode(allocPtr);
      if (relocSize == 0) {
         return asmjit::kErrorInvalidState;
      }

      mCurAddress += relocSize;

      // We should not hold the lock through a flush as it could be expensive
      lock.unlock();

      flush(allocPtr, codeSize);
      *dst = allocPtr;

      return asmjit::kErrorOk;
   }

   ASMJIT_API virtual asmjit::Error release(void* p) noexcept
   {
      // We do not release memory
      return asmjit::kErrorOk;
   }

   std::mutex mMutex;
   size_t mIncreaseSize;
   size_t mCommittedSize;
   asmjit::Ptr mRootAddress;
   asmjit::Ptr mCurAddress;

};

} // namespace jit
} // namespace cpu
