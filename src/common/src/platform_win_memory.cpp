#include "platform.h"
#include "platform_memory.h"

#ifdef PLATFORM_WINDOWS
#include <Windows.h>

namespace platform
{

static DWORD flagsToPageProtect(ProtectFlags flags) {
   switch (flags) {
   case ProtectFlags::ReadOnly:
      return PAGE_READONLY;
   case ProtectFlags::ReadWrite:
      return PAGE_READWRITE;
   case ProtectFlags::ReadExecute:
      return PAGE_EXECUTE_READ;
   case ProtectFlags::ReadWriteExecute:
      return PAGE_EXECUTE_READWRITE;
   case ProtectFlags::NoAccess:
   default:
      return PAGE_NOACCESS;
   }
}

bool
reserveMemory(size_t address, size_t size)
{
   auto baseAddress = reinterpret_cast<LPVOID>(address);
   auto result = VirtualAlloc(baseAddress, size, MEM_RESERVE, PAGE_NOACCESS);
   if (result != baseAddress) {
      return false;
   }

   return true;
}

bool
freeMemory(size_t address, size_t size)
{
   auto baseAddress = reinterpret_cast<LPVOID>(address);
   return VirtualFree(baseAddress, size, MEM_RELEASE) != 0;
}

bool
commitMemory(size_t address, size_t size, ProtectFlags flags)
{
   auto baseAddress = reinterpret_cast<LPVOID>(address);
   auto result = VirtualAlloc(baseAddress, size, MEM_COMMIT, flagsToPageProtect(flags));

   if (result != baseAddress) {
      return false;
   }

   return true;
}

bool
uncommitMemory(size_t address, size_t size)
{
   auto baseAddress = reinterpret_cast<LPVOID>(address);
   return VirtualFree(baseAddress, size, MEM_DECOMMIT) != 0;
}

bool
protectMemory(size_t address, size_t size, ProtectFlags flags)
{
   auto baseAddress = reinterpret_cast<LPVOID>(address);
   DWORD oldProtect;
   auto result = VirtualProtect(baseAddress, size, flagsToPageProtect(flags), &oldProtect);
   return (result != 0);
}

} // namespace platform

#endif
