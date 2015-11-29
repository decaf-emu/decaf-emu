#include "platform.h"
#include "platform_memorymap.h"

#ifdef PLATFORM_POSIX
#include <sys/mman.h>

namespace platform
{

struct MemoryMappedFile
{
   size_t size;
};

MemoryMappedFile *
createMemoryMappedFile(size_t size)
{
   auto file = new MemoryMappedFile();
   file->size = size;
   return file;
}

void
destroyMemoryMappedFile(MemoryMappedFile *file)
{
   delete file;
}

bool
mapMemory(MemoryMappedFile *file, size_t offset, size_t address, size_t size)
{
   if (!file || offset > file->size || offset + size > file->size) {
      return false;
   }

   auto baseAddress = reinterpret_cast<void *>(address);
   auto result = mmap(baseAddress, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS, -1, 0);

   if (result != baseAddress) {
      unmapMemory(file, address, size);
      return false;
   }

   return true;
}

bool
unmapMemory(MemoryMappedFile *file, size_t address, size_t size)
{
   auto baseAddress = reinterpret_cast<void *>(address);
   return !munmap(baseAddress, size);
}

bool
commitMemory(MemoryMappedFile *file, size_t address, size_t size)
{
   // mmap commits memory
   return true;
}

bool
protectMemory(size_t address, size_t size)
{
   auto baseAddress = reinterpret_cast<void *>(address);
   return mprotect(baseAddress, size, PROT_NONE) == 0;
}

} // namespace platform

#endif
