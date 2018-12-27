#include "decaf_debug_api.h"

#include <libcpu/mem.h>

namespace decaf::debug
{

size_t
readMemory(VirtualAddress address, void *dst, size_t size)
{
   auto out = reinterpret_cast<uint8_t *>(dst);
   auto bytesRead = size_t { 0 };

   while (bytesRead < size) {
      if (!cpu::isValidAddress(cpu::VirtualAddress { address })) {
         break;
      }

      *(out++) = mem::read<uint8_t>(address++);
      ++bytesRead;
   }

   return bytesRead;
}

size_t
writeMemory(VirtualAddress address, const void *src, size_t size)
{
   auto in = reinterpret_cast<const uint8_t *>(src);
   auto bytesWritten = size_t { 0 };

   while (bytesWritten < size) {
      if (!cpu::isValidAddress(cpu::VirtualAddress { address })) {
         break;
      }

      mem::write<uint8_t>(address++, *(in++));
      ++bytesWritten;
   }

   return bytesWritten;
}

} // namespace decaf::debug
