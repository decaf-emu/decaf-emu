#include "decaf_debug_api.h"

#include <libcpu/mem.h>

namespace decaf::debug
{

bool
isValidVirtualAddress(VirtualAddress address)
{
   return cpu::isValidAddress(cpu::VirtualAddress { address });
}

size_t
getMemoryPageSize()
{
   return cpu::PageSize;
}

size_t
readMemory(VirtualAddress address, void *dst, size_t size)
{
   auto out = reinterpret_cast<uint8_t *>(dst);
   constexpr auto pageMask = ~(cpu::PageSize - 1);
   auto bytesRemaining = size;

   // Copy bytes, checking validity of each memory page as we cross it
   while (bytesRemaining > 0) {
      auto currentPage = address & pageMask;
      if (!cpu::isValidAddress(cpu::VirtualAddress { currentPage })) {
         break;
      }

      auto nextPage = currentPage + cpu::PageSize;
      auto readBytes = std::min<size_t>(bytesRemaining, nextPage - address);

      std::memcpy(out, mem::translate<uint8_t>(address), readBytes);
      address += static_cast<uint32_t>(readBytes);
      out += readBytes;
      bytesRemaining -= readBytes;
   }

   return size - bytesRemaining;
}

size_t
writeMemory(VirtualAddress address, const void *src, size_t size)
{
   auto in = reinterpret_cast<const uint8_t *>(src);
   constexpr auto pageMask = ~(cpu::PageSize - 1);
   auto bytesRemaining = size;

   // Copy bytes, checking validity of each memory page as we cross it
   while (bytesRemaining > 0) {
      auto currentPage = address & pageMask;
      if (!cpu::isValidAddress(cpu::VirtualAddress{ currentPage })) {
         break;
      }

      auto nextPage = currentPage + cpu::PageSize;
      auto readBytes = std::min<size_t>(bytesRemaining, nextPage - address);

      std::memcpy(mem::translate<uint8_t>(address), in, readBytes);
      address += static_cast<uint32_t>(readBytes);
      in += readBytes;
      bytesRemaining -= readBytes;
   }

   return size - bytesRemaining;
}

} // namespace decaf::debug
