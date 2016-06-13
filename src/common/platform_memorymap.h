#pragma once
#include <cstddef>

namespace platform
{

struct MemoryMappedFile;

MemoryMappedFile *
createMemoryMappedFile(size_t size);

void
destroyMemoryMappedFile(MemoryMappedFile *file);

bool
mapMemory(MemoryMappedFile *file, size_t offset, size_t address, size_t size);

bool
unmapMemory(MemoryMappedFile *file, size_t address, size_t size);

bool
commitMemory(MemoryMappedFile *file, size_t address, size_t size);

bool
protectMemory(size_t address, size_t size);

}
