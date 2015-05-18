#pragma once
#include <cassert>
#include <Windows.h>
#include "log.h"

enum MemoryMap : size_t
{
   ApplicationCode = 0x02000000,
   ApplicationData = 0x10000000,
   ApplicationMemoryEnd = 0x02000000 + 0x40000000, // 1GB
   GraphicsResources = 0xF4000000,
   GraphicsResourcesEnd = 0xF6000000
};

class Memory
{
   struct Allocation
   {
      size_t physical;
      size_t size;
   };

public:
   void initialise();
   size_t translate(size_t address);
   size_t allocate(size_t address, size_t size);
   void free(size_t address);

private:
   size_t mVirtualAddress;
   size_t mAddress;
   size_t mMemorySize;
};

extern Memory gMemory;
