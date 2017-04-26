#include "platform.h"
#include "platform_memory.h"
#include "log.h"

#ifdef PLATFORM_WINDOWS
#include <Windows.h>

namespace platform
{

static DWORD
flagsToPageProtect(ProtectFlags flags)
{
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


static DWORD
flagsToFileMap(ProtectFlags flags)
{
   switch (flags) {
   case ProtectFlags::ReadOnly:
      return FILE_MAP_READ;
   case ProtectFlags::ReadWrite:
      return FILE_MAP_WRITE;
   case ProtectFlags::ReadExecute:
      return FILE_MAP_READ | FILE_MAP_EXECUTE;
   case ProtectFlags::ReadWriteExecute:
      return FILE_MAP_READ | FILE_MAP_WRITE | FILE_MAP_EXECUTE;
   case ProtectFlags::NoAccess:
   default:
      return 0;
   }
}


size_t
getSystemPageSize()
{
   SYSTEM_INFO info;
   GetSystemInfo(&info);
   return static_cast<size_t>(info.dwAllocationGranularity);
}


MapFileHandle
createMemoryMappedFile(size_t size)
{
   auto sizeLo = static_cast<DWORD>(size & 0xFFFFFFFFu);
   auto sizeHi = static_cast<DWORD>((size >> 32) & 0xFFFFFFFFu);
   auto handle = CreateFileMappingW(INVALID_HANDLE_VALUE,
                                    NULL,
                                    SEC_COMMIT | PAGE_READWRITE,
                                    sizeHi,
                                    sizeLo,
                                    NULL);

   if (!handle) {
      gLog->error("createMemoryMappedFile(0x{:X}) failed with error: {}",
                  size, GetLastError());
      return InvalidMapFileHandle;
   }

   return reinterpret_cast<MapFileHandle>(handle);
}


bool
closeMemoryMappedFile(MapFileHandle handle)
{
   if (!CloseHandle(reinterpret_cast<HANDLE>(handle))) {
      gLog->error("closeMemoryMappedFile({}) failed with error: {}",
                  handle, GetLastError());
      return false;
   }

   return true;
}


void *
mapViewOfFile(MapFileHandle handle,
              ProtectFlags flags,
              size_t offset,
              size_t size,
              void *dst)
{
   auto access = flagsToFileMap(flags);
   auto offsetLo = static_cast<DWORD>(offset & 0xFFFFFFFFu);
   auto offsetHi = static_cast<DWORD>((offset >> 32) & 0xFFFFFFFFu);
   auto result = MapViewOfFileEx(reinterpret_cast<HANDLE>(handle),
                                 access,
                                 offsetHi,
                                 offsetLo,
                                 size,
                                 dst);

   if (result == nullptr) {
      gLog->error("mapViewOfFile(offset: 0x{:X}, size: 0x{:X}, dst: 0x{:X}) failed with error: {}",
                  offset, size, dst, GetLastError());
   }

   return result;
}


bool
unmapViewOfFile(void *view,
                size_t size)
{
   if (!UnmapViewOfFile(view)) {
      gLog->error("unmapViewOfFile(view: 0x{:X}, size: 0x{:X}) failed with error: {}",
                  view, size, GetLastError());
      return false;
   }

   return true;
}


bool
reserveMemory(uintptr_t address,
              size_t size)
{
   auto baseAddress = reinterpret_cast<LPVOID>(address);
   auto result = VirtualAlloc(baseAddress, size, MEM_RESERVE, PAGE_NOACCESS);

   if (result != baseAddress) {
      gLog->debug("reserveMemory(address: 0x{:X}, size: 0x{:X}) failed with error: {}",
                  address, size, GetLastError());
      return false;
   }

   return true;
}


bool
freeMemory(uintptr_t address,
           size_t size)
{
   auto baseAddress = reinterpret_cast<LPVOID>(address);

   if (!VirtualFree(baseAddress, 0, MEM_RELEASE)) {
      gLog->error("freeMemory(address: {}, size: {}) failed with error: {}",
                  address, size, GetLastError());
      return false;
   }

   return true;
}


bool
commitMemory(uintptr_t address,
             size_t size,
             ProtectFlags flags)
{
   auto baseAddress = reinterpret_cast<LPVOID>(address);
   auto result = VirtualAlloc(baseAddress,
                              size,
                              MEM_COMMIT,
                              flagsToPageProtect(flags));

   if (result != baseAddress) {
      gLog->error("commitMemory(address: 0x{:X}, size: 0x{:X}, flags: {}) failed with error: {}",
                  address, size, static_cast<int>(flags), GetLastError());
      return false;
   }

   return true;
}


bool
uncommitMemory(uintptr_t address,
               size_t size)
{
   auto baseAddress = reinterpret_cast<LPVOID>(address);

   if (!VirtualFree(baseAddress, size, MEM_DECOMMIT)) {
      gLog->error("uncommitMemory(address: 0x{:X}, size: 0x{:X}) failed with error: {}",
                  address, size, GetLastError());
      return false;
   }

   return true;
}


bool
protectMemory(uintptr_t address,
              size_t size,
              ProtectFlags flags)
{
   auto baseAddress = reinterpret_cast<LPVOID>(address);
   DWORD oldProtect;

   if (!VirtualProtect(baseAddress, size, flagsToPageProtect(flags), &oldProtect)) {
      gLog->error("protectMemory(address: 0x{:X}, size: 0x{:X}, flags: {}) failed with error: {}",
                  address, size, static_cast<int>(flags), GetLastError());
      return false;
   }

   return true;
}

} // namespace platform

#endif
