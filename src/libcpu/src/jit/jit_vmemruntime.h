#include <asmjit/asmjit.h>
#include <stdexcept>
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
      _rootAddress = 0;
      for (auto n = 2; n < 32; ++n) {
         auto base = 0x100000000 * n;

         if (platform::reserveMemory(base, sizeLimit)) {
            _rootAddress = base;
            break;
         }
      }

      if (!_rootAddress) {
         throw std::logic_error("Failed to map memory for JIT");
      }

      if (!platform::commitMemory(_rootAddress, initialSize, platform::ProtectFlags::ReadWriteExecute)) {
         throw std::logic_error("Failed to commit memory for JIT");
      }

      _sizeLimit = sizeLimit;
      _committedSize = initialSize;
      _increaseSize = initialSize;
      _baseAddress = _rootAddress;
   }

   ~VMemRuntime()
   {
      platform::freeMemory(_rootAddress, _sizeLimit);
   }

   asmjit::Ptr getBaseAddress() const noexcept
   {
      return _baseAddress;
   }

   size_t getSizeLimit() const noexcept
   {
      return _sizeLimit;
   }

   ASMJIT_API virtual asmjit::Error add(void** dst, asmjit::Assembler* assembler) noexcept
   {
      size_t codeSize = assembler->getCodeSize();
      auto cursorPos = _baseAddress - _rootAddress;

      // If there was no code generated, this is an error
      if (codeSize == 0) {
         *dst = nullptr;
         return asmjit::kErrorNoCodeGenerated;
      }

      // If we don't have any room left, we need to error
      if (cursorPos + codeSize > _sizeLimit) {
         *dst = nullptr;
         return asmjit::kErrorCodeTooLarge;
      }

      // If this code block will push us past our commited region
      // we need to commit more memory to the JIT code block.
      
      while (cursorPos + codeSize > _committedSize) {
         if (!platform::commitMemory(_rootAddress + _committedSize, _increaseSize, platform::ProtectFlags::ReadWriteExecute)) {
            *dst = nullptr;
            return asmjit::kErrorCodeTooLarge;
         }
         _committedSize += _increaseSize;
      }

      asmjit::Ptr baseAddress = _baseAddress;
      uint8_t* p = static_cast<uint8_t*>((void*)static_cast<uintptr_t>(baseAddress));

      // Since the base address is known the `relocSize` returned should be equal
      // to `codeSize`. It's better to fail if they don't match instead of passsing
      // silently.
      size_t relocSize = assembler->relocCode(p, baseAddress);
      if (relocSize == 0 || codeSize != relocSize) {
         *dst = nullptr;
         return asmjit::kErrorInvalidState;
      }

      _baseAddress += codeSize;

      flush(p, codeSize);
      *dst = p;

      return asmjit::kErrorOk;
   }

   ASMJIT_API virtual asmjit::Error release(void* p) noexcept
   {
      return asmjit::kErrorOk;
   }

   size_t _increaseSize;
   size_t _committedSize;
   asmjit::Ptr _rootAddress;
   
};

} // namespace jit
} // namespace cpu
