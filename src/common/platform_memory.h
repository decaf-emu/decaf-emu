#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

namespace platform
{

enum class ProtectFlags
{
   NoAccess,
   ReadOnly,
   ReadWrite,
   ReadExecute,
   ReadWriteExecute
};

using MapFileHandle = intptr_t;
static constexpr MapFileHandle InvalidMapFileHandle = -1;

size_t
getSystemPageSize();

MapFileHandle
createMemoryMappedFile(size_t size);

MapFileHandle
openMemoryMappedFile(const std::string &path,
                     ProtectFlags flags,
                     size_t *outSize);

bool
closeMemoryMappedFile(MapFileHandle handle);

void *
mapViewOfFile(MapFileHandle handle,
              ProtectFlags flags,
              size_t offset,
              size_t size,
              void *dst = nullptr);

bool
unmapViewOfFile(void *view,
                size_t size);

bool
reserveMemory(uintptr_t address,
              size_t size);

bool
freeMemory(uintptr_t address,
           size_t size);

bool
commitMemory(uintptr_t address,
             size_t size,
             ProtectFlags flags = ProtectFlags::ReadWrite);

bool
uncommitMemory(uintptr_t address,
               size_t size);

bool
protectMemory(uintptr_t address,
              size_t size,
              ProtectFlags flags);

} // namespace platform
