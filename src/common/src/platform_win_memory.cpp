#include "platform.h"

#ifdef PLATFORM_WINDOWS
#include "decaf_assert.h"
#include "log.h"
#include "platform_memory.h"

#define WIN32_LEAN_AND_MEAN
#include <map>
#include <mutex>
#include <Windows.h>

namespace platform
{

struct WindowsMapFileHandle
{
   HANDLE fileHandle;
   HANDLE mappingHandle;
};

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

   auto windowsHandle = new WindowsMapFileHandle { };
   windowsHandle->mappingHandle = handle;
   windowsHandle->fileHandle = NULL;
   return reinterpret_cast<MapFileHandle>(windowsHandle);
}


MapFileHandle
openMemoryMappedFile(const std::string &path,
                     ProtectFlags flags,
                     size_t *outSize)
{
   // Only support READ ONLY for now
   decaf_check(flags == ProtectFlags::ReadOnly);
   OFSTRUCT of;
   auto fileHandle = reinterpret_cast<HANDLE>(static_cast<uintptr_t>(OpenFile(path.c_str(), &of, OF_READ)));
   if (!fileHandle) {
      gLog->error("openMemoryMappedFile(\"{}\") OpenFile failed with error: {}",
                  path, GetLastError());
      return InvalidMapFileHandle;
   }

   auto mapHandle = CreateFileMappingW(fileHandle, NULL, PAGE_READONLY, 0, 0, NULL);
   if (!mapHandle) {
      CloseHandle(fileHandle);
      gLog->error("openMemoryMappedFile(\"{}\") CreateFileMapping failed with error: {}",
                  path, GetLastError());
      return InvalidMapFileHandle;
   }

   if (outSize) {
      LARGE_INTEGER size;
      GetFileSizeEx(fileHandle, &size);
      *outSize = static_cast<size_t>(size.QuadPart);
   }

   auto windowsHandle = new WindowsMapFileHandle { };
   windowsHandle->mappingHandle = mapHandle;
   windowsHandle->fileHandle = fileHandle;
   return reinterpret_cast<MapFileHandle>(windowsHandle);
}


bool
closeMemoryMappedFile(MapFileHandle handle)
{
   auto windowsHandle = reinterpret_cast<WindowsMapFileHandle *>(handle);

   if (windowsHandle->fileHandle) {
      if (!CloseHandle(windowsHandle->fileHandle)) {
         gLog->error("closeMemoryMappedFile({}) close file handle failed with error: {}",
                     handle, GetLastError());
      }

      windowsHandle->fileHandle = NULL;
   }

   if (windowsHandle->mappingHandle) {
      if (!CloseHandle(windowsHandle->mappingHandle)) {
         gLog->error("closeMemoryMappedFile({}) close map handle failed with error: {}",
                     handle, GetLastError());
         return false;
      }

      windowsHandle->mappingHandle = NULL;
   }

   delete windowsHandle;
   return true;
}


void *
mapViewOfFile(MapFileHandle handle,
              ProtectFlags flags,
              size_t offset,
              size_t size,
              void *dst)
{
   auto windowsHandle = reinterpret_cast<WindowsMapFileHandle *>(handle);
   auto access = flagsToFileMap(flags);
   auto offsetLo = static_cast<DWORD>(offset & 0xFFFFFFFFu);
   auto offsetHi = static_cast<DWORD>((offset >> 32) & 0xFFFFFFFFu);
   auto result = MapViewOfFileEx(reinterpret_cast<HANDLE>(windowsHandle->mappingHandle),
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
