#pragma once
#include <cstddef>

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

bool
reserveMemory(size_t address, size_t size);

bool
freeMemory(size_t address, size_t size);

bool
commitMemory(size_t address, size_t size, ProtectFlags flags = ProtectFlags::ReadWrite);

bool
uncommitMemory(size_t address, size_t size);

bool
protectMemory(size_t address, size_t size, ProtectFlags flags);

}
