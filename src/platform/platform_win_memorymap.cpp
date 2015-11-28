#include "platform.h"
#include "platform_memorymap.h"

#ifdef PLATFORM_WINDOWS
#include <Windows.h>

namespace platform
{

struct MemoryMappedFile
{
   HANDLE handle = nullptr;
   size_t size = 0;
};

MemoryMappedFile *
createMemoryMappedFile(size_t size)
{
   auto file = new MemoryMappedFile();
   auto lo = static_cast<DWORD>(size & 0xFFFFFFFF);
   auto hi = static_cast<DWORD>(size >> 32);
   file->handle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE | SEC_RESERVE, hi, lo, NULL);
   file->size = size;

   if (!file->handle) {
      delete file;
      return nullptr;
   }

   return file;
}

void
destroyMemoryMappedFile(MemoryMappedFile *file)
{
   CloseHandle(file->handle);
   delete file;
}

bool
mapMemory(MemoryMappedFile *file, size_t offset, size_t address, size_t size)
{
   if (!file || offset > file->size || offset + size > file->size) {
      return false;
   }

   auto baseAddress = reinterpret_cast<LPVOID>(address);
   auto offsetLo = static_cast<DWORD>(offset & 0xFFFFFFFF);
   auto offsetHi = static_cast<DWORD>(offset >> 32);
   auto result = MapViewOfFileEx(file->handle, FILE_MAP_WRITE, offsetHi, offsetLo, size, baseAddress);

   if (result != baseAddress) {
      unmapMemory(file, address, size);
      return false;
   }

   return true;
}

bool
unmapMemory(MemoryMappedFile *file, size_t address, size_t size)
{
   auto baseAddress = reinterpret_cast<LPVOID>(address);
   return !!UnmapViewOfFile(baseAddress);
}

bool
commitMemory(MemoryMappedFile *file, size_t address, size_t size)
{
   auto baseAddress = reinterpret_cast<LPVOID>(address);
   return !!VirtualAlloc(baseAddress, size, MEM_COMMIT, PAGE_READWRITE);
}

bool
protectMemory(size_t address, size_t size)
{
   auto baseAddress = reinterpret_cast<LPVOID>(address);
   return !!VirtualAlloc(baseAddress, size, MEM_RESERVE, PAGE_NOACCESS);
}

} // namespace platform

#endif
